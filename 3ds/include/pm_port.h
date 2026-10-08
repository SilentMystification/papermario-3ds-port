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
void pm_gfx_retrace(void);
void pm_audio_init(void);
void pm_audio_post(const void* pcm, unsigned bytes, unsigned rate);

#endif
