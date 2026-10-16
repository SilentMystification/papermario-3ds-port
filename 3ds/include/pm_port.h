#ifndef PM_PORT_H
#define PM_PORT_H

/* Plain C seam. Game files include ultra64; GPU files include 3ds.h.
 * Neither side includes the other. */

void pm_log(const char* fmt, ...);
void pm_log_init(int live);
void pm_log_poll(void);
void pm_log_set_switches(const char* s);
void pm_crash_line(const char* s, int n);
void pm_frame_loop(void);

int pm_debug_has(const char* word);
unsigned pm_ticks(void);

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

#endif
