#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "pm_port.h"
#include "pm_romload.h"

/* BOOT: the ROM image is streamed from the SD card while the game starts. pm_rom_init checks the header, allocates the image and
 * calls pm_rom_stream_start; the game starts at once. A loader thread (core 0, priority 0x2F) reads the image in 128 KB chunks of
 * the original ROM address space, in address order. Each reader of the image (pi_read_raw, pm_rom_ptr) calls rs_need first: one
 * acquire load of pm_rs_all once the stream is done; before that, pm_rs_need_slow reads a missing chunk on the calling thread
 * (demand) or waits for the loader to finish the chunk it is reading.
 * Chunk state rs_st[c]: 0 absent, 1 loading (one thread owns it, by CAS 0 -> 1), 2 ready, 3 failed. The owner stores 2 or 3 with
 * release order after its read, so a reader that sees 2 (acquire) sees the bytes.
 * SD access while asleep or in HOME (PT68): a reader counts itself in rs_inflight, then checks pm_io_quiet (both seq_cst). io_gate(1)
 * sets pm_io_quiet, then waits for rs_inflight == 0 (pm_rom_stream_quiet). Either the reader sees the gate and waits, or the gate
 * sees the reader and waits for its read to end.
 * hw_guidelines: the fast path is one load; the state table is 320 bytes, the static total below 1 KB; no division; cold file (-Os). */

#define RS_SHIFT 17u
#define RS_CHUNK (1u << RS_SHIFT)
#define RS_N (0x02800000u >> RS_SHIFT) /* PM_ROM_FULL: 320 chunks */
#define RS_PIECE 0x8000u /* 32 KB per loader read (about 6 ms at 5 MB/s) */
#define RS_GUARD 256u /* bytes past each access that must be ready too (prefetch and look-ahead reads of the decoders) */

volatile int pm_rs_all; /* 1: every chunk is ready (or the image was never streamed) */
static volatile u8 rs_st[RS_N];
static volatile int rs_err; /* a chunk read failed */
static volatile int rs_inflight; /* readers inside an SD read */
static volatile int rs_stop; /* exit: the loader ends after its current chunk */
static Handle rs_fh;
static u8* rs_block;
static const PmRsSeg* rs_keep;
static unsigned rs_nkeep;
static u32 rs_size;
static Thread rs_th;
static u64 rs_t0, rs_tdone;
static volatile u32 rs_demand, rs_waits, rs_direct;
static volatile u32 rs_wait_ms;
static u32 rs_bytes;
static Thread rs_game; /* the thread that started the stream: the game thread, which may show the fatal screen */

extern volatile int pm_io_quiet; /* pm_log.c: the app is suspended or asleep */

static int rs_io_enter(void) {
    for (;;) {
        __atomic_add_fetch(&rs_inflight, 1, __ATOMIC_SEQ_CST);
        if (!__atomic_load_n(&pm_io_quiet, __ATOMIC_SEQ_CST)) return 1;
        __atomic_sub_fetch(&rs_inflight, 1, __ATOMIC_SEQ_CST);
        if (rs_stop) return 0;
        svcSleepThread(50000000LL); /* as the flash thread (pm_flash.c): no SD read while the gate is closed */
    }
}
static void rs_io_leave(void) { __atomic_sub_fetch(&rs_inflight, 1, __ATOMIC_SEQ_CST); }

/* Read the kept bytes of chunk c into their packed places. Returns the bytes read, -1 on a failed read, -2 at exit (rs_stop). */
static int rs_read_chunk(unsigned c) {
    u32 lo = c << RS_SHIFT, hi = lo + RS_CHUNK, got = 0;
    if (hi > rs_size) hi = rs_size;
    for (unsigned i = 0; i < rs_nkeep; i++) {
        u32 s = rs_keep[i].start, e = s + rs_keep[i].len;
        if (e <= lo) continue;
        if (s >= hi) break; /* the table is sorted by start */
        u32 a = s > lo ? s : lo, b = e < hi ? e : hi;
        while (a < b) { /* RS_PIECE reads: a direct read of the game waits behind one piece at most */
            u32 len = b - a < RS_PIECE ? b - a : RS_PIECE, n = 0;
            if (!rs_io_enter()) return -2;
            Result r = FSFILE_Read(rs_fh, &n, a, rs_block + rs_keep[i].packed_off + (a - s), len);
            rs_io_leave();
            if (R_FAILED(r) || n != len) return -1;
            got += n;
            a += len;
        }
    }
    return (int)got;
}

