/* F11-1: port settings. options.ini in the working folder (PT30: was options.txt, read as a fallback once) (sdmc:/3ds/PaperMario). One key=value per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pm_port.h"
#include "pm_build.h"

int pm_debug_value(const char* key, char* out, unsigned n);

PmOpt pm_opt = {0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 5, 0, 500, 200}; /* fps60, wide, dbg_hud, dbg_stats, dbg_nofog, dbg_nolight, o3ds_z, log, loglevel (warn), fixedseed, dpad_mode (multi), warp (off), warp_entry, dz_pad 5, dz_cs 0 */

/* F11-5: N64 button masks by name, the cycle order of the menu, and the 3DS input names (options.txt keys map.<input>=<button>) */
static const struct { const char* name; unsigned short mask; } n64_btn[] = { {"NONE", 0}, {"A", 0x8000}, {"B", 0x4000}, {"Z", 0x2000}, {"START", 0x1000}, {"L", 0x0020}, {"R", 0x0010},
                                                                              {"CUP", 0x0008}, {"CDOWN", 0x0004}, {"CLEFT", 0x0002}, {"CRIGHT", 0x0001} };
#define N_BTN ((int)(sizeof n64_btn / sizeof n64_btn[0]))
const char* const pm_in_name[PM_NIN] = { "A", "B", "X", "Y", "L", "R", "ZL", "ZR", "START", "DUP", "DDOWN", "DLEFT", "DRIGHT", "CSUP", "CSDOWN", "CSLEFT", "CSRIGHT" };
static const unsigned short map_def[PM_NIN] = { 0x8000, 0x4000, 0x0008, 0x0004, 0x2000, 0x2000, 0x0020, 0x0010, 0x1000, 0x0008, 0x0004, 0x0002, 0x0001, 0x0008, 0x0004, 0x0002, 0x0001 };
unsigned short pm_map[PM_NIN] = { 0x8000, 0x4000, 0x0008, 0x0004, 0x2000, 0x2000, 0x0020, 0x0010, 0x1000, 0x0008, 0x0004, 0x0002, 0x0001, 0x0008, 0x0004, 0x0002, 0x0001 };
const char* pm_n64_name(unsigned short mask) {
    for (int i = 0; i < N_BTN; i++) if (n64_btn[i].mask == mask) return n64_btn[i].name;
    return "?";
}
unsigned short pm_n64_next(unsigned short mask) {
    for (int i = 0; i < N_BTN; i++) if (n64_btn[i].mask == mask) return n64_btn[(i + 1) % N_BTN].mask;
    return n64_btn[0].mask;
}
/* PT60: one hold repeat for the Options panel and the Select menu, for the D-pad and the circle pad. The first step fires at once, the second after
 * rep_delay ms (500 = 15 game updates), the next ones every rep_rate ms (200 = 6 updates, the F11-9 gap of the panel). */
int pm_repeat(PmRepeat* r, int dir, int tps) {
    int delay = pm_opt.rep_delay * tps / 1000, rate = pm_opt.rep_rate * tps / 1000;
    if (dir == 0) { r->dir = 0; r->t = 0; r->n = 0; return 0; }
    if (dir != r->dir) { r->dir = dir; r->t = 0; r->n = 1; return 1; }
    if (++r->t >= (r->n == 1 ? delay : rate)) { r->t = 0; r->n++; return 1; }
    return 0;
}
/* UI1c: D-pad mode. One setting, two views (the Controls tab and the Select menu). The O3DS has no analog choice. */
static const char* const dpad_name[3] = { "multi", "cstick", "analog" };
static const char* const dpad_label[3] = { "Multifunction", "To C-Stick", "To Analog" };
static int dpad_eff(void) { return pm_opt.dpad_mode == 2 && !pm_new3ds ? 0 : pm_opt.dpad_mode; }
const char* pm_dpad_mode_name(void) { return dpad_label[dpad_eff()]; }
void pm_dpad_mode_next(void) {
    pm_opt.dpad_mode = (dpad_eff() + 1) % (pm_new3ds ? 3 : 2);
    pm_options_touch();
    pm_log("options: dpad_mode %s", dpad_name[pm_opt.dpad_mode]);
}
void pm_map_reset(void) { memcpy(pm_map, map_def, sizeof pm_map); }

