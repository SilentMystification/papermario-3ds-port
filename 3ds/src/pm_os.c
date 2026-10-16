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
static u16 fb_mem[320 * 240];
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
    return cur_fb ? cur_fb : fb_mem;
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
    static unsigned ticks, sum, worst;
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
        if (++ticks == 120) {
            pm_log("audio cost avg %u us max %u us per 60 Hz tick", (unsigned)((unsigned long long)sum * 1000 / 120 / 268112), (unsigned)((unsigned long long)worst * 1000 / 268112));
            ticks = sum = worst = 0;
        }
    }
}

void pm_rom_init(void) {
    if (rom_tried) return;
    rom_tried = 1;
    FILE* f = fopen("papermario.z64", "rb");
    if (!f) f = fopen("baserom.us.z64", "rb");
    if (!f) {
        pm_log("rom missing (sdmc:/3ds/PaperMario/papermario.z64)");
        pm_log("ram n64 4MB, port %uMB, rom not loaded", pm_linear_bytes() >> 20);
        return;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        pm_log("rom size failed");
        return;
    }
    long sz = ftell(f);
    rewind(f);
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
            pm_log("rom resident %uKB of %uKB%s", need >> 10, rom_size >> 10, can_pack ? " (packed)" : "");
            pm_log("ram n64 4MB, rom %uMB, port %uMB", (need + 0xFFFFF) >> 20, pm_linear_bytes() >> 20);
            return;
        }
        free(block);
    }
    rewind(f);
    rom = f;
    pm_log("rom streaming (%ld bytes, cap %uKB)", sz, cap >> 10);
    pm_log("ram n64 4MB, port %uMB", pm_linear_bytes() >> 20);
}

static void pi_read_raw(u32 rom_addr, void* buf, u32 size) {
    if (!buf || !size) return;
    if ((unsigned)buf < 0x00100000u) {
        static int once;
        if (!once) {
            once = 1;
            pm_log("dma skip rom %08x buf %08x n %u", rom_addr, (unsigned)buf, size);
        }
        return;
    }
    if (!rom_tried) pm_rom_init();
    /* Walk the request over the original offsets. A byte in a dropped range is a hard error. */
    u8* out = (u8*)buf;
    u32 left = size, addr = rom_addr;
    while (left) {
        u32 n = left;
        if (addr >= rom_size) {
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
        if (dropped) {
            static unsigned hits;
            if (hits++ < 100) pm_log("ROM READ IN DROPPED RANGE %08x n %u (req %08x n %u)", addr, n, rom_addr, size);
        }
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

/* N64 flash save chip (fio.c). The 3DS port has no flash. Reads return erased flash (0xFF), so no save is found.
 * Writes and erases do nothing and report success. Every call completes its message queue like the OS would. */
void osFlashReadArray(OSIoMesg* mb, s32 pri, u32 pageNum, void* dramAddr, u32 pageCount, OSMesgQueue* mq) {
    (void)pri; (void)pageNum;
    memset(dramAddr, 0xFF, pageCount * 128);
    if (mq) osSendMesg(mq, (OSMesg)mb, OS_MESG_NOBLOCK);
}
void osFlashWriteBuffer(OSIoMesg* mb, s32 pri, void* dramAddr, OSMesgQueue* mq) {
    (void)pri; (void)dramAddr;
    if (mq) osSendMesg(mq, (OSMesg)mb, OS_MESG_NOBLOCK);
}
s32 osFlashWriteArray(u32 pageNum) { (void)pageNum; return 0; }
s32 osFlashSectorErase(u32 pageNum) { (void)pageNum; return 0; }
void osInvalDCache(void* addr, s32 nbytes) { (void)addr; (void)nbytes; }
void osWritebackDCache(void* addr, s32 nbytes) { (void)addr; (void)nbytes; }
OSPiHandle* osFlashInit(void) { return NULL; }
