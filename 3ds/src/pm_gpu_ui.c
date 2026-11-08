/* pm_gpu_ui.c: the bottom screen text (port menu lines, HUD rows, footer, log frame), moved from pm_gpu.c without change (C8). The present, the grey pass and the shots stay in pm_gpu.c. */
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "pm_gpu.h"
#include "pm_port.h"
#include "pm_gpu_int.h"
volatile int pos_on, stats_on; /* set by pm_gpu_hud on the game thread: the Mario row and the stats row are drawn */
int frame_key = -1, frame_lay = -1; /* LOG5: pm_log_header_key of the drawn header, and the layout key; -1: not drawn yet */
volatile int hud_new, foot_new;
char foot_row[2][40]; /* BOTLAYOUT: footer rows 28 (map) and 29 (apt, model, build) */
PrintConsole bottom_con;

/* F11-6, PT64, PT65: the port menu text on the bottom console. The menu owns rows 3 to 14 (MENU_ROWS lines), the log keeps rows 16 to 30 and keeps
 * scrolling there while the menu is open. Nothing is cleared: a line is rewritten, padded with spaces, only when its text changed, so the
 * screen never shows an empty frame between a clear and a redraw (PT64). On close the menu rows are blanked and the log gets its old window back. */
#define MENU_ROWS 30 /* PT69: the menu fills the whole bottom screen (rows 0 to 29); the log and the HUD wait while it is open */
#define MENU_W 39
#define MENU_LN 192 /* MN0: a line with ANSI colour codes is longer than its 38 columns */
static char menu_prev[MENU_ROWS][MENU_LN];
int menu_on;
int menu_dirty; /* MN1: a menu draw since the last present (pm_gpu_menu_present) */
static char menu_l0[MENU_LN];
/* MN0: pad or cut a line to 38 visible columns. An ESC [ ... letter sequence takes no column, so a coloured line keeps its text. */
static size_t menu_fit(char* b, size_t cap, const char* s) {
    const size_t cols = MENU_W - 1;
    size_t o = 0, vis = 0;
    const char* p = s;
    while (*p && o < cap - cols - 2) {
        if (p[0] == 0x1b && p[1] == '[') {
            size_t j = 2;
            while (p[j] && !(p[j] >= 0x40 && p[j] <= 0x7e)) j++;
            if (!p[j]) break;
            memcpy(b + o, p, j + 1); o += j + 1; p += j + 1;
            continue;
        }
        if (vis < cols) { b[o++] = *p; vis++; }
        p++;
    }
    while (vis < cols) { b[o++] = ' '; vis++; }
    b[o] = 0;
    return o;
}
static void menu_put_line(PrintConsole* c, int i, const char* s) {
    char b[MENU_LN];
    menu_fit(b, sizeof b, s); /* one column spare: the last column never wraps the 1 row window */
    if (menu_on && !strcmp(b, menu_prev[i])) return;
    memcpy(menu_prev[i], b, sizeof b);
    consoleSetWindow(c, 0, 0, 40, 30); /* PT79: the whole screen as the window and an absolute cursor move: a 1 row window on row 29 scrolled the text up by a row */
    printf("\x1b[%d;1H\x1b[0m", i + 1); /* MAPHUD: the reset ends a red value (pm_options_ui.c) of an earlier line; it is not counted in the 38 columns */
    fputs(b, stdout);
}
void pm_gpu_menu_draw(const char* text) {
    PrintConsole* c = &bottom_con;
    int i = 0, was_on = menu_on, cx, cy, wx, wy, ww, wh;
    if (!c->consoleInitialised) return;
    menu_dirty = 1;
    pm_con_lock();
    cx = c->cursorX; cy = c->cursorY; wx = c->windowX; wy = c->windowY; ww = c->windowWidth; wh = c->windowHeight;
    consoleSelect(c);
    if (!text || !text[0]) {
        if (menu_on) {
            for (i = 0; i < MENU_ROWS; i++) { menu_prev[i][0] = 1; menu_put_line(c, i, ""); }
            consoleSetWindow(c, 0, 0, 40, 30); consoleClear(); /* PAUSE BOTTOM: the whole console is cleared at the close, no menu row or old log text stays */
            menu_on = 0;
            consoleSetWindow(c, 1, LOG_TOP, 40, LOG_ROWS); /* the log window again, new lines come at its top (the rows are blank) */
            c->cursorX = 1; c->cursorY = 1; /* window-relative: the top row of the log window */
            hud_new = 1; foot_new = 1; frame_key = -1; /* BOTLAYOUT: the menu blanked the HUD, footer and frame rows: draw them again */
            fflush(stdout);
        }
        pm_con_unlock();
        return;
    }
    if (!menu_on) { consoleSetWindow(c, 0, 0, 40, 30); consoleClear(); for (int k = 0; k < MENU_ROWS; k++) menu_prev[k][0] = 1; } /* once at open: no old log text in the last columns */
    for (; i < MENU_ROWS; i++) { /* one text line per row, an empty line stays an empty row */
        const char* e = strchr(text, '\n');
        char ln[MENU_LN];
        size_t n = e ? (size_t)(e - text) : strlen(text);
        size_t k = n > MENU_LN - 1 ? MENU_LN - 1 : n; /* MN0: the colour codes are kept, menu_fit cuts the visible columns */
        memcpy(ln, text, k);
        ln[k] = 0;
        if (i == 0) memcpy(menu_l0, ln, k + 1);
        menu_put_line(c, i, ln);
        text += e ? n + 1 : n;
    }
    for (; i < MENU_ROWS; i++) menu_put_line(c, i, "");
    menu_on = 1;
    if (!was_on) { menu_prev[0][0] = 1; menu_put_line(c, 0, menu_l0); } /* PT75: row 0 (the tab line) was lost on the first draw: write it again */
    fflush(stdout);
    (void)was_on;
    consoleSetWindow(c, wx, wy, ww, wh); /* the log window and its cursor stay as they were (the log is not printed while the menu is open) */
    c->cursorX = cx; c->cursorY = cy;
    pm_con_unlock();
}