/* Claim chunk c (0 -> 1) and read it. Returns 1 when this thread read it (ready or failed), 0 when another thread owns it. */
static int rs_claim(unsigned c) {
    u8 want = 0;
    if (!__atomic_compare_exchange_n(&rs_st[c], &want, 1, 0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) return 0;
    int n = rs_read_chunk(c);
    if (n < 0) {
        if (n == -1) rs_err = 1; /* -2: the process ends, no fatal screen */
        __atomic_store_n(&rs_st[c], 3, __ATOMIC_RELEASE);
    } else {
        __atomic_add_fetch(&rs_bytes, (u32)n, __ATOMIC_RELAXED);
        __atomic_store_n(&rs_st[c], 2, __ATOMIC_RELEASE);
    }
    return 1;
}

static void rs_loader(void* arg) {
    (void)arg;
    for (unsigned c = 0; c < RS_N && !rs_stop; c++) rs_claim(c);
    /* A chunk a demand reader still owns is not done: wait for it before the stream is declared done. */
    for (unsigned c = 0; c < RS_N && !rs_stop; c++)
        while (__atomic_load_n(&rs_st[c], __ATOMIC_ACQUIRE) == 1 && !rs_stop) svcSleepThread(1000000LL);
    if (rs_stop) return;
    rs_tdone = svcGetSystemTick();
    if (!rs_err) __atomic_store_n(&pm_rs_all, 1, __ATOMIC_RELEASE);
    unsigned ms = (unsigned)((rs_tdone - rs_t0) / (SYSCLOCK_ARM11 / 1000ull));
    unsigned kbps = ms ? (unsigned)((u64)rs_bytes * 1000u / 1024u / ms) : 0u; /* once per boot */
    pm_log_w("rom stream done ms %u MB/s %u.%u demand %u waits %u wait_ms %u direct %u%s", ms, kbps >> 10, ((kbps & 1023u) * 10u) >> 10,
             rs_demand, rs_waits, rs_wait_ms, rs_direct, rs_err ? " FAILED" : "");
}

static __attribute__((noinline)) void rs_fail_here(void) {
    pm_fatal_screen("Cannot read the ROM. Check the SD card.");
    pm_exit_process();
}

void pm_rs_need_slow(unsigned addr, unsigned len) {
    u32 end = addr + len + RS_GUARD;
    if (end > rs_size || end < addr) end = rs_size;
    if (addr >= end) return;
    unsigned c0 = addr >> RS_SHIFT, c1 = (end - 1u) >> RS_SHIFT;
    u64 t0 = 0;
    int blocked = 0;
    for (unsigned c = c0; c <= c1; c++) {
        for (;;) {
            u8 st = __atomic_load_n(&rs_st[c], __ATOMIC_ACQUIRE);
            if (st == 2) break;
            if (!blocked) { blocked = 1; t0 = svcGetSystemTick(); }
            if (st == 0) {
                if (rs_claim(c)) __atomic_add_fetch(&rs_demand, 1, __ATOMIC_RELAXED);
                continue;
            }
            if (st == 3) {
                if (threadGetCurrent() == rs_game) rs_fail_here();
                /* another thread: hold here; the game thread shows the fatal screen at its next frame (pm_rom_stream_poll) */
                svcSleepThread(50000000LL);
                continue;
            }
            svcSleepThread(1000000LL); /* the owner reads it (about 25 ms for 128 KB at 5 MB/s) */
        }
    }
    if (blocked) {
        __atomic_add_fetch(&rs_waits, 1, __ATOMIC_RELAXED);
        u32 dms = (u32)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000ull)); /* cold: once per blocked access */
        __atomic_add_fetch(&rs_wait_ms, dms, __ATOMIC_RELAXED);
    }
}