static int* opt_field(const char* key) {
    if (!strcmp(key, "fps60")) return &pm_opt.fps60;
    if (!strcmp(key, "wide")) return &pm_opt.wide;
    if (!strcmp(key, "hud")) return &pm_opt.dbg_hud;
    if (!strcmp(key, "stats")) return &pm_opt.dbg_stats;
    if (!strcmp(key, "nofog")) return &pm_opt.dbg_nofog;
    if (!strcmp(key, "nolight")) return &pm_opt.dbg_nolight;
    if (!strcmp(key, "o3dsz")) return &pm_opt.o3ds_z;
    if (!strcmp(key, "log")) return &pm_opt.log;
    if (!strcmp(key, "loglevel")) return &pm_opt.loglevel;
    if (!strcmp(key, "fixedseed")) return &pm_opt.fixedseed;
    return NULL;
}

void pm_options_write(void) {
    FILE* f = fopen("options.tmp", "w");
    /* PT30: INI file, key=value lines. The Warp-to choice is not saved (PT01b): a boot-time warp is only for the debug switches. */
    if (!f) {
        pm_log_e("options: cannot write options.tmp");
        return;
    }
    fprintf(f, "; Paper Mario 3DS port settings (key=value)\nversion=1\nfps60=%d\nwide=%d\nhud=%d\nstats=%d\nnofog=%d\nnolight=%d\no3dsz=%d\nlog=%d\nloglevel=%d\nfixedseed=%d\ndpad_mode=%s\ndz_pad=%d\ndz_cs=%d\nrep_delay=%d\nrep_rate=%d\n", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
            pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, pm_opt.o3ds_z, pm_opt.log, pm_opt.loglevel, pm_opt.fixedseed, dpad_name[pm_opt.dpad_mode], pm_opt.dz_pad, pm_opt.dz_cs, pm_opt.rep_delay, pm_opt.rep_rate);
    for (int i = 0; i < PM_NIN; i++) if (pm_map[i] != map_def[i]) fprintf(f, "map.%s=%s\n", pm_in_name[i], pm_n64_name(pm_map[i]));
    if (fclose(f) != 0) {
        pm_log_e("options: write of options.tmp failed");
        return;
    }
    unlink("options.ini"); /* FAT rename does not replace an existing file */
    if (rename("options.tmp", "options.ini") != 0) pm_log_e("options: rename to options.ini failed");
}

