/* F11-6: the port menu. Select, when no game code takes the press (file select takes it for its own Options panel), opens a text menu on the
 * bottom screen in any game mode. While it is open pm_pad_scan gives the game no button and no stick and the log writer prints nothing on
 * the bottom screen. It reads and writes the same settings as the Options panel (pm_options_get, pm_options_flip) and holds the F11-5 controls:
 * one row per 3DS input, A gives it the next N64 button. Port layer only, no game logic. */
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "pm_port.h"

volatile int pm_menu_open;

#define N_OPT 9
#define N_ROWS (N_OPT + PM_NIN + 1)
#define VIS 24
static int sel, top;

static const char* const opt_lbl[N_OPT] = { "60 FPS", "Widescreen", "HUD", "Stats", "No fog", "No light", "Logging", "Log level", "Fixed seed" };
static const char* const lvl_name[4] = { "Error", "Warn", "Info", "Debug" };

static void draw(void) {
    char out[1400];
    int n = snprintf(out, sizeof out, "SETTINGS  dpad: move  A: change  B/Select: close\n\n");
    if (sel < top) top = sel;
    if (sel >= top + VIS) top = sel - VIS + 1;
    for (int r = top; r < top + VIS && r < N_ROWS; r++) {
        char line[48];
        if (r < N_OPT) {
            int v = pm_options_get(r);
            snprintf(line, sizeof line, "%-12s %s", opt_lbl[r], r == 7 ? lvl_name[v & 3] : v ? "On" : "Off");
        } else if (r < N_OPT + PM_NIN) {
            int i = r - N_OPT;
            snprintf(line, sizeof line, "Ctl %-7s %s", pm_in_name[i], pm_n64_name(pm_map[i]));
        } else snprintf(line, sizeof line, "Reset controls");
        n += snprintf(out + n, sizeof out - (size_t)n, "%c %s\n", r == sel ? '>' : ' ', line);
    }
    pm_gpu_menu_draw(out);
}

void pm_menu_set(int on) {
    if (on == pm_menu_open) return;
    pm_menu_open = on;
    if (on) {
        sel = top = 0;
        pm_log("menu: open");
        draw();
    } else {
        pm_options_close(); /* saves options.txt when something changed */
        pm_gpu_menu_draw("");
        pm_log("menu: close");
    }
}

void pm_menu_key(unsigned k) {
    if (k & (KEY_B | KEY_SELECT)) { pm_menu_set(0); return; }
    if (k & KEY_DDOWN) sel = (sel + 1) % N_ROWS;
    else if (k & KEY_DUP) sel = (sel + N_ROWS - 1) % N_ROWS;
    else if (k & KEY_A) {
        if (sel < N_OPT) pm_options_flip(sel);
        else if (sel < N_OPT + PM_NIN) {
            int i = sel - N_OPT;
            pm_map[i] = pm_n64_next(pm_map[i]);
            pm_options_touch();
            pm_log("menu: map %s = %s", pm_in_name[i], pm_n64_name(pm_map[i]));
        } else {
            pm_map_reset();
            pm_options_touch();
            pm_log("menu: controls reset");
        }
    } else return;
    draw();
}