static void hud_put_line(PrintConsole* c, int row, const char* s) {
    char b[40];
    snprintf(b, sizeof b, "%-39.39s", s ? s : "");
    c->cursorX = 1;
    c->cursorY = row;
    fputs(b, stdout);
}

/* PI6: the game thread only copies the three HUD rows (no printf, no console call). pm_gpu_hud_flush draws them on the
 * log sink thread, at most 10 times per second. A HUD row written during a draw may show one update late. */
static char hud_rows[6][40]; /* [0] fps, [1] memory, [2] VRAM, [3] to [5] the map block (rows 4 to 6) */
static char pos_row[40]; /* POSHUD: Mario's row, drawn after the map block when pos_on */
static char stat_row[40]; /* stats: the stats row, drawn after the Mario row when stats_on */
static void row_copy(char* d, const char* s) { strncpy(d, s ? s : "", 39); d[39] = 0; }
/* pos: NULL = no Mario row. stats: NULL = no stats row. map: the 3 row map block, always. */
void pm_gpu_hud(const char* line0, const char* stats, char (*mem)[40], const char* pos, char (*map)[40]) {
    if (menu_on) return; /* PT69: the menu owns all rows */
    row_copy(hud_rows[0], line0);
    row_copy(hud_rows[1], mem[0]);
    row_copy(hud_rows[2], mem[1]);
    for (int r = 0; r < 3; r++) row_copy(hud_rows[3 + r], map[r]);
    pos_on = pos != NULL;
    if (pos) row_copy(pos_row, pos);
    stats_on = stats != NULL;
    if (stats) row_copy(stat_row, stats);
    hud_new = 1;
}