void pm_options_init(void) {
    char line[64];
    int have = 0;
    FILE* f = fopen("options.ini", "r");
    int from_txt = 0;
    if (!f && (f = fopen("options.txt", "r")) != NULL) from_txt = 1; /* PT30: old name, read once, options.ini is written below */
    if (f) {
        have = 1;
        while (fgets(line, sizeof(line), f)) {
            char* eq = strchr(line, '=');
            char* end;
            long v;
            int* fld;
            line[strcspn(line, "\r\n")] = 0;
            if (!line[0] || line[0] == ';' || line[0] == '#' || line[0] == '[') continue; /* PT30: INI comment and section lines */
            if (!eq) {
                pm_log_w("options: skip line '%s'", line);
                continue;
            }
            *eq = 0;
            if (!strcmp(line, "version")) continue;
            if (!strncmp(line, "map.", 4)) { /* F11-5: map.<3DS input>=<N64 button> */
                int in, bt;
                for (in = 0; in < PM_NIN && strcmp(line + 4, pm_in_name[in]); in++) {}
                for (bt = 0; bt < N_BTN && strcmp(eq + 1, n64_btn[bt].name); bt++) {}
                if (in == PM_NIN || bt == N_BTN) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1);
                else { pm_map[in] = n64_btn[bt].mask; pm_log("options: map %s = %s", line + 4, eq + 1); }
                continue;
            }
            if (!strcmp(line, "dpad_mode")) { /* UI1c */
                int m;
                for (m = 0; m < 3 && strcmp(eq + 1, dpad_name[m]); m++) {}
                if (m == 3) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1); else pm_opt.dpad_mode = m;
                continue;
            }
            if (!strcmp(line, "dz_pad") || !strcmp(line, "dz_cs") || !strcmp(line, "rep_delay") || !strcmp(line, "rep_rate")) { /* PT61, PT60 */
                int k = !strcmp(line, "dz_pad") ? 0 : !strcmp(line, "dz_cs") ? 1 : !strcmp(line, "rep_delay") ? 2 : 3;
                static const int lo[4] = { 0, 0, 200, 50 }, hi[4] = { 50, 50, 1000, 500 };
                int* tf = k == 0 ? &pm_opt.dz_pad : k == 1 ? &pm_opt.dz_cs : k == 2 ? &pm_opt.rep_delay : &pm_opt.rep_rate;
                v = strtol(eq + 1, &end, 10);
                if (end == eq + 1 || *end || v < lo[k] || v > hi[k]) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1); else *tf = (int)v;
                continue;
            }
            if (!strcmp(line, "warp")) { /* PT01b: an old file can hold warp=. A boot-time warp is only for the debug switches. */
                pm_log_w("options: warp= ignored at boot (value '%s')", eq + 1);
                continue;
            }
            fld = opt_field(line);
            v = strtol(eq + 1, &end, 10);
            if (!fld || end == eq + 1 || *end || v < 0 || v > (fld == &pm_opt.loglevel ? 3 : 1)) {
                pm_log_w("options: skip key '%s' value '%s'", line, eq + 1);
                continue;
            }
            *fld = (int)v;
        }
        fclose(f);
    }
    {   /* L02: a log switch wins over options.ini. The panel and the switches use pm_log_set_level and pm_log_set_file. */
        char v[16];
        if (pm_debug_value("loglevel", v, sizeof v)) pm_opt.loglevel = pm_log_level(); else pm_log_set_level(pm_opt.loglevel);
        if (pm_debug_value("log", v, sizeof v)) pm_opt.log = pm_log_file(); else pm_log_set_file(pm_opt.log);
    }
    pm_log("options: fps60=%d wide=%d hud=%d stats=%d nofog=%d nolight=%d (%s)", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
           pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, have ? (from_txt ? "read from options.txt, written as options.ini" : "read from options.ini") : "defaults");
    pm_log_switch("nofog", pm_opt.dbg_nofog); /* a debug word that the code reads each frame */
    pm_log_switch("nolight", pm_opt.dbg_nolight);
    if (!have || from_txt) pm_options_write();
}

/* F11-3: the panel. nofog is live. nolight is cached in pm_gbi.c at first use, so it takes effect at the next boot. */
static int changed;
void pm_options_touch(void) { changed = 1; }

int pm_options_get(int row) {
    if (row == 6) return pm_opt.log;
    if (row == 7) return pm_opt.loglevel; /* 0 error, 1 warn, 2 info, 3 debug */
    if (row == 8) return pm_opt.fixedseed;
    int* f = row == 0 ? &pm_opt.fps60 : row == 1 ? &pm_opt.wide : row == 2 ? &pm_opt.dbg_hud : row == 3 ? &pm_opt.dbg_stats
           : row == 4 ? &pm_opt.dbg_nofog : row == 5 ? &pm_opt.dbg_nolight : NULL;
    return f ? *f : 0;
}

void pm_options_flip(int row) {
    if (row == 6 || row == 7) { /* F11-8: Logging On/Off, Log level (A cycles error, warn, info, debug) */
        if (row == 6) { pm_opt.log = !pm_opt.log; pm_log_set_file(pm_opt.log); }
        else { pm_opt.loglevel = (pm_opt.loglevel + 1) & 3; pm_log_set_level(pm_opt.loglevel); }
        changed = 1;
        return;
    }
    if (row == 8) { pm_opt.fixedseed = !pm_opt.fixedseed; changed = 1; return; } /* F11-7: takes effect at the next boot */
    int* f = row == 0 ? &pm_opt.fps60 : row == 1 ? &pm_opt.wide : row == 2 ? &pm_opt.dbg_hud : row == 3 ? &pm_opt.dbg_stats
           : row == 4 ? &pm_opt.dbg_nofog : row == 5 ? &pm_opt.dbg_nolight : NULL;
    if (!f) return;
    *f = !*f;
    changed = 1;
    {   /* F11-10: apply now */
        extern int pm_interp_want, pm_wide_cfg, pm_nolight;
        if (row == 0) pm_interp_want = *f; /* next game step */
        if (row == 1) pm_wide_cfg = *f || pm_debug_has("wide"); /* pm_wide follows at the next game mode change (the panel is a 4:3 mode) */
        if (row == 5) pm_nolight = *f || pm_debug_has("nolight");
        pm_log("options: live row %d value %d", row, *f);
    }
    if (row == 4) pm_log_switch("nofog", *f);
    if (row == 5) pm_log_switch("nolight", *f);
}

