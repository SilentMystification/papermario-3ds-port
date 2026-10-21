#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ultra64.h"
#include "nu/nusys.h"
#include "enums.h"
#include "pm_port.h"
#include "pm_tex.h"
#include "pm_au2.h"
#include "rom_remap.h"

void osInvalICache(void* addr, s32 nbytes) {
    (void)addr;
    (void)nbytes;
}

/* The R4300 TLB does not exist here. Effect overlays are linked in, so nothing needs mapping. */
void osMapTLB(s32 index, OSPageMask pm, void* vaddr, u32 evenpaddr, u32 oddpaddr, s32 pagesize) {
    (void)index; (void)pm; (void)vaddr; (void)evenpaddr; (void)oddpaddr; (void)pagesize;
}
void osUnmapTLB(s32 index) { (void)index; }
void osUnmapTLBAll(void) {}

void bcopy(const void* src, void* dst, unsigned int n) {
    memcpy(dst, src, n);
}

s32 osTvType = OS_TV_NTSC;
u32 osMemSize = 0x00400000;
OSViMode osViModeNtscLan1;
OSViMode osViModeMpalLan1;

static void* cur_fb;
extern u16 gFrameBuf0[]; /* M01: the real 150KB frame buffer is the fallback, no second 150KB array */
static FILE* rom;
static u8* rom_bytes;
static u32 rom_size;
static int rom_tried;
static int rom_remap; /* 1: rom_bytes is the packed image (pm_rom_keep), 0: full image or file stream */

/* V26: the N64 counts from the reset, so the game's gRandSeed += osGetCount() (main.c) starts the same on every boot and the demo scenes
 * (Bill Blasters fire by RandInt) play the same way. The 3DS tick counts from the console start, so the seed was random per boot.
 * Count from the first call, in the N64 rate (46.875 MHz). A boot salt (the tick since console start) is added unless the switch fixedseed is set. The 32 bit tick wraps every 16 s, enough for deltas of the seed call. */
u32 osGetCount(void) {
    static u32 t0;
    static int started;
    static u32 salt;
    u32 t = pm_ticks();
    if (!started) { started = 1; t0 = t; salt = (pm_debug_has("fixedseed") || pm_opt.fixedseed) ? 0 : t * 2654435761u; } /* R-SEED: random start seed each boot, the switch fixedseed (tests, golden, smoke) keeps it the same */
    return salt + (u32)((unsigned long long)(t - t0) * 46875u / 268111u);
}

u32 osVirtualToPhysical(void* addr) {
    u32 p = (u32)(unsigned long)addr;
    if (p >= 0x80000000u && p < 0x80800000u) return p - 0x80000000u;
    return p;
}

void osViSetMode(OSViMode* mode) { (void)mode; }
void osViSetSpecialFeatures(u32 func) { (void)func; }

void* osViGetCurrentFramebuffer(void) {
    return cur_fb ? cur_fb : (void*)gFrameBuf0;
}

void osViSwapBuffer(void* fb) { cur_fb = fb; }

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count) {
    if (!mq) return;
    mq->mtqueue = NULL;
    mq->fullqueue = NULL;
    mq->validCount = 0;
    mq->first = 0;
    mq->msgCount = count;
    mq->msg = msg;
}

s32 osSendMesg(OSMesgQueue* mq, OSMesg mesg, s32 block) {
    (void)block;
    if (!mq || mq->msgCount <= 0 || !mq->msg) return -1;
    if (mq->validCount >= mq->msgCount) return -1;
    /* P18: first < msgCount and validCount < msgCount, so one subtract replaces the modulo. Same result. */
    s32 i = mq->first + mq->validCount;
    if (i >= mq->msgCount) i -= mq->msgCount;
    mq->msg[i] = mesg;
    mq->validCount++;
    return 0;
}