/* BOTLAYOUT: the separator, the log header and the footer. Absolute rows (ANSI), window the whole screen while they print, then the log window again.
 * The separator is 39 columns: a 40th column would wrap. */
static void frame_draw(PrintConsole* c) {
    char sep[40], top[40], panel[40], live[24];
    memset(sep, '-', 39);
    sep[39] = 0;
    pm_log_header(panel, sizeof panel, live, sizeof live); /* CFG-2: the panel row, and the live connection on the separator above it */
    memcpy(top, sep, sizeof top);
    int ll = (int)strlen(live), at = 39 - (ll + 2);
    if (at >= 0) { top[at] = ' '; memcpy(top + at + 1, live, (size_t)ll); top[at + 1 + ll] = ' '; }
    consoleSetWindow(c, 0, 0, 40, 30);
    printf("\x1b[%d;1H\x1b[0m%s", LOG_TOP - 2, top);
    printf("\x1b[%d;1H\x1b[0m%-39s", LOG_TOP - 1, panel);
    printf("\x1b[27;1H\x1b[0m%s", sep);
    printf("\x1b[30;1H%39s", "");
    consoleSetWindow(c, 1, LOG_TOP, 40, LOG_ROWS);
    frame_key = pm_log_header_key();
    frame_lay = LAY_KEY;
}

/* BOTLAYOUT: footer rows 28 (map) and 29 (apt, model, build). Set by the game thread, drawn by the sink thread. */
void pm_gpu_footer(const char* map, const char* sys) {
    if (!strncmp(foot_row[0], map, 39) && !strncmp(foot_row[1], sys, 39)) return;
    strncpy(foot_row[0], map, 39);
    foot_row[0][39] = 0;
    strncpy(foot_row[1], sys, 39);
    foot_row[1][39] = 0;
    foot_new = 1;
}

/* Log sink thread only. Draws the HUD rows, the footer and the frame (header levels) on the bottom console. */
void pm_gpu_hud_flush(void) {
    PrintConsole* c = &bottom_con;
    int cx, cy, wx, wy, ww, wh;
    int hud = hud_new, foot = foot_new;
    int frame = frame_key != pm_log_header_key() || frame_lay != LAY_KEY;
    int lay = frame_lay != LAY_KEY; /* the layout changed (poshud or stats row), the log window and cursor restart at its top */
    if (!(hud || foot || frame) || !c->consoleInitialised) return;
    pm_con_lock();
    if (menu_on) { /* B1: the menu test is under con_lock (pm_gpu_menu_draw draws under this lock): the flags stay set for the close */
        pm_con_unlock();
        return;
    }
    hud_new = 0;
    foot_new = 0;
    cx = c->cursorX;
    cy = c->cursorY;
    wx = c->windowX;
    wy = c->windowY;
    ww = c->windowWidth;
    wh = c->windowHeight;
    consoleSelect(c);
    if (hud) {
        int r = 7;
        consoleSetWindow(c, 1, 1, 40, 6 + HUD_X); /* cursor row 1 is the top row of the screen */
        for (int i = 0; i < 6; i++) hud_put_line(c, i + 1, hud_rows[i]);
        if (pos_on) hud_put_line(c, r++, pos_row);
        if (stats_on) hud_put_line(c, r++, stat_row);
    }
    if (foot) { /* BOTLAYOUT: footer rows 28 and 29 */
        consoleSetWindow(c, 0, 0, 40, 30);
        printf("\x1b[28;1H\x1b[0m%-39.39s", foot_row[0]);
        printf("\x1b[29;1H\x1b[0m%-39.39s", foot_row[1]);
    }
    if (frame) frame_draw(c);
    if (lay) { wx = 1; wy = LOG_TOP; ww = 40; wh = LOG_ROWS; cx = 1; cy = 1; }
    consoleSetWindow(c, wx, wy, ww, wh);
    c->cursorX = cx;
    c->cursorY = cy;
    pm_con_unlock();
}