void pm_options_close(void) {
    if (!changed) return;
    changed = 0;
    pm_log("options: fps60=%d wide=%d hud=%d stats=%d nofog=%d nolight=%d (saved)", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
           pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight);
    pm_options_write();
}

/* Text to game message (0-9 = 0x10, A = 0x21, a = 0x41, '-' = 0x0D, ':' '<' '>' '_', space = 0xF7, end = 0xFD). Other characters become a space. */
const unsigned char* pm_options_msg(const char* t, int slot) {
    static unsigned char m[8][40];
    unsigned char* o = m[slot & 7];
    int n = 0;
    for (; *t && n < 38; t++, n++) {
        char c = *t;
        o[n] = c >= '0' && c <= '9' ? 0x10 + (c - '0') : c >= 'A' && c <= 'Z' ? 0x21 + (c - 'A') : c >= 'a' && c <= 'z' ? 0x41 + (c - 'a') : c == '-' ? 0x0D : c == ':' ? 0x1A : c == '<' ? 0x1C : c == '>' ? 0x1E : c == '_' ? 0x3F : 0xF7;
    }
    o[n] = 0xFD;
    return o;
}

/* BN1: "Build <n><suffix>" for the Options panel footer. */
const unsigned char* pm_options_build_msg(void) { return pm_options_msg("Build " PM_BUILD_STR, 7); }

/* UI1a, PT59: the rows of the Options panel in tabs. The file select panel (mode 0) and the Select menu (mode 1) use the same table, the same value code and
 * the same action code. Tab 0 General, tab 1 Controls, tab 2 Debug. The last row of each tab is Back. Mode 1 adds the rows that only make sense in the
 * port menu (Return to Title Screen, Exit to Homebrew) and its Warp now changes the map at once. */
