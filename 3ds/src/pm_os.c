#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "ultra64.h"
#include "nu/nusys.h"
#include "enums.h"
#include "pm_port.h"
#include "pm_tex.h"

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
    /* Heap is 44MB. Leave 4MB of it for the game. */
    unsigned leave = 4u * 1024u * 1024u;
    unsigned heap = pm_heap_bytes();
    unsigned cap = heap > leave ? heap - leave : 0;
    if (sz > 0 && (unsigned)sz <= cap) {
        u8* block = (u8*)memalign(16, (size_t)sz);
        if (block && fread(block, 1, (size_t)sz, f) == (size_t)sz) {
            rom_bytes = block;
            rom_size = (u32)sz;
            fclose(f);
            pm_log("rom resident %uKB", rom_size >> 10);
            pm_log("ram n64 4MB, rom %uMB, port %uMB", (rom_size + 0xFFFFF) >> 20, pm_linear_bytes() >> 20);
            return;
        }
        free(block);
    }
    rewind(f);
    rom = f;
    pm_log("rom streaming (%ld bytes, cap %uKB)", sz, cap >> 10);
    pm_log("ram n64 4MB, port %uMB", pm_linear_bytes() >> 20);
}

void nuPiReadRom(u32 rom_addr, void* buf, u32 size) {
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
    if (rom_bytes) {
        if (rom_addr >= rom_size) {
            memset(buf, 0, size);
            pm_tex_invalidate(buf, size);
            return;
        }
        u32 n = size;
        if (rom_addr + n > rom_size) n = rom_size - rom_addr;
        memcpy(buf, rom_bytes + rom_addr, n);
        if (n < size) memset((u8*)buf + n, 0, size - n);
        pm_tex_invalidate(buf, size);
        return;
    }
    /* Byte stream, not word-swapped. A display list copied out of the ROM is still
     * big-endian until the loader that knows the type swaps it once. */
    if (!rom || fseek(rom, (long)rom_addr, SEEK_SET) != 0) {
        memset(buf, 0, size);
        pm_tex_invalidate(buf, size);
        return;
    }
    size_t n = fread(buf, 1, size, rom);
    if (n < size) memset((u8*)buf + n, 0, size - n);
    pm_tex_invalidate(buf, size);
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