s32 osRecvMesg(OSMesgQueue* mq, OSMesg* mesg, s32 block) {
    if (!mq || mq->msgCount <= 0) return -1;
    /* Only the audio manager thread blocks. It gives control back to the game thread until a message arrives. */
    while (mq->validCount <= 0) {
        if (!block || !pm_au_thread_yield()) return -1;
    }
    if (mesg) *mesg = mq->msg[mq->first];
    mq->first = mq->first + 1;
    if (mq->first >= mq->msgCount) mq->first = 0; /* P18: same result as the modulo */
    mq->validCount--;
    return 0;
}

/* The audio thread of the N64 nusys scheduler. nusys itself is not built. */
NUSched nusched = { .retraceCount = 1 }; /* one audio frame per 60 Hz retrace */
static NUScClient* au_client;
static OSThread* au_os_thread;
static void (*au_entry)(void*);
static void* au_arg;
OSPiHandle* nuPiCartHandle;

void osCreateThread(OSThread* t, OSId id, void (*entry)(void*), void* arg, void* sp, OSPri p) {
    (void)sp; (void)p;
    t->id = id;
    if (id == THREAD_ID_AUDIO) {
        au_os_thread = t;
        au_entry = entry;
        au_arg = arg;
    }
}

void osStartThread(OSThread* t) {
    if (t == au_os_thread && au_entry) pm_au_thread_start(au_entry, au_arg);
}

OSIntMask osSetIntMask(OSIntMask m) { return m; }

void nuScAddClient(NUScClient* c, OSMesgQueue* mq, NUScMsg msgType) {
    c->msgQ = mq;
    c->msgType = msgType;
    c->next = nusched.clientList;
    nusched.clientList = c;
    if (msgType & NU_SC_RETRACE_MSG) au_client = c;
}

/* AU1c: the driver role, called on the audio thread from pm_au_thread_yield. nusys is not built: this plays the part of the retrace
 * source, the RSP and the scheduler. Each audio task nuAuMgr submits is finished at once (the work ran in pm_rsp_audio.c). */
int pm_audio_serve(void) {
    int any = 0;
    OSMesg m;
    if (!nusched.audioRequestMQ.msgCount) return 0;
    while (osRecvMesg(&nusched.audioRequestMQ, &m, OS_MESG_NOBLOCK) == 0) {
        NUScTask* t = (NUScTask*)m;
        osSendMesg(t->msgQ, t->msg, OS_MESG_NOBLOCK);
        any = 1;
    }
    return any;
}

static unsigned a_t0;
static int a_in;
void pm_audio_retrace_send(void) {
    if (!au_client) return;
    if (!nusched.audioRequestMQ.msgCount) {
        osCreateMesgQueue(&nusched.audioRequestMQ, nusched.audioRequestBuf, NU_SC_MAX_MESGS);
        nusched.retraceMsg = NU_SC_RETRACE_MSG;
    }
    a_t0 = pm_ticks();
    a_in = 1;
    osSendMesg(au_client->msgQ, (OSMesg)&nusched.retraceMsg, OS_MESG_NOBLOCK);
}

/* The retrace is finished (nuAuMgr is waiting for the next one): add its cost. 268111856 ticks per second on the ARM11. */
void pm_audio_tick_end(void) {
    static unsigned ticks, worst;
    static unsigned long long sum;
    if (!a_in) return;
    a_in = 0;
    unsigned dt = pm_ticks() - a_t0;
    sum += dt;
    if (dt > worst) worst = dt;
    /* L01: every 2 s with the audiostat switch, else every 20 s (S28 reads this line on hardware) */
    if (++ticks == (pm_debug_has("audiostat") ? 120u : 1200u)) {
        pm_log_d("audio cost avg %u us max %u us per 60 Hz tick", (unsigned)((unsigned long long)sum * 1000 / ticks / 268112), (unsigned)((unsigned long long)worst * 1000 / 268112));
        ticks = worst = 0;
        sum = 0;
    }
}

