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

/* MN3b: the HOME Menu title of this region (0004001000021000 + 0x100 per region: JPN 0, USA 1, EUR 2; other regions use USA). */
static void pm_home_chainload(void) {
    u8 region = 1;
    u64 tid;
    cfguInit();
    CFGU_SecureInfoGetRegion(&region);
    cfguExit();
    if (region > 2) region = 1;
    tid = 0x0004001000021000ULL + ((u64)region << 8);
    aptSetChainloader(tid, MEDIATYPE_NAND); /* void in this libctru */
    pm_log("menu: HOME Menu chainload %016llx region %u", (unsigned long long)tid, (unsigned)region);
}
volatile int pm_exit_request; /* PT28: set by the menu row Exit to Homebrew, read by the main loop (pm_main.c) */

#define OPT_TABS 4 /* CH1: 0 General, 1 Controls, 2 Debug, 3 Cheats (Select menu only) */
#define VIS 20 /* PT69, PT76, MN3: 30 text rows (tab, blank, 20 options, 2, controls, 3, 2 feedback), see draw */
#define MODE 1
/* PT01c: tab PM_OPT_SUB is the Warp area submenu. A open sets tab to it and home_tab keeps the tab it was opened from (B returns there). */
static int tab, home_tab, sel[PM_OPT_SUB + 1], top[PM_OPT_SUB + 1];
static PmRepeat rep_y, rep_x;
static const char* const tab_name[PM_OPT_SUB + 1] = { "General", "Controls", "Debug", "Cheats", "Warp area" }; /* Boss is a folder under Debug (MN4) */
const char* pm_optui_sub_name(void);
int pm_optui_sub_kind(void);

/* PT75, PT76, PT77: the 30 rows of the bottom screen, from the top: row 0 tab line, rows 1 to VIS the options, 2 empty rows, the controls line,
 * 3 empty rows, then the 2 row feedback area (rows 28 and 29). Every error of the menu goes through pm_menu_msg. */
static char fb[2][40];
void pm_menu_msg(const char* fmt, ...);
static void draw(void) {
    char out[2400];
    int n = 0, rows = pm_optui_rows(tab, MODE);
    if (sel[tab] < top[tab]) top[tab] = sel[tab];
    if (sel[tab] >= top[tab] + VIS) top[tab] = sel[tab] - VIS + 1;
    if (tab == PM_OPT_SUB) n += snprintf(out + n, sizeof out - (size_t)n, "%s > %s", tab_name[home_tab], pm_optui_sub_name()); /* MN4 breadcrumb */
    else for (int t = 0; t < OPT_TABS; t++) n += snprintf(out + n, sizeof out - (size_t)n, t == tab ? "[%s] " : " %s  ", tab_name[t]);
    n += snprintf(out + n, sizeof out - (size_t)n, "\n\n"); /* MN3: a blank line under the tab line */
    for (int r = top[tab]; r < top[tab] + VIS && r < rows; r++) {
        const char* v = pm_optui_value(tab, r, MODE);
        /* MN3: the armed row (second A runs it): label bright blue, value "Press A again" bright red (ANSI, bottom console) */
        if (pm_optui_alert(tab, r, MODE)) n += snprintf(out + n, sizeof out - (size_t)n, "%c \x1b[1;34m%-22s\x1b[1;31m%s\x1b[0m" "\n", r == sel[tab] ? '>' : ' ', pm_optui_label(tab, r, MODE), v ? v : "");
        else n += snprintf(out + n, sizeof out - (size_t)n, "%c %-22s %s" "\n", r == sel[tab] ? '>' : ' ', pm_optui_label(tab, r, MODE), v ? v : "");
    }
    for (int r = (rows < VIS ? rows : VIS); r < VIS; r++) n += snprintf(out + n, sizeof out - (size_t)n, "\n");
    n += snprintf(out + n, sizeof out - (size_t)n, "\n\n%s\n\n\n\n%s\n%s", tab == PM_OPT_SUB ? (pm_optui_warn() ? "B: warp menu  A: back" : pm_optui_sub_kind() == 2 ? "< > change  A: +1  B: back" : !strcmp(pm_optui_sub_name(), "Boss") ? "A: fight  B: back" : pm_optui_sub_level() == 2 ? "< > entry  A: warp  B: back" : "A: open or warp  B: back") : "L R: tab  < > A: change  B: close", fb[0], fb[1]);
    pm_log_d("menu: tab %d row %d %s = %s", tab, sel[tab], pm_optui_label(tab, sel[tab], MODE), pm_optui_value(tab, sel[tab], MODE) ? pm_optui_value(tab, sel[tab], MODE) : "-");
    pm_gpu_menu_draw(out);
    if (pm_debug_has("shotmenu")) { /* MN0: test aid, one bottom shot per menu draw as shots/bot_m<seq>.ppm */
        static unsigned mseq;
        char nm[24];
        snprintf(nm, sizeof nm, "bot_m%04u", mseq++);
        pm_gpu_shot_bottom(nm);
        pm_log("menu: shot %s tab %d row %d", nm, tab, sel[tab]);
    }
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
        if (tab == PM_OPT_SUB) tab = home_tab; /* the menu opens on the tab it was in */
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
    if ((k & (KEY_L | KEY_R)) && tab != PM_OPT_SUB) { /* the submenu has no tab change */
        tab = (tab + ((k & KEY_L) ? OPT_TABS - 1 : 1)) % OPT_TABS;
        pm_optui_disarm();
    } else if ((k & KEY_B) && tab == PM_OPT_SUB && pm_optui_warn()) { /* MN6: B on the warning page is "warp menu" */
        pm_optui_warn_confirm();
        sel[tab] = pm_optui_sub_restore(); top[tab] = 0;
    } else if (k & KEY_B) {
        int back;
        if (tab != PM_OPT_SUB) { pm_menu_set(0); return; }
        back = pm_optui_sub_pop(sel[tab]);
        if (back >= 0) { /* MN6 rest: an area page goes back to the area list, the menu stays in the folder */
            sel[tab] = back; top[tab] = 0;
            pm_log("menu: page pop depth 1");
        } else {
            tab = home_tab; /* B in the submenu goes back to the parent tab, the menu stays open */
            pm_log("menu: page pop depth 0");
        }
        pm_optui_disarm();
    } else if ((k & KEY_A) && tab == PM_OPT_SUB && pm_optui_warn()) { /* MN6: any other key on the warning page backs out */
        pm_optui_sub_pop(sel[tab]);
        tab = home_tab;
        pm_log("menu: page pop depth 0");
        pm_optui_disarm();
    } else if (k & KEY_A) {
        int r = pm_optui_act(tab, sel[tab], MODE);
        if (r == 6) { home_tab = tab; tab = PM_OPT_SUB; sel[tab] = 0; top[tab] = 0; pm_optui_disarm(); pm_log("menu: page push %d depth 1", PM_OPT_SUB); }
        if (r == 8) { sel[tab] = pm_optui_sub_enter(); top[tab] = 0; pm_optui_disarm(); pm_log("menu: page push %d depth 2", PM_OPT_SUB); } /* MN6 rest: an area page */
        if (r == 1 || r == 5) { pm_menu_set(0); return; }
        if (r == 7) { /* MN3b: Close game. The app ends and the HOME Menu runs (chainload, region HOME Menu title). */
            pm_log("menu: close game to HOME Menu");
            pm_menu_set(0);
            pm_home_chainload();
            pm_exit_request = 1;
            return;
        }
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
