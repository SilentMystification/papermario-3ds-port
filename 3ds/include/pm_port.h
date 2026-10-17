#ifndef PM_PORT_H
#define PM_PORT_H

/* Plain C seam. Game files include ultra64; GPU files include 3ds.h.
 * Neither side includes the other. */

void pm_log(const char* fmt, ...);
void pm_log_init(int live);
void pm_log_poll(void);
void pm_con_lock(void);
int pm_con_trylock(void);
void pm_con_unlock(void);
void pm_log_start(void);
void pm_log_stop(void);
int pm_log_writer_core(void);
unsigned pm_log_writer_hb(void);
unsigned pm_log_dropped(void);
void pm_log_stat(float avg_ms, unsigned frames, float secs);
void pm_log_top(void);
void pm_log_set_switches(const char* s);
void pm_crash_line(const char* s, int n);
void pm_fatal_stop(const char* msg); /* R01: log FATAL, flush, [CRASH] line, svcBreak */
void pm_frame_loop(void);

int pm_debug_has(const char* word);
const char* pm_debug_text(void);
int pm_debug_value(const char* key, char* out, unsigned n);
unsigned pm_ticks(void);

/* PT01: per-frame phase timer. Switch `phase` turns it on (pm_ph_on). Exclusive time: a nested phase takes its time from
 * the phase that was running. Counters are per drawn frame. All macros are one load and one branch when the switch is off. */
enum { PH_OTHER, PH_LOGIC, PH_DL, PH_GBI, PH_GPU, PH_TEX, PH_AUDIO, PH_ROM, PH_YAY0, PH_FBEGIN, PH_FEND, PH_IDLE, PH_GCACHE, PH_GDRAW, PH_GVTX, PH_GTRI, PH_GMTX, PH_GOP, PH_FBREAD, PH_N };
enum { C_TRI, C_TRIOUT, C_VTXLOAD, C_VTXSLOT, C_DRAW, C_FLUSH, C_TEXCALL, C_TEXMISS, C_TEXBYTE, C_ROMN, C_ROMB, C_YAYN, C_YAYB, C_DBG, C_ATICK, C_CMD, C_N };
extern int pm_ph_on;
extern int pm_ph_fine; /* switch phasegbi: time G_VTX, triangle and G_MTX ops on their own (two tick reads per op, so it inflates the frame) */
extern unsigned pm_ph_cnt[C_N];
void pm_ph_init(void);
void pm_ph_push(int id);
void pm_ph_pop(void);
void pm_ph_frame_end(void);
#define PH_PUSH(id) do { if (pm_ph_on) pm_ph_push(id); } while (0)
#define PH_POP() do { if (pm_ph_on) pm_ph_pop(); } while (0)
#define PH_ADD(c, n) do { if (pm_ph_on) pm_ph_cnt[c] += (unsigned)(n); } while (0)

void pm_pad_scan(void);
void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger);

void pm_game_bind(void);
int pm_rom_init(void);
void pm_fatal_screen(const char* msg);
void pm_crash_selftest(void);
void pm_crash_install(void* stack, unsigned size);
void pm_stack_register(int id, const char* name, unsigned size);
unsigned pm_stack_used(int id, unsigned* size);
void pm_fpscr_setup(const char* who);
int pm_mem_crash_line(char* buf, int cap);
void pm_gpu_log_stats(const char* tag);
void* pm_soc_buffer(unsigned* size);
unsigned pm_heap_bytes(void);
unsigned pm_linear_bytes(void);
void pm_mem_log(const char* tag);
void pm_gfx_retrace(void);
void pm_audio_init(void);
void pm_audio_post(const void* pcm, unsigned bytes, unsigned rate);
void pm_au_thread_start(void (*entry)(void*), void* arg);
int pm_au_thread_started(void);
int pm_au_thread_yield(void);
void pm_au_thread_resume(void);
unsigned pm_audio_retraces(void);
unsigned pm_audio_pending(void);
void pm_audio_tick(void);

/* MS01: static data of a map is restored when the map is entered again (pm_mapdata.c, map_restore_table.c is generated). */
typedef struct PmMapRst {
    const char* id;          /* map id, or machi / dgb for the files shared by the maps of an area */
    unsigned char *d0, *d1;  /* block of the .data symbols that change at run time (pristine copy in save) */
    unsigned char *b0, *b1;  /* block of all .bss of the map (zeroed) */
    unsigned char* save;
} PmMapRst;
void pm_mapdata_enter(const char* id);

#endif