/* Expected Paper Mario (US 1.0, NMQE) ROM: size, z64 magic, FNV-1a of the first 64 KB. A full SHA1 of 40 MB on the ARM11 would
 * take seconds, the first 64 KB covers the header and the boot code and costs one small read. Measured on the verified ROM
 * (SHA1 3837f44cda784b466c9a2d99df70d77c322b97a0 as in ver/us/checksum.sha1). */
#define ROM_HEAD_BYTES 0x10000u
#define ROM_HEAD_FNV 0x0244636Cu

static int rom_fail(FILE* f, const char* msg) {
    if (f) fclose(f);
    pm_fatal_screen(msg);
    return 0;
}

/* Returns 1 when the ROM can be used. On 0 the reason is on the bottom screen and the game must not run. */
int pm_rom_init(void) {
    if (rom_tried) return rom_bytes != NULL || rom != NULL;
    rom_tried = 1;
    FILE* f = fopen("papermario.z64", "rb");
    if (!f) f = fopen("baserom.us.z64", "rb");
    if (!f) return rom_fail(NULL, "ROM missing. Copy the US Paper Mario ROM to sdmc:/3ds/PaperMario/papermario.z64 (big-endian .z64).");
    if (fseek(f, 0, SEEK_END) != 0) return rom_fail(f, "Cannot read the ROM size. Check the SD card.");
    long sz = ftell(f);
    rewind(f);
    {
        u8* head = (u8*)malloc(ROM_HEAD_BYTES);
        size_t got = head ? fread(head, 1, ROM_HEAD_BYTES, f) : 0;
        char msg[200];
        int bad = 0;
        if (!head || got != ROM_HEAD_BYTES) {
            free(head);
            return rom_fail(f, "Cannot read the ROM header. The file is too small or the SD card failed.");
        }
        u32 magic = ((u32)head[0] << 24) | ((u32)head[1] << 16) | ((u32)head[2] << 8) | head[3];
        u32 h = 2166136261u;
        for (u32 i = 0; i < ROM_HEAD_BYTES; i++) h = (h ^ head[i]) * 16777619u;
        free(head);
        if (magic == 0x37804012u || magic == 0x40123780u) {
            snprintf(msg, sizeof(msg), "ROM is byte-swapped (.v64 or .n64). Convert it to big-endian .z64.");
            bad = 1;
        } else if (magic != 0x80371240u) {
            snprintf(msg, sizeof(msg), "File is not an N64 ROM (header %08x).", (unsigned)magic);
            bad = 1;
        } else if (sz != (long)PM_ROM_FULL) {
            snprintf(msg, sizeof(msg), "ROM size is %ld bytes, expected %u. Use the US 1.0 ROM.", sz, (unsigned)PM_ROM_FULL);
            bad = 1;
        } else if (h != ROM_HEAD_FNV) {
            snprintf(msg, sizeof(msg), "Wrong ROM version (header check %08x). Use the US 1.0 ROM.", (unsigned)h);
            bad = 1;
        }
        if (bad) return rom_fail(f, msg);
        rewind(f);
    }
    /* M01: keep only the ranges in pm_rom_keep, packed back to back. Offsets the game uses stay the original ones. */
    int can_pack = sz == (long)PM_ROM_FULL && !pm_debug_has("romfull");
    unsigned need = can_pack ? PM_ROM_PACKED : (unsigned)sz;
    /* ROMFIT: leave 3.5MB of the heap for the game (build 1011: 4MB left a cap 38KB under the packed image). */
    unsigned leave = 3584u * 1024u;
    unsigned heap = pm_heap_bytes();
    unsigned cap = heap > leave ? heap - leave : 0;
    if (sz > 0 && need > cap) pm_log_e("ROM does not fit: need %u KB cap %u KB, streaming from the SD card", need >> 10, cap >> 10);
    if (sz > 0 && need <= cap) {
        u8* block = (u8*)memalign(16, (size_t)need);
        int ok = block != NULL;
        if (ok && can_pack) {
            for (unsigned i = 0; ok && i < sizeof(pm_rom_keep) / sizeof(pm_rom_keep[0]); i++)
                ok = fseek(f, (long)pm_rom_keep[i].start, SEEK_SET) == 0 &&
                     fread(block + pm_rom_keep[i].packed_off, 1, pm_rom_keep[i].len, f) == pm_rom_keep[i].len;
        } else if (ok) {
            ok = fread(block, 1, (size_t)sz, f) == (size_t)sz;
        }
        if (ok) {
            rom_bytes = block;
            rom_size = (u32)sz;
            rom_remap = can_pack;
            fclose(f);
            pm_log_w("rom resident %uKB of %uKB%s, cap %uKB, margin %uKB", need >> 10, rom_size >> 10, can_pack ? " (packed)" : "", cap >> 10, (cap - need) >> 10);
            pm_log_d("ram n64 4MB, rom %uMB, port %uMB", (need + 0xFFFFF) >> 20, pm_linear_bytes() >> 20);
            return 1;
        }
        free(block);
    }
    rewind(f);
    rom = f;
    rom_size = (u32)sz; /* ROMFIT: the streaming read checks rom_size, 0 turned every read into PAST END plus zeros */
    pm_log_w("rom streaming (%ld bytes, need %uKB, cap %uKB)", sz, need >> 10, cap >> 10);
    pm_log_d("ram n64 4MB, port %uMB", pm_linear_bytes() >> 20);
    return 1;
}

