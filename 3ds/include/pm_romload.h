#ifndef PM_ROMLOAD_H
#define PM_ROMLOAD_H
/* BOOT: the ROM image streamed from the SD card (pm_romload.c). */
typedef struct PmRsSeg { unsigned start, len, packed_off; } PmRsSeg; /* same layout as PmRomSeg (rom_remap.h) */
extern volatile int pm_rs_all; /* 1: the whole image is in RAM */
void pm_rs_need_slow(unsigned addr, unsigned len);
/* Before a read of len bytes at ROM address addr from the image: one acquire load once the stream is done. */
static inline void pm_rs_need(unsigned addr, unsigned len) {
    if (__builtin_expect(__atomic_load_n(&pm_rs_all, __ATOMIC_ACQUIRE), 1)) return;
    pm_rs_need_slow(addr, len);
}
int pm_rs_read_direct(unsigned addr, void* out, unsigned n);
unsigned pm_rom_direct_n(void);
int pm_rom_read_exact(unsigned addr, void* out, unsigned n); /* BOOT2 fix a: exact bytes while the stream runs (pm_os.c), 1 = done */
void pm_rs_auread(unsigned addr, unsigned n); /* BOOT2 auread: au_read_rom reads before the stream is done */
void pm_rs_auread_get(unsigned* n, unsigned* bytes, unsigned* max, unsigned* chunks); /* BOOT2: direct reads so far */ /* 1: out was read from the file (chunks not ready yet) */
/* Opens sdmc:/3ds/PaperMario/<path> and starts the loader thread. keep NULL: the full image at its own offsets. 0: open failed. */
int pm_rom_stream_start(const char* path, unsigned char* block, unsigned size, const PmRsSeg* keep, unsigned nkeep);
void pm_rom_stream_poll_slow(void);
/* Game thread, top of each frame: shows the fatal screen when a chunk read failed. One load once the stream is done. */
static inline void pm_rom_stream_poll(void) {
    if (__builtin_expect(!__atomic_load_n(&pm_rs_all, __ATOMIC_RELAXED), 0)) pm_rom_stream_poll_slow();
}
int pm_rom_stream_quiet(unsigned ms); /* io_gate(1): wait until no stream read is in flight; ms waited or -1 */
void pm_rom_stream_stop(void); /* exit: the loader ends, the file is closed */
/* Done tick (0: not done) and the counters of the readers that blocked. */
unsigned long long pm_rom_stream_info(unsigned* waits, unsigned* wait_ms, unsigned* demand);
#endif
