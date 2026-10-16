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
void pm_rom_init(void);
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