/* R01: a ROM read that no real asset makes. Logged once per address through the non-blocking log. With the debug switch
 * romstrict it also stops the program (pm_fatal_stop: FATAL line, [CRASH] line, svcBreak), so the run script stops.
 * Placeholder ROM symbols (icon_ROM_START was 1, H04) show up here as a tiny offset. */
static void rom_suspect(const char* why, u32 addr, u32 n, u32 req, u32 size) {
    static u32 seen[512];
    static unsigned cnt;
    for (unsigned i = 0; i < cnt; i++)
        if (seen[i] == addr) return;
    if (cnt < 512) seen[cnt++] = addr;
    pm_log_w("ROM READ %s %08x n %u (req %08x n %u)", why, addr, n, req, size);
    if (pm_debug_has("romstrict")) {
        char m[80];
        snprintf(m, sizeof(m), "romstrict, ROM read %s at %08x n %u", why, addr, n);
        pm_fatal_stop(m);
    }
}

/* HW3: both tables are generated sorted and without overlap (121 entries each). A read used to scan all 242 entries. The binary
 * search gives the same n and the same offset. pm_rom_sorted is checked once; an unsorted table keeps the linear scan. */
static int pm_rom_sorted = -1;
static void rom_check_sorted(void) {
    int ok = 1;
    unsigned i, nk = sizeof(pm_rom_keep) / sizeof(pm_rom_keep[0]), nd = sizeof(pm_rom_drop) / sizeof(pm_rom_drop[0]);
    for (i = 0; i + 1 < nk; i++) if (pm_rom_keep[i].start + pm_rom_keep[i].len > pm_rom_keep[i + 1].start) ok = 0;
    for (i = 0; i + 1 < nd; i++) if (pm_rom_drop[i].end > pm_rom_drop[i + 1].start) ok = 0;
    for (i = 0; i < nd; i++) if (pm_rom_drop[i].start >= pm_rom_drop[i].end) ok = 0;
    pm_rom_sorted = ok;
    if (!ok) pm_log_e("rom_remap tables are not sorted, linear scan");
}
/* First drop entry with end > addr, or nd. */
static unsigned rom_drop_first(u32 addr) {
    unsigned lo = 0, hi = sizeof(pm_rom_drop) / sizeof(pm_rom_drop[0]);
    while (lo < hi) {
        unsigned m = (lo + hi) >> 1;
        if (pm_rom_drop[m].end > addr) hi = m; else lo = m + 1;
    }
    return lo;
}
/* Index of the keep entry that holds addr, or -1. */
static int rom_keep_find(u32 addr) {
    int lo = 0, hi = (int)(sizeof(pm_rom_keep) / sizeof(pm_rom_keep[0])) - 1;
    while (lo <= hi) {
        int m = (lo + hi) >> 1;
        if (addr < pm_rom_keep[m].start) hi = m - 1;
        else if (addr >= pm_rom_keep[m].start + pm_rom_keep[m].len) lo = m + 1;
        else return m;
    }
    return -1;
}

