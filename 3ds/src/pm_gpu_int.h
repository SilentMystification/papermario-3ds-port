/* pm_gpu_int.h: state that pm_gpu.c and pm_gpu_ui.c share (C8: the bottom screen text moved out of pm_gpu.c, no logic change). Not a public interface. */
#pragma once
#include <3ds.h>
/* Bottom screen rows, 1-based as the cursor and ANSI count them (row 1 is the top). The HUD (pm_gpu_hud_flush) has fixed rows:
 * 1 fps, 2 memory, 3 VRAM, 4 to 6 the map block (the old and new map during the 2 s change, else the current map).
 * Then, when on: the Mario row (poshud), the stats row (stats). Then the separator, the log header (console and SD level),
 * the log window (LOG_TOP to 26, it scrolls only here), separator 27, footer 28 to 30 (map, system).
 * hud_x = the rows added by poshud and stats: the separator is row 7+hud_x, the header 8+hud_x, the log window 9+hud_x to 26. */
#define HUD_X (pos_on + stats_on)
#define LOG_TOP (9 + HUD_X)
#define LOG_ROWS (18 - HUD_X)
#define LAY_KEY (2 * pos_on + stats_on)
extern const char* const pm_lvl_name[5]; /* pm_options.c: Error, Warn, Info, Debug, Off */
extern volatile int pos_on, stats_on;
extern int frame_key, frame_lay;
extern volatile int hud_new, foot_new;
extern char foot_row[2][40];
extern PrintConsole bottom_con;
extern int menu_on, menu_dirty; /* pm_gpu_ui.c: the menu text is on the bottom screen; a draw since the last present (pm_gpu_menu_present) */