enum { RW_FPS, RW_WIDE, RW_TITLE, RW_EXIT, RW_BACK, RW_DPAD, RW_RESET, RW_WARPTO, RW_ENTRY, RW_WARPNOW, RW_DZPAD, RW_DZCS, RW_RDELAY, RW_RRATE, RW_MAP = 100, RW_DBG = 200, RW_GIVE = 300, RW_STAT = 310, RW_BOSS = 400 };
void pm_dbgg_count(int g, int* have, int* total); void pm_dbgg_give(int g); int pm_dbgg_stat_get(int k); void pm_dbgg_stat_step(int k, int dir);
static const char* const give_lbl[3] = { "Enable partners", "All badges", "All key items" };
static const char* const stat_lbl[6] = { "Max HP", "Max FP", "BP", "Level", "Coins", "Star points" };
static const char* const lvl_name[4] = { "Error", "Warn", "Info", "Debug" };
static const char* const dbg_lbl[7] = { "HUD", "Stats", "No fog", "No light", "Logging", "Log level", "Fixed seed" };
static int tab_ids(int tab, int mode, int* ids) {
    int n = 0, i;
    if (tab == 0) {
        ids[n++] = RW_FPS; ids[n++] = RW_WIDE;
        if (mode) { ids[n++] = RW_TITLE; ids[n++] = RW_EXIT; }
    } else if (tab == 1) {
        ids[n++] = RW_DPAD; ids[n++] = RW_DZPAD; ids[n++] = RW_DZCS; ids[n++] = RW_RDELAY; ids[n++] = RW_RRATE;
        for (i = 0; i < PM_NIN; i++) ids[n++] = RW_MAP + i;
        ids[n++] = RW_RESET;
    } else {
        for (i = 0; i < 7; i++) ids[n++] = RW_DBG + i;
        for (i = 0; i < 3; i++) ids[n++] = RW_GIVE + i; /* PT67 */
        for (i = 0; i < 6; i++) ids[n++] = RW_STAT + i;
        ids[n++] = RW_WARPTO; ids[n++] = RW_ENTRY; ids[n++] = RW_WARPNOW;
    }
    if (tab == 3) { n = 0; for (i = 0; i < pm_boss_count(); i++) ids[n++] = RW_BOSS + i; } /* Select menu only (mode 1): the Boss tab */
    ids[n++] = RW_BACK;
    return n;
}
static int row_id(int tab, int row, int mode) {
    int ids[48], n = tab_ids(tab, mode, ids);
    return row >= 0 && row < n ? ids[row] : RW_BACK;
}
static int exit_armed;
void pm_optui_disarm(void) { exit_armed = 0; }
int pm_optui_rows(int tab, int mode) { int ids[48]; return tab_ids(tab, mode, ids); }
const char* pm_optui_label(int tab, int row, int mode) {
    int id = row_id(tab, row, mode);
    if (id >= RW_BOSS) return pm_boss_label(id - RW_BOSS);
    if (id >= RW_STAT) return stat_lbl[id - RW_STAT];
    if (id >= RW_GIVE) return give_lbl[id - RW_GIVE];
    if (id >= RW_DBG) return dbg_lbl[id - RW_DBG];
    if (id >= RW_MAP) return pm_in_name[id - RW_MAP];
    switch (id) {
        case RW_FPS: return "60 FPS";
        case RW_WIDE: return "Widescreen";
        case RW_TITLE: return "Return to Title Screen";
        case RW_EXIT: return "Exit to Homebrew Menu";
        case RW_DPAD: return "D-PAD";
        case RW_DZPAD: return "Circle pad dead zone";
        case RW_DZCS: return "C-Stick dead zone";
        case RW_RDELAY: return "Repeat delay";
        case RW_RRATE: return "Repeat rate";
        case RW_RESET: return "Reset controls";
        case RW_WARPTO: return "Warp to";
        case RW_ENTRY: return "Entry";
        case RW_WARPNOW: return "Warp now";
        default: return "Back";
    }
}
const char* pm_optui_value(int tab, int row, int mode) {
    int id = row_id(tab, row, mode);
    if (id >= RW_BOSS) return "A: fight";
    if (id >= RW_GIVE) {
        static char gb[16][16];
        char* b = gb[id - RW_GIVE];
        if (id >= RW_STAT) snprintf(b, 16, "%d", pm_dbgg_stat_get(id - RW_STAT));
        else { int h, t; pm_dbgg_count(id - RW_GIVE, &h, &t); snprintf(b, 16, "%d/%d", h, t); }
        return b;
    }
    if (id >= RW_DBG) return id == RW_DBG + 5 ? lvl_name[pm_options_get(7) & 3] : pm_options_get(id - RW_DBG + 2) ? "On" : "Off"; /* debug rows 0 to 6 are option rows 2 to 8 */
    if (id >= RW_MAP) return pm_n64_name(pm_map[id - RW_MAP]);
    switch (id) {
        case RW_FPS: case RW_WIDE: return pm_options_get(id == RW_FPS ? 0 : 1) ? "On" : "Off";
        case RW_DPAD: return pm_dpad_mode_name();
        case RW_DZPAD: case RW_DZCS: case RW_RDELAY: case RW_RRATE: {
            static char nb[4][12];
            char* b = nb[id - RW_DZPAD];
            snprintf(b, 12, id == RW_DZPAD ? "%d%%" : id == RW_DZCS ? "%d%%" : "%d ms", id == RW_DZPAD ? pm_opt.dz_pad : id == RW_DZCS ? pm_opt.dz_cs : id == RW_RDELAY ? pm_opt.rep_delay : pm_opt.rep_rate);
            return b;
        }
        case RW_EXIT: return exit_armed ? "A again" : NULL;
        case RW_WARPTO: case RW_ENTRY: { /* PT01: < value > shows that left and right change it */
            static char wb[32];
            if (id == RW_WARPTO) snprintf(wb, sizeof wb, "< %s >", pm_warp_choice_text(pm_opt.warp, pm_opt.warp_entry));
            else if (pm_opt.warp > pm_warp_preset_count()) snprintf(wb, sizeof wb, "< %d >", pm_opt.warp_entry);
            else return "-";
            return wb;
        }
        default: return NULL;
    }
}
/* PT01: left (dir < 0) or right (dir > 0) on a row. Warp to walks the choices (off, presets, every map of gAreas) with wrap. Entry walks 0 to 15. */
int pm_optui_step(int tab, int row, int dir, int mode) {
    int id = row_id(tab, row, mode);
    if (id >= RW_STAT) { pm_dbgg_stat_step(id - RW_STAT, dir); return 1; } /* PT67c: live, not saved */
    if (id >= RW_DZPAD && id <= RW_RRATE) { /* PT61, PT60: a number row. Left and right step it, A steps up and wraps. */
        static const struct { int lo, hi, st; } rg[4] = { { 0, 50, 5 }, { 0, 50, 5 }, { 200, 1000, 100 }, { 50, 500, 50 } };
        int* f = id == RW_DZPAD ? &pm_opt.dz_pad : id == RW_DZCS ? &pm_opt.dz_cs : id == RW_RDELAY ? &pm_opt.rep_delay : &pm_opt.rep_rate;
        int k = id - RW_DZPAD, v = *f + dir * rg[k].st;
        if (dir > 0 && *f >= rg[k].hi) v = rg[k].lo; /* wrap with A, stop at the end with right? both wrap */
        if (dir < 0 && *f <= rg[k].lo) v = rg[k].hi;
        if (v < rg[k].lo) v = rg[k].lo;
        if (v > rg[k].hi) v = rg[k].hi;
        *f = v;
        pm_options_touch();
        return 1;
    }
    if (id != RW_WARPTO && id != RW_ENTRY) return 0;
    if (id == RW_WARPTO) {
        int n = pm_warp_choice_count();
        pm_opt.warp = ((pm_opt.warp + dir) % n + n) % n;
        if (pm_opt.warp <= pm_warp_preset_count()) pm_opt.warp_entry = 0;
    } else {
        if (pm_opt.warp <= pm_warp_preset_count()) return 0;
        pm_opt.warp_entry = ((pm_opt.warp_entry + dir) % 16 + 16) % 16;
    }
    return 1; /* the warp choice is not saved (PT32): no pm_options_touch */
}
/* result: 0 nothing more, 1 close the panel (Back), 2 leave the file select now (Warp now, mode 0),
 * 3 exit to Homebrew (mode 1, second press), 4 return to the title (mode 1), 5 warp started (mode 1, close the menu) */