static void pi_read_raw(u32 rom_addr, void* buf, u32 size) {
    if (!buf || !size) return;
    if ((unsigned)buf < 0x00100000u) {
        static int once;
        if (!once) {
            once = 1;
            pm_log_d("dma skip rom %08x buf %08x n %u", rom_addr, (unsigned)buf, size);
        }
        return;
    }
    if (!rom_tried) pm_rom_init();
    PH_PUSH(PH_ROM);
    if (!pm_on_audio()) { PH_ADD(C_ROMN, 1); PH_ADD(C_ROMB, size); } /* AU1c: the counters belong to the game thread */
    if (rom_addr < 0x1000u) rom_suspect("AT TINY OFFSET", rom_addr, size, rom_addr, size);
    /* Walk the request over the original offsets. A byte in a dropped range is a hard error. */
    u8* out = (u8*)buf;
    u32 left = size, addr = rom_addr;
    while (left) {
        u32 n = left;
        if (addr >= rom_size) {
            rom_suspect("PAST END OF ROM", addr, left, rom_addr, size);
            memset(out, 0, left);
            break;
        }
        if (n > rom_size - addr) n = rom_size - addr;
        int dropped = 0;
        if (pm_rom_sorted < 0) rom_check_sorted();
        if (pm_rom_sorted) {
            unsigned i = rom_drop_first(addr);
            if (i < sizeof(pm_rom_drop) / sizeof(pm_rom_drop[0])) {
                u32 ds = pm_rom_drop[i].start, de = pm_rom_drop[i].end;
                if (addr >= ds) { dropped = 1; if (n > de - addr) n = de - addr; }
                else if (ds < addr + n) n = ds - addr;
            }
        } else
        for (unsigned i = 0; i < sizeof(pm_rom_drop) / sizeof(pm_rom_drop[0]); i++) {
            u32 ds = pm_rom_drop[i].start, de = pm_rom_drop[i].end;
            if (addr >= ds && addr < de) { dropped = 1; if (n > de - addr) n = de - addr; }
            else if (ds > addr && ds < addr + n) n = ds - addr;
        }
        if (dropped) rom_suspect("IN DROPPED RANGE", addr, n, rom_addr, size);
        if (dropped && !pm_debug_has("romfull")) {
            memset(out, 0, n);
        } else if (rom_bytes) {
            u32 off = addr;
            if (rom_remap && pm_rom_sorted) {
                int k = rom_keep_find(addr);
                if (k >= 0) {
                    off = pm_rom_keep[k].packed_off + (addr - pm_rom_keep[k].start);
                    if (n > pm_rom_keep[k].start + pm_rom_keep[k].len - addr) n = pm_rom_keep[k].start + pm_rom_keep[k].len - addr;
                }
            } else if (rom_remap) {
                for (unsigned i = 0; i < sizeof(pm_rom_keep) / sizeof(pm_rom_keep[0]); i++)
                    if (addr >= pm_rom_keep[i].start && addr < pm_rom_keep[i].start + pm_rom_keep[i].len) {
                        off = pm_rom_keep[i].packed_off + (addr - pm_rom_keep[i].start);
                        if (n > pm_rom_keep[i].start + pm_rom_keep[i].len - addr) n = pm_rom_keep[i].start + pm_rom_keep[i].len - addr;
                    }
            }
            memcpy(out, rom_bytes + off, n);
        } else {
            /* Byte stream, not word-swapped. A display list copied out of the ROM is still
             * big-endian until the loader that knows the type swaps it once. */
            size_t got = 0;
            if (pm_au2_active) pm_au2_rom_lock(); /* AU2: the render thread reads samples while the game thread loads */
            if (rom && fseek(rom, (long)addr, SEEK_SET) == 0) got = fread(out, 1, n, rom);
            if (pm_au2_active) pm_au2_rom_unlock();
            if (got < n) memset(out + got, 0, n - got);
        }
        out += n; addr += n; left -= n;
    }
    if (!pm_on_audio()) pm_tex_invalidate(buf, size); /* AU1c: the audio thread reads sound data, never a texture. The cache belongs to core 0. */
    PH_POP();
}