/* A copy out of the image (pi_read_raw) whose chunks are not all ready: read the n bytes at ROM address addr straight from the
 * file into out. The boot reads small parts (bank headers and tables) of many chunks: a whole-chunk read would cost 128 KB each.
 * Returns 1 when out holds the bytes, 0 when the image has them (or the direct read failed: the chunk path reports it). */
int pm_rs_read_direct(unsigned addr, void* out, unsigned n) {
    unsigned c0 = addr >> RS_SHIFT, c1 = (addr + n - 1u) >> RS_SHIFT, c;
    if (!n || addr + n > rs_size) return 0;
    for (c = c0; c <= c1; c++)
        if (__atomic_load_n(&rs_st[c], __ATOMIC_ACQUIRE) != 2) break;
    if (c > c1) return 0;
    u64 t0 = svcGetSystemTick();
    u32 got = 0;
    if (!rs_io_enter()) return 0;
    Result r = FSFILE_Read(rs_fh, &got, addr, out, n);
    rs_io_leave();
    if (R_FAILED(r) || got != n) return 0;
    __atomic_add_fetch(&rs_direct, 1, __ATOMIC_RELAXED);
    __atomic_add_fetch(&rs_waits, 1, __ATOMIC_RELAXED);
    __atomic_add_fetch(&rs_wait_ms, (u32)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000ull)), __ATOMIC_RELAXED);
    return 1;
}

int pm_rom_stream_start(const char* path, unsigned char* block, unsigned size, const PmRsSeg* keep, unsigned nkeep) {
    static PmRsSeg whole;
    char p[64];
    snprintf(p, sizeof p, "/3ds/PaperMario/%s", path);
    Result r = FSUSER_OpenFileDirectly(&rs_fh, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, p), FS_OPEN_READ, 0);
    if (R_FAILED(r)) {
        pm_log_e("rom stream: open %s failed %08lx", p, (unsigned long)r);
        return 0;
    }
    if (!keep) { /* the full image (switch romfull): one segment at its own offset */
        whole.start = 0; whole.len = size; whole.packed_off = 0;
        keep = &whole; nkeep = 1;
    }
    rs_block = block; rs_size = size; rs_keep = keep; rs_nkeep = nkeep;
    rs_game = threadGetCurrent();
    rs_t0 = svcGetSystemTick();
    rs_th = threadCreate(rs_loader, NULL, 8 * 1024, 0x2F, 0, false);
    if (!rs_th) { /* no thread: the game reads every chunk on demand; nothing else changes */
        pm_log_e("rom stream: no loader thread, demand reads only");
    }
    return 1;
}

void pm_rom_stream_poll_slow(void) {
    if (!rs_err) return;
    rs_fail_here();
}

int pm_rom_stream_quiet(unsigned ms) {
    u64 t0 = svcGetSystemTick();
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    for (unsigned i = 0; __atomic_load_n(&rs_inflight, __ATOMIC_SEQ_CST); i++) {
        if (i >= ms) return -1;
        svcSleepThread(1000000LL);
    }
    return (int)((svcGetSystemTick() - t0) / (SYSCLOCK_ARM11 / 1000ull));
}

void pm_rom_stream_stop(void) {
    if (!rs_th) return;
    rs_stop = 1;
    if (R_SUCCEEDED(threadJoin(rs_th, 1000000000ULL))) {
        threadFree(rs_th);
        rs_th = NULL;
        FSFILE_Close(rs_fh);
    }
}

u64 pm_rom_stream_info(unsigned* waits, unsigned* wait_ms, unsigned* demand) {
    *waits = rs_waits;
    *wait_ms = rs_wait_ms;
    *demand = rs_demand;
    return __atomic_load_n(&pm_rs_all, __ATOMIC_ACQUIRE) ? rs_tdone : 0;
}