int pm_optui_act(int tab, int row, int mode) {
    int id = row_id(tab, row, mode);
    if (id != RW_EXIT) exit_armed = 0;
    if (id >= RW_BOSS) return pm_boss_launch(id - RW_BOSS) ? 5 : 0; /* 5: started, the menu closes */
    if (id >= RW_STAT) { pm_dbgg_stat_step(id - RW_STAT, 1); return 0; }
    if (id >= RW_GIVE) { pm_dbgg_give(id - RW_GIVE); pm_log("options: debug give %d", id - RW_GIVE); return 0; }
    if (id >= RW_DBG) { pm_options_flip(id - RW_DBG + 2); return 0; }
    if (id >= RW_MAP) {
        int i = id - RW_MAP;
        pm_map[i] = pm_n64_next(pm_map[i]); pm_options_touch(); pm_log("options: map %s = %s", pm_in_name[i], pm_n64_name(pm_map[i]));
        return 0;
    }
    switch (id) {
        case RW_BACK: return 1;
        case RW_FPS: pm_options_flip(0); return 0;
        case RW_WIDE: pm_options_flip(1); return 0;
        case RW_DPAD: pm_dpad_mode_next(); return 0;
        case RW_RESET: pm_map_reset(); pm_options_touch(); pm_log("options: controls reset"); return 0;
        case RW_TITLE: return 4;
        case RW_EXIT:
            if (!exit_armed) { exit_armed = 1; return 0; }
            exit_armed = 0;
            return 3;
        case RW_WARPTO: case RW_ENTRY: case RW_DZPAD: case RW_DZCS: case RW_RDELAY: case RW_RRATE: pm_optui_step(tab, row, 1, mode); return 0;
        case RW_WARPNOW:
            if (pm_opt.warp == 0) { if (mode) pm_menu_msg("Warp to is Off: pick a target first"); return 0; }
            if (mode) return pm_warp_live(pm_opt.warp, pm_opt.warp_entry) ? 5 : 0;
            pm_options_close();
            pm_warp_now(pm_opt.warp, pm_opt.warp_entry);
            return pm_warp_pending ? 2 : 0;
    }
    return 0;
}