/* HW3: pointer to len bytes of the resident ROM image at ROM address addr, or NULL (file stream, dropped range, range not in one
 * kept block). The bytes are raw big-endian ROM bytes, without the swaps nuPiReadRom does. Read only. */
const unsigned char* pm_rom_ptr(unsigned addr, unsigned len) {
    if (!rom_tried) pm_rom_init();
    if (!rom_bytes || !len || addr >= rom_size || len > rom_size - addr) return NULL;
    for (unsigned i = 0; i < sizeof(pm_rom_drop) / sizeof(pm_rom_drop[0]); i++)
        if (addr < pm_rom_drop[i].end && addr + len > pm_rom_drop[i].start) return NULL;
    if (!rom_remap) return rom_bytes + addr;
    for (unsigned i = 0; i < sizeof(pm_rom_keep) / sizeof(pm_rom_keep[0]); i++)
        if (addr >= pm_rom_keep[i].start && addr + len <= pm_rom_keep[i].start + pm_rom_keep[i].len)
            return rom_bytes + pm_rom_keep[i].packed_off + (addr - pm_rom_keep[i].start);
    return NULL;
}

/* The map asset table (28-byte entries, see AssetHeader in src/world/world.c) is read by the real load_asset_by_name and
 * get_asset_offset as native u32 fields. The ROM stores them big-endian, so swap the three u32 fields of each entry that
 * lies inside the table. Asset data is not touched. */
#define ASSET_TABLE_START 0x1E40020u
#define ASSET_ENTRY_SIZE 0x1Cu
void nuPiReadRom(u32 rom_addr, void* buf, u32 size) {
    static u32 table_bytes;
    pi_read_raw(rom_addr, buf, size);
    if (!buf || !size || (unsigned)buf < 0x00100000u) return;
    /* S19b: s16 data (Lava Piranha animation scripts). The ROM is big-endian, the reader wants native s16.
     * A read starts and ends on an even offset (dma_copy chunks and the segment bounds are even). */
    for (unsigned i = 0; i < sizeof(pm_rom_swap16) / sizeof(pm_rom_swap16[0]); i++) {
        u32 s = pm_rom_swap16[i].start, e = pm_rom_swap16[i].end;
        u32 from = rom_addr > s ? rom_addr : s, to = rom_addr + size < e ? rom_addr + size : e;
        for (u32 a = from; a + 1 < to; a += 2) {
            u8* f = (u8*)buf + (a - rom_addr), t0 = f[0];
            f[0] = f[1]; f[1] = t0;
        }
    }
    if (table_bytes == 0) {
        u8 b[4];
        pi_read_raw(ASSET_TABLE_START + 0x10, b, 4);
        table_bytes = ((u32)b[0] << 24) | ((u32)b[1] << 16) | ((u32)b[2] << 8) | b[3];
    }
    if ((rom_addr & 3) == 0 && (size & 3) == 0 && rom_addr < ASSET_TABLE_START + table_bytes && rom_addr + size > ASSET_TABLE_START) {
        /* dma_copy reads in chunks, so decide per word from its offset in the table. */
        u32 i;
        for (i = 0; i < size; i += 4) {
            u32 w = rom_addr + i;
            u32 field = (w - ASSET_TABLE_START) % ASSET_ENTRY_SIZE;
            if (w >= ASSET_TABLE_START && w < ASSET_TABLE_START + table_bytes && field >= 0x10) {
                u8* f = (u8*)buf + i;
                u8 t0 = f[0], t1 = f[1];
                f[0] = f[3]; f[1] = f[2]; f[2] = t1; f[3] = t0;
            }
        }
    }
    if (PMDL("romtrace")) {
        /* M01 evidence: address, size, FNV-1a of the bytes the game gets. Own buffered file: pm_log fsyncs every line. */
        static FILE* tr;
        static unsigned lines;
        if (!tr) tr = fopen(pm_debug_has("romfull") ? "romtrace_full.txt" : "romtrace_pack.txt", "w");
        u32 h = 2166136261u;
        for (u32 i = 0; i < size; i++) h = (h ^ ((u8*)buf)[i]) * 16777619u;
        if (tr) {
            fprintf(tr, "%08x %u %08x\n", rom_addr, size, h);
            if ((++lines & 63) == 0) fflush(tr);
        }
    }
}

