#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ultra64.h"
#include "nu/nusys.h"
#include "enums.h"
#include "pm_port.h"
#include "pm_tex.h"
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

u32 osGetCount(void) { return (u32)pm_ticks(); }

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
    s32 i = (mq->first + mq->validCount) % mq->msgCount;
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
    mq->first = (mq->first + 1) % mq->msgCount;
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

/* One retrace. Wakes the audio manager, then plays the part of the RSP and the scheduler: each audio task it
 * submits is finished at once (the work already ran in pm_rsp_audio.c) and its reply message is sent. */
void pm_audio_tick(void) {
    static unsigned ticks, worst;
    static unsigned long long sum;
    unsigned t0;
    if (!au_client || !pm_au_thread_started()) return;
    t0 = pm_ticks();
    if (!nusched.audioRequestMQ.msgCount) {
        osCreateMesgQueue(&nusched.audioRequestMQ, nusched.audioRequestBuf, NU_SC_MAX_MESGS);
        nusched.retraceMsg = NU_SC_RETRACE_MSG;
    }
    osSendMesg(au_client->msgQ, (OSMesg)&nusched.retraceMsg, OS_MESG_NOBLOCK);
    for (int i = 0; i < 8; i++) {
        int any = 0;
        OSMesg m;
        pm_au_thread_resume();
        while (osRecvMesg(&nusched.audioRequestMQ, &m, OS_MESG_NOBLOCK) == 0) {
            NUScTask* t = (NUScTask*)m;
            osSendMesg(t->msgQ, t->msg, OS_MESG_NOBLOCK);
            any = 1;
        }
        if (!any) break;
    }
    {
        /* Frame cost of the audio engine. 268111856 ticks per second on the ARM11. */
        unsigned dt = pm_ticks() - t0;
        sum += dt;
        if (dt > worst) worst = dt;
        /* L01: every 2 s with the audiostat switch, else every 20 s (S28 reads this line on hardware) */
        if (++ticks == (pm_debug_has("audiostat") ? 120u : 1200u)) {
            pm_log_d("audio cost avg %u us max %u us per 60 Hz tick", (unsigned)((unsigned long long)sum * 1000 / ticks / 268112), (unsigned)((unsigned long long)worst * 1000 / 268112));
            ticks = worst = 0;
            sum = 0;
        }
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
    /* Heap is 44MB. Leave 4MB of it for the game. */
    unsigned leave = 4u * 1024u * 1024u;
    unsigned heap = pm_heap_bytes();
    unsigned cap = heap > leave ? heap - leave : 0;
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
            pm_log_d("rom resident %uKB of %uKB%s", need >> 10, rom_size >> 10, can_pack ? " (packed)" : "");
            pm_log_d("ram n64 4MB, rom %uMB, port %uMB", (need + 0xFFFFF) >> 20, pm_linear_bytes() >> 20);
            return 1;
        }
        free(block);
    }
    rewind(f);
    rom = f;
    pm_log_d("rom streaming (%ld bytes, cap %uKB)", sz, cap >> 10);
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
    PH_ADD(C_ROMN, 1);
    PH_ADD(C_ROMB, size);
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
            if (rom_remap) {
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
            if (rom && fseek(rom, (long)addr, SEEK_SET) == 0) got = fread(out, 1, n, rom);
            if (got < n) memset(out + got, 0, n - got);
        }
        out += n; addr += n; left -= n;
    }
    pm_tex_invalidate(buf, size);
    PH_POP();
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
    if (pm_debug_has("romtrace")) {
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

static void flash_sync(u32 off, u32 len) {
    static unsigned fails;
    FILE* f = fopen("save.flash", "r+b");
    if (!f) f = fopen("save.flash", "w+b");
    int ok = f != NULL && fseek(f, (long)off, SEEK_SET) == 0 && fwrite(flash_mem + off, 1, len, f) == len;
    /* fclose flushes: a full SD card shows up here. The save stays in RAM, the game goes on. */
    if (f && fclose(f) != 0) ok = 0;
    if (!ok && fails++ < 5) pm_log_e("save.flash write FAILED (off %u len %u, SD full or missing?), save kept in RAM only", off, len);
}

static void flash_init(void) {
    FILE* f;
    if (flash_ready) return;
    flash_ready = 1;
    memset(flash_mem, 0xFF, sizeof(flash_mem));
    f = pm_debug_has("freshsave") ? NULL : fopen("save.flash", "rb");
    if (f) {
        size_t n = fread(flash_mem, 1, sizeof(flash_mem), f);
        fclose(f);
        pm_log("save.flash loaded %u bytes", (unsigned)n);
    } else {
        flash_sync(0, FLASH_SIZE);
        pm_log("save.flash new");
    }
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
