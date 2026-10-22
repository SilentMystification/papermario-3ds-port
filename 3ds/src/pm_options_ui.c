/* F11-6, PT59: the port menu. Select, when no game code takes the press (file select takes it for its own Options panel), opens a text menu on
 * the bottom screen in any game mode. While it is open pm_pad_scan gives the game no button and no stick. The menu is the mirror of the Options
 * panel of the file select (src/filemenu/filemenu_options.c): the tabs, the rows, the values and the actions come from the same table in
 * pm_options.c (pm_optui_*, mode 1 adds Return to Title and Exit to Homebrew). Port layer only, no game logic. */
#include <3ds.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "pm_port.h"

volatile int pm_menu_open;
volatile int pm_exit_request; /* PT28: set by the menu row Exit to Homebrew, read by the main loop (pm_main.c) */

#define OPT_TABS 4
#define VIS 21 /* PT69, PT76: 30 text rows, see draw */
#define MODE 1
static int tab, sel[OPT_TABS], top[OPT_TABS];
static PmRepeat rep_y, rep_x;
static const char* const tab_name[OPT_TABS] = { "General", "Controls", "Debug", "Boss" };

/* PT75, PT76, PT77: the 30 rows of the bottom screen, from the top: row 0 tab line, rows 1 to VIS the options, 2 empty rows, the controls line,
 * 3 empty rows, then the 2 row feedback area (rows 28 and 29). Every error of the menu goes through pm_menu_msg. */
static char fb[2][40];
void pm_menu_msg(const char* fmt, ...);
static void draw(void) {
    char out[2400];
    int n = 0, rows = pm_optui_rows(tab, MODE);
    if (sel[tab] < top[tab]) top[tab] = sel[tab];
    if (sel[tab] >= top[tab] + VIS) top[tab] = sel[tab] - VIS + 1;
    for (int t = 0; t < OPT_TABS; t++) n += snprintf(out + n, sizeof out - (size_t)n, t == tab ? "[%s] " : " %s  ", tab_name[t]);
    n += snprintf(out + n, sizeof out - (size_t)n, "\n");
    for (int r = top[tab]; r < top[tab] + VIS && r < rows; r++) {
        const char* v = pm_optui_value(tab, r, MODE);
        n += snprintf(out + n, sizeof out - (size_t)n, "%c %-22s %s" "\n", r == sel[tab] ? '>' : ' ', pm_optui_label(tab, r, MODE), v ? v : "");
    }
    for (int r = (rows < VIS ? rows : VIS); r < VIS; r++) n += snprintf(out + n, sizeof out - (size_t)n, "\n");
    n += snprintf(out + n, sizeof out - (size_t)n, "\n\nL R: tab  < > A: change  B: close\n\n\n\n%s\n%s", fb[0], fb[1]);
    pm_log_d("menu: tab %d row %d %s = %s", tab, sel[tab], pm_optui_label(tab, sel[tab], MODE), pm_optui_value(tab, sel[tab], MODE) ? pm_optui_value(tab, sel[tab], MODE) : "-");
    pm_gpu_menu_draw(out);
}

/* PT76: the one function for a message to the user. Two rows of 39 characters at the bottom of the menu. A new message replaces the old one, a key press
 * clears it. Also written to the log as a warning. */
void pm_menu_msg(const char* fmt, ...) {
    char t[96];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    memset(fb, 0, sizeof fb);
    strncpy(fb[0], t, 39);
    if (strlen(t) > 39) strncpy(fb[1], t + 39, 39);
    pm_log_w("menu: %s", t);
    if (pm_menu_open) draw();
}

void pm_menu_set(int on) {
    if (on == pm_menu_open) return;
    pm_menu_open = on;
    pm_optui_disarm();
    memset(&rep_x, 0, sizeof rep_x);
    memset(&rep_y, 0, sizeof rep_y);
    if (on) {
        memset(fb, 0, sizeof fb);
        pm_log("menu: open");
        draw();
    } else {
        pm_options_close(); /* saves options.ini when something changed */
        pm_gpu_menu_draw("");
        pm_log("menu: close");
    }
}

/* PT60: direction from the D-pad or the circle pad, called every pad scan (60 Hz) while the menu is open. Same hold repeat as the Options panel. */
void pm_menu_hold(int dx, int dy) {
    int rows = pm_optui_rows(tab, MODE), ch = 0;
    if (pm_repeat(&rep_y, dy, 60)) {
        pm_optui_disarm();
        sel[tab] = (sel[tab] + (dy > 0 ? 1 : rows - 1)) % rows;
        ch = 1;
    }
    if (pm_repeat(&rep_x, dx, 60) && pm_optui_step(tab, sel[tab], dx * (rep_x.n > 12 ? 5 : 1), MODE)) ch = 1;
    if (ch) draw();
}

void pm_menu_key(unsigned k) {
    if (k & (KEY_A | KEY_B | KEY_L | KEY_R | KEY_SELECT)) memset(fb, 0, sizeof fb); /* a key clears the last message */
    if (k & KEY_SELECT) { pm_menu_set(0); return; }
    if (k & (KEY_L | KEY_R)) {
        tab = (tab + ((k & KEY_L) ? OPT_TABS - 1 : 1)) % OPT_TABS;
        pm_optui_disarm();
    } else if (k & KEY_B) {
        pm_menu_set(0);
        return;
    } else if (k & KEY_A) {
        int r = pm_optui_act(tab, sel[tab], MODE);
        if (r == 1 || r == 5) { pm_menu_set(0); return; }
        if (r == 3) { /* PT28: the main loop ends, pm_shutdown runs (HW7 order), main returns to the Homebrew Launcher. */
            pm_log("menu: exit to Homebrew requested");
            pm_menu_set(0); /* saves options.ini */
            pm_exit_request = 1;
            return;
        }
        if (r == 4) {
            pm_menu_set(0);
            pm_title_return();
            return;
        }
    } else return;
    draw();
}