s32 osPiStartDma(OSIoMesg* mb, s32 priority, s32 direction, u32 devAddr, void* vAddr, u32 nbytes, OSMesgQueue* mq) {
    (void)priority;
    if (direction == OS_READ) nuPiReadRom(devAddr, vAddr, nbytes);
    if (mq) osSendMesg(mq, (OSMesg)mb, OS_MESG_NOBLOCK);
    return 0;
}

s32 osEPiStartDma(OSPiHandle* handle, OSIoMesg* mb, s32 direction) {
    (void)handle;
    if (direction == OS_READ) nuPiReadRom(mb->devAddr, mb->dramAddr, mb->size);
    if (mb->hdr.retQueue) osSendMesg(mb->hdr.retQueue, (OSMesg)mb, OS_MESG_NOBLOCK);
    return 0;
}

s32 osAiSetFrequency(u32 hz) {
    pm_audio_post(NULL, 0, (unsigned)hz);
    return (s32)hz;
}

s32 osAiSetNextBuffer(void* buf, u32 nbytes) {
    if (pm_au2_active && !pm_au2_idle()) { /* AU2: the render thread posts it after it rendered the frame (queue order). Idle render thread: post at once, no wake. */
        AuCmd c = { AUC_POST };
        c.v0 = (s32)(uintptr_t)buf;
        c.v1 = (s32)nbytes;
        pm_au2_emit(&c);
        pm_au2_commit();
        return 0;
    }
    pm_audio_post(buf, (unsigned)nbytes, 0);
    return 0;
}

u32 osAiGetLength(void) { return pm_audio_pending(); }
u32 osAiGetStatus(void) { return 0; }

/* N64 flash save chip (fio.c). OS-level shim: the 128 KB chip is an array backed by save.flash next to the ROM on the SD card
 * (page = 128 bytes, sector = 128 pages = 16 KB, erased bytes are 0xFF). No game logic. Debug switch `freshsave` starts
 * with an erased chip and overwrites the file. Every call completes its message queue like the OS would. */
#define FLASH_PAGE 128u
#define FLASH_SECTOR (128u * FLASH_PAGE)
#define FLASH_SIZE (8u * FLASH_SECTOR)
static u8 flash_mem[FLASH_SIZE];
static int flash_ready;
static const void* flash_wr_src;

/* HW2: the SD file work is in pm_flash.c (thread on core 1). */
static void flash_sync(u32 off, u32 len) { pm_flash_mark(off, len); }

static void flash_selftest(void);
s32 osFlashWriteArray(u32 pageNum);
s32 osFlashSectorErase(u32 pageNum);

static void flash_init(void) {
    FILE* f;
    if (flash_ready) return;
    flash_ready = 1;
    pm_flash_attach(flash_mem, sizeof(flash_mem));
    memset(flash_mem, 0xFF, sizeof(flash_mem));
    f = pm_debug_has("freshsave") ? NULL : fopen("save.flash", "rb");
    if (f) {
        size_t n = fread(flash_mem, 1, sizeof(flash_mem), f);
        fclose(f);
        pm_log("save.flash loaded %u bytes", (unsigned)n);
        if (pm_debug_has("flashselftest")) { u32 h = 0, k; for (k = 0; k < FLASH_SIZE; k++) h = h * 31u + flash_mem[k]; pm_log("flash selftest: loaded hash %08x", h); }
    } else {
        flash_sync(0, FLASH_SIZE);
        pm_flash_flush("new file");
        pm_log("save.flash new");
    }
    if (pm_debug_has("flashselftest")) flash_selftest();
}

/* Test switch flashselftest: one save pattern through the real osFlash calls (erase sector 3, 39 pages, erase sector 0, one
 * page), then flashfake joins at once, like a HOME press during the 70 ms wait. 3ds/tools/flash_selftest.py checks save.flash. */
static void flash_selftest(void) {
    u8 pg[FLASH_PAGE];
    u32 i, p, crc = 0;
    osFlashSectorErase(3 * 128);
    for (p = 0; p < 39; p++) {
        for (i = 0; i < FLASH_PAGE; i++) pg[i] = (u8)(p * 7 + i);
        flash_wr_src = pg;
        osFlashWriteArray(3 * 128 + p);
    }
    osFlashSectorErase(0);
    for (i = 0; i < FLASH_PAGE; i++) pg[i] = (u8)(0xA5 + i);
    flash_wr_src = pg;
    osFlashWriteArray(0);
    if (pm_debug_has("flashfake")) pm_flash_flush("fake suspend");
    for (i = 0; i < FLASH_SIZE; i++) crc = crc * 31u + flash_mem[i];
    pm_log("flash selftest: ram hash %08x", crc);
}

void osFlashReadArray(OSIoMesg* mb, s32 pri, u32 pageNum, void* dramAddr, u32 pageCount, OSMesgQueue* mq) {
    (void)pri;
    flash_init();
    if ((pageNum + pageCount) * FLASH_PAGE <= FLASH_SIZE) memcpy(dramAddr, flash_mem + pageNum * FLASH_PAGE, pageCount * FLASH_PAGE);
    else memset(dramAddr, 0xFF, pageCount * FLASH_PAGE);
    if (mq) osSendMesg(mq, (OSMesg)mb, OS_MESG_NOBLOCK);
}
void osFlashWriteBuffer(OSIoMesg* mb, s32 pri, void* dramAddr, OSMesgQueue* mq) {
    (void)pri;
    flash_wr_src = dramAddr;
    if (mq) osSendMesg(mq, (OSMesg)mb, OS_MESG_NOBLOCK);
}
s32 osFlashWriteArray(u32 pageNum) {
    flash_init();
    if ((pageNum + 1) * FLASH_PAGE > FLASH_SIZE || !flash_wr_src) return -1;
    memcpy(flash_mem + pageNum * FLASH_PAGE, flash_wr_src, FLASH_PAGE);
    flash_sync(pageNum * FLASH_PAGE, FLASH_PAGE);
    return 0;
}
s32 osFlashSectorErase(u32 pageNum) {
    u32 off = pageNum / 128u * FLASH_SECTOR;
    flash_init();
    if (off + FLASH_SECTOR > FLASH_SIZE) return -1;
    memset(flash_mem + off, 0xFF, FLASH_SECTOR);
    flash_sync(off, FLASH_SECTOR);
    return 0;
}
void osInvalDCache(void* addr, s32 nbytes) { (void)addr; (void)nbytes; }
void osWritebackDCache(void* addr, s32 nbytes) { (void)addr; (void)nbytes; }
OSPiHandle* osFlashInit(void) { flash_init(); return NULL; }
