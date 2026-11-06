/* F11-1: port settings. options.ini in the working folder (PT30: was options.txt, read as a fallback once) (sdmc:/3ds/PaperMario). One key=value per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pm_port.h"
#include "pm_build.h"

int pm_debug_value(const char* key, char* out, unsigned n);

/* Designated initializers: a value cannot move to the next field when a field is added. Defaults: fps60 off, wide off, dbg_hud on, dbg_stats, dbg_nofog, dbg_nolight, o3ds_z off, log on, loglevel warn, conlevel warn, fixedseed off, dpad_mode multi, dz_pad 5, dz_cs 0, rep_delay 500, rep_rate 200, fskip on, hud_ms 1000, pos_hud off, poslog off. */
PmOpt pm_opt = {
    .fps60 = 0, .wide = 0, .dbg_hud = 1, .dbg_stats = 0, .dbg_nofog = 0, .dbg_nolight = 0, .o3ds_z = 0,
    .log = 1, .loglevel = 1, .conlevel = 1, .fixedseed = 0, .dpad_mode = 0,
    .dz_pad = 5, .dz_cs = 0, .rep_delay = 500, .rep_rate = 200, .fskip = 1, .hud_ms = 1000, .pos_hud = 0, .poslog = 0,
};
/* Fails the build when PmOpt gains or loses a field without this initializer being updated. */
_Static_assert(sizeof(PmOpt) == 20 * sizeof(int), "PmOpt field count changed: update pm_opt above");
/* ARG1: the saved log and loglevel (options.ini, or the defaults). pm_opt.log and pm_opt.loglevel hold the values of this run (a launch argument may differ). CONLV: sv_conlevel the same for conlevel. */
static int sv_log = 1, sv_loglevel = 1, sv_conlevel = 1;
/* PROFROW: the Profiling row. Modes 0 off, 1 prof, 2 phase (the index of prof_name). prof_row is this run, sv_prof the saved value (options.ini profmode=). */
static const char* const prof_name[3] = { "off", "prof", "phase" };
static const char* const prof_label[3] = { "Off", "Prof", "Phase" };
static int prof_row, sv_prof;
void pm_prof_row_set(int mode); /* pm_phase.c: applies at the next drawn frame end */

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
    if (!strcmp(key, "conlevel")) return &pm_opt.conlevel;
    if (!strcmp(key, "fixedseed")) return &pm_opt.fixedseed;
    if (!strcmp(key, "fskip")) return &pm_opt.fskip;
    if (!strcmp(key, "poshud")) return &pm_opt.pos_hud;
    if (!strcmp(key, "poslog")) return &pm_opt.poslog; /* MN7 rest */
    return NULL;
}

void pm_options_write(void) {
    FILE* f = fopen("options.tmp", "w");
    /* PT30: INI file, key=value lines. The Warp-to choice is not saved (PT01b): a boot-time warp is only for the debug switches. */
    if (!f) {
        pm_log_e("options: cannot write options.tmp");
        return;
    }
    fprintf(f, "; Paper Mario 3DS port settings (key=value)\nversion=1\nfps60=%d\nwide=%d\nhud=%d\nstats=%d\nnofog=%d\nnolight=%d\no3dsz=%d\nlog=%d\nloglevel=%d\nconlevel=%d\nfixedseed=%d\ndpad_mode=%s\ndz_pad=%d\ndz_cs=%d\nrep_delay=%d\nrep_rate=%d\nfskip=%d\nhudint=%d\nposhud=%d\nposlog=%d\nprofmode=%s\n",pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
            pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, pm_opt.o3ds_z, sv_log, sv_loglevel, sv_conlevel, pm_opt.fixedseed, dpad_name[pm_opt.dpad_mode], pm_opt.dz_pad, pm_opt.dz_cs, pm_opt.rep_delay, pm_opt.rep_rate, pm_opt.fskip, pm_opt.hud_ms, pm_opt.pos_hud, pm_opt.poslog, prof_name[sv_prof]);
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
            if (!strcmp(line, "profmode")) { /* PROFROW */
                int m;
                for (m = 0; m < 3 && strcmp(eq + 1, prof_name[m]); m++) {}
                if (m == 3) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1); else prof_row = sv_prof = m;
                continue;
            }
            if (!strcmp(line, "warp")) { /* PT01b: an old file can hold warp=. A boot-time warp is only for the debug switches. */
                pm_log_w("options: warp= ignored at boot (value '%s')", eq + 1);
                continue;
            }
            if (!strcmp(line, "hudint")) { /* HUDINT: HUD memory rows refresh interval in ms, one of the five steps */
                v = strtol(eq + 1, &end, 10);
                if (end == eq + 1 || *end || (v != 250 && v != 500 && v != 1000 && v != 2000 && v != 5000)) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1);
                else pm_opt.hud_ms = (int)v;
                continue;
            }
            fld = opt_field(line);
            v = strtol(eq + 1, &end, 10);
            if (!fld || end == eq + 1 || *end || v < 0 || v > (fld == &pm_opt.conlevel ? 4 : fld == &pm_opt.loglevel ? 3 : 1)) {
                pm_log_w("options: skip key '%s' value '%s'", line, eq + 1);
                continue;
            }
            *fld = (int)v;
        }
        fclose(f);
    }
    sv_log = pm_opt.log;
    sv_loglevel = pm_opt.loglevel;
    sv_conlevel = pm_opt.conlevel;
    {   /* ARG1: precedence launch argument > saved option (options.ini) > built-in default. The argument sets this run only, sv_* keep the saved values.
         * Runs before pm_ph_init (pm_main.c), so phase, prof, pmu and fsplit read the final gate. */
        char v[16];
        if (pm_debug_value("loglevel", v, sizeof v)) pm_opt.loglevel = pm_log_level();
        if (pm_debug_value("conlevel", v, sizeof v)) pm_opt.conlevel = pm_log_conlevel(); /* CONLV */
        if (pm_debug_value("log", v, sizeof v)) pm_opt.log = pm_log_file();
        pm_log_set_level(pm_opt.loglevel);
        pm_log_set_conlevel(pm_opt.conlevel);
        pm_log_set_file(pm_opt.log);
    }
    pm_log("options: fps60=%d wide=%d hud=%d stats=%d nofog=%d nolight=%d (%s)", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
           pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, have ? (from_txt ? "read from options.txt, written as options.ini" : "read from options.ini") : "defaults");
    pm_log_i("options: boot dz_cs=%d rep_delay=%d rep_rate=%d hud_ms=%d conlevel=%d loglevel=%d log=%d", pm_opt.dz_cs, pm_opt.rep_delay,
             pm_opt.rep_rate, pm_opt.hud_ms, sv_conlevel, sv_loglevel, sv_log); /* OPTFIX: the saved values, as read from options.ini or the defaults */
    pm_log_switch("nofog", pm_opt.dbg_nofog); /* a debug word that the code reads each frame */
    pm_log_switch("nolight", pm_opt.dbg_nolight);
    if (!have || from_txt) pm_options_write();
}

/* F11-3: the panel. nofog is live. nolight is cached in pm_gbi.c at first use, so it takes effect at the next boot. */
static int changed;
void pm_options_touch(void) { changed = 1; }

/* PROFROW: a row change saves the mode and applies it at the next drawn frame. */
static void prof_set(int m) {
    prof_row = sv_prof = m;
    pm_options_touch();
    pm_prof_row_set(m);
    pm_log("options: profiling %s", prof_name[m]);
}
/* PROFROW: at boot a launch word (prof or phase) sets this run's mode, as ARG1 does for the log. Without one the saved mode is applied. */
void pm_options_prof_boot(void) {
    char v[16];
    if (pm_debug_has("prof") || pm_debug_value("prof", v, sizeof v)) { prof_row = 1; return; }
    if (pm_debug_has("phase")) { prof_row = 2; return; }
    prof_row = sv_prof;
    if (prof_row) pm_prof_row_set(prof_row);
}

int pm_options_get(int row) {
    if (row == 6) return pm_opt.log;
    if (row == 7) return pm_opt.loglevel; /* 0 error, 1 warn, 2 info, 3 debug */
    if (row == 11) return pm_opt.conlevel; /* CONLV: 0 error, 1 warn, 2 info, 3 debug, 4 off */
    if (row == 8) return pm_opt.fixedseed;
    if (row == 9) return pm_opt.fskip; /* FS1: frame skip */
    if (row == 10) return pm_opt.pos_hud; /* POSHUD */
    if (row == 12) return pm_opt.poslog; /* MN7 rest: Position log combo */
    int* f = row == 0 ? &pm_opt.fps60 : row == 1 ? &pm_opt.wide : row == 2 ? &pm_opt.dbg_hud : row == 3 ? &pm_opt.dbg_stats
           : row == 4 ? &pm_opt.dbg_nofog : row == 5 ? &pm_opt.dbg_nolight : NULL;
    return f ? *f : 0;
}

void pm_options_flip(int row) {
    if (row == 6 || row == 7) { /* F11-8: Logging On/Off, Log level (A cycles error, warn, info, debug) */
        if (row == 6) { pm_opt.log = !pm_opt.log; sv_log = pm_opt.log; pm_log_set_file(pm_opt.log); }
        else { pm_opt.loglevel = (pm_opt.loglevel + 1) & 3; sv_loglevel = pm_opt.loglevel; pm_log_set_level(pm_opt.loglevel); }
        changed = 1;
        return;
    }
    if (row == 11) { pm_opt.conlevel = (pm_opt.conlevel + 1) % 5; sv_conlevel = pm_opt.conlevel; pm_log_set_conlevel(pm_opt.conlevel); changed = 1; return; } /* CONLV: A cycles error, warn, info, debug, off */
    if (row == 8) { pm_opt.fixedseed = !pm_opt.fixedseed; changed = 1; return; } /* F11-7: takes effect at the next boot */
    if (row == 9) { pm_opt.fskip = !pm_opt.fskip; pm_fskip_set(pm_opt.fskip); changed = 1; return; }
    if (row == 10) { pm_opt.pos_hud = !pm_opt.pos_hud; changed = 1; return; } /* POSHUD: the Debug tab row 8 */
    if (row == 12) { pm_opt.poslog = !pm_opt.poslog; changed = 1; return; } /* MN7 rest: the combo logs a position line */
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
enum { RW_FPS, RW_WIDE, RW_TITLE, RW_EXIT, RW_BACK, RW_DPAD, RW_RESET, RW_WARPAREA, RW_DZPAD, RW_DZCS, RW_RDELAY, RW_RRATE, RW_CLOSE, RW_BOSSFOLDER, RW_MAP = 100, RW_DBG = 200, RW_GIVE = 300, RW_STAT = 310, RW_BOSS = 400, RW_PROF = 220, RW_SUB = 700, RW_WMAP = 800 };
/* PT01c, MN6 rest: the Warp area folder rows (tab PM_OPT_SUB): RW_SUB + row, the presets first, then one row per area. RW_WMAP + row is a map row of an area page. */
void pm_dbgg_count(int g, int* have, int* total); void pm_dbgg_give(int g); int pm_dbgg_stat_get(int k); void pm_dbgg_stat_step(int k, int dir);
static const char* const give_lbl[3] = { "Enable partners", "All badges", "All key items" };
static const char* const stat_lbl[6] = { "Max HP", "Max FP", "BP", "Level", "Coins", "Star points" };
const char* const pm_lvl_name[5] = { "Error", "Warn", "Info", "Debug", "Off" }; /* Off: only the console level (conlevel 4) */
static const char* const dbg_lbl[11] = { "HUD", "Stats", "No fog", "No light", "Logging", "Log level", "Fixed seed", "Frame skip", "Position HUD", "Console level", "Position log combo" };
static int tab_ids(int tab, int mode, int* ids) {
    int n = 0, i;
    if (tab == 0) {
        ids[n++] = RW_FPS; ids[n++] = RW_WIDE;
        if (mode) { ids[n++] = RW_TITLE; ids[n++] = RW_CLOSE; ids[n++] = RW_EXIT; }
    } else if (tab == 1) {
        ids[n++] = RW_DPAD; ids[n++] = RW_DZPAD; ids[n++] = RW_DZCS; ids[n++] = RW_RDELAY; ids[n++] = RW_RRATE;
        for (i = 0; i < PM_NIN; i++) ids[n++] = RW_MAP + i;
        ids[n++] = RW_RESET;
    } else {
        for (i = 0; i < 6; i++) ids[n++] = RW_DBG + i; /* HUD .. Log level */
        ids[n++] = RW_DBG + 9; /* Console level, next to Log level */
        for (i = 6; i < 9; i++) ids[n++] = RW_DBG + i; /* Fixed seed, Frame skip, Position HUD */
        ids[n++] = RW_PROF; /* PROFROW */
        for (i = 0; i < 3; i++) ids[n++] = RW_GIVE + i; /* PT67 */
        for (i = 0; i < 6; i++) ids[n++] = RW_STAT + i;
        ids[n++] = RW_WARPAREA; /* opens the Warp area folder */
        if (mode) ids[n++] = RW_BOSSFOLDER; /* MN4: opens the Boss folder (Select menu only) */
        ids[n++] = RW_DBG + 10; /* MN7 rest: Position log combo, after the folders so the rows above keep their numbers */
    }
    ids[n++] = RW_BACK;
    return n;
}
/* MN4: the folder page (tab PM_OPT_SUB) shows the Warp area rows (0) or the Boss rows (1). */
static int sub_kind;
/* MN6: the Warp area folder opens on a warning page (B warps on, any other key backs out). MN6 rest: the folder has two levels. Level 1 lists
 * the presets and the areas; A on an area opens its page (level 2, sub_area = area index): one row per map, < > moves the entry, A warps.
 * The last area row and the last map row of the warp memory are kept in RAM (not saved), entering again restores them and logs it. */
static int warp_warn, warp_last_row, sub_area = -1, warp_mem_area = -1, warp_mem_row;
static char sub_nm[48];
int pm_optui_warn(void) { return warp_warn; }
void pm_optui_warn_confirm(void) { warp_warn = 0; }
/* B or A on the folder: a level 2 page pops to the area list and returns that area row (>= 0); a level 1 page closes the folder (-1). */
int pm_optui_sub_pop(int row) {
    if (!sub_kind && sub_area >= 0) {
        warp_mem_area = sub_area; warp_mem_row = row;
        sub_area = -1;
        warp_last_row = pm_warp_preset_count() + warp_mem_area;
        return warp_last_row;
    }
    if (!sub_kind) warp_last_row = row;
    warp_warn = 0;
    return -1;
}
int pm_optui_sub_restore(void) {
    int r = warp_last_row, P = pm_warp_preset_count();
    if (sub_kind || r < 0) return 0;
    if (r >= P + pm_warp_area_count()) return 0;
    pm_log("menu: warp sub restore area %d row %d", r < P ? -1 : r - P, r); /* area -1: a preset row */
    return r;
}
/* MN6 rest: an area page opens (level 2). The cursor goes to the map row of the warp memory when it was this area, else row 0. */
int pm_optui_sub_enter(void) {
    int r = warp_mem_area == sub_area ? warp_mem_row : 0, n = pm_warp_area_size(sub_area);
    if (r < 0 || r >= n) r = 0;
    pm_log("menu: warp area %d maps %d row %d", sub_area, n, r);
    return r;
}
int pm_optui_sub_level(void) { return sub_area >= 0 && !sub_kind ? 2 : 1; }
const char* pm_optui_sub_name(void) {
    if (sub_kind) return "Boss";
    if (sub_area < 0) return "Warp area";
    snprintf(sub_nm, sizeof sub_nm, "Warp area > %s", pm_warp_area_name(sub_area));
    return sub_nm;
}
/* MN6 rest: the entry of map k as shown and used (clamped to the entry count of the map). */
static int wmap_entry(int k) {
    int n = pm_warp_map_entries(k), e = pm_warp_entry_get(k);
    return e < n ? e : 0;
}
static int row_id(int tab, int row, int mode) {
    int ids[48], n;
    if (tab == PM_OPT_SUB) return sub_kind ? RW_BOSS + row : sub_area >= 0 ? RW_WMAP + row : RW_SUB + row;
    n = tab_ids(tab, mode, ids);
    return row >= 0 && row < n ? ids[row] : RW_BACK;
}
/* MN3: one armed row (Return to Title or Exit to Homebrew Menu). First A arms, the second A on the same row runs it. */
static int armed_id;
void pm_optui_disarm(void) { armed_id = 0; }
int pm_optui_alert(int tab, int row, int mode) { int id = row_id(tab, row, mode); return armed_id && id == armed_id; }
int pm_optui_rows(int tab, int mode) {
    int ids[48];
    if (tab == PM_OPT_SUB && warp_warn) return 2;
    if (tab == PM_OPT_SUB) return sub_kind ? pm_boss_count() : sub_area >= 0 ? pm_warp_area_size(sub_area) : pm_warp_preset_count() + pm_warp_area_count();
    return tab_ids(tab, mode, ids);
}
const char* pm_optui_label(int tab, int row, int mode) {
    int id;
    if (tab == PM_OPT_SUB && warp_warn) return row == 0 ? "Warping is dangerous." : "A warp can break the save file.";
    id = row_id(tab, row, mode);
    if (id >= RW_WMAP) return pm_warp_map_name(pm_warp_area_map(sub_area, id - RW_WMAP));
    if (id >= RW_SUB) { int r = id - RW_SUB, P = pm_warp_preset_count(); return r < P ? pm_warp_preset_name(r) : pm_warp_area_name(r - P); }
    if (id == RW_PROF) return "Profiling";
    if (id >= RW_BOSS) return pm_boss_label(id - RW_BOSS);
    if (id >= RW_STAT) return stat_lbl[id - RW_STAT];
    if (id >= RW_GIVE) return give_lbl[id - RW_GIVE];
    if (id >= RW_DBG) return dbg_lbl[id - RW_DBG];
    if (id >= RW_MAP) return pm_in_name[id - RW_MAP];
    switch (id) {
        case RW_FPS: return "60 FPS";
        case RW_WIDE: return "Widescreen";
        case RW_TITLE: return "Return to Title Screen";
        case RW_CLOSE: return "Close game";
        case RW_EXIT: return "Exit to Homebrew Menu";
        case RW_DPAD: return "D-PAD";
        case RW_DZPAD: return "Circle pad dead zone";
        case RW_DZCS: return "C-Stick dead zone";
        case RW_RDELAY: return "Repeat delay";
        case RW_RRATE: return "Repeat rate";
        case RW_RESET: return "Reset controls";
        case RW_WARPAREA: return "Warp area";
        case RW_BOSSFOLDER: return "Boss >";
        default: return "Back";
    }
}
const char* pm_optui_value(int tab, int row, int mode) {
    int id;
    if (tab == PM_OPT_SUB && warp_warn) return NULL;
    id = row_id(tab, row, mode);
    if (id >= RW_WMAP) { /* a map row shows its entry and the entry count */
        static char eb[16];
        int k = pm_warp_area_map(sub_area, id - RW_WMAP);
        snprintf(eb, sizeof eb, "< %d of %d >", wmap_entry(k), pm_warp_map_entries(k));
        return eb;
    }
    if (id >= RW_SUB) return id - RW_SUB < pm_warp_preset_count() ? pm_warp_preset_map(id - RW_SUB) : "A: open"; /* a preset shows its map, an area row opens */
    if (id == RW_PROF) return prof_label[prof_row];
    if (id >= RW_BOSS) return "A: fight";
    if (id >= RW_GIVE) {
        static char gb[16][16];
        char* b = gb[id - RW_GIVE];
        if (id >= RW_STAT) snprintf(b, 16, "%d", pm_dbgg_stat_get(id - RW_STAT));
        else { int h, t; pm_dbgg_count(id - RW_GIVE, &h, &t); snprintf(b, 16, "%d/%d", h, t); }
        return b;
    }
    if (id >= RW_DBG) return id == RW_DBG + 5 ? pm_lvl_name[pm_options_get(7) & 3] : id == RW_DBG + 9 ? pm_lvl_name[pm_options_get(11) % 5] : pm_options_get(id - RW_DBG + 2) ? "On" : "Off"; /* debug rows 0 to 8 are option rows 2 to 10 */
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
        case RW_EXIT: case RW_TITLE: case RW_CLOSE: return armed_id == id ? "Press A again" : NULL;
        case RW_WARPAREA: case RW_BOSSFOLDER: return "A: open";
        default: return NULL;
    }
}
/* PT01c: left (dir < 0) or right (dir > 0) on a row. A prefix row with more than one map walks its members with wrap. */
int pm_optui_step(int tab, int row, int dir, int mode) {
    int id = row_id(tab, row, mode);
    if (id >= RW_WMAP) { /* MN6 rest: left and right move the entry of the map (wrap). RAM only (PT32): no pm_options_touch */
        int k = pm_warp_area_map(sub_area, id - RW_WMAP), n = pm_warp_map_entries(k);
        pm_warp_entry_set(k, ((wmap_entry(k) + dir) % n + n) % n);
        return 1;
    }
    if (id == RW_PROF) { prof_set((prof_row + (dir > 0 ? 1 : 2)) % 3); return 1; } /* PROFROW: left and right cycle off, prof, phase */
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
    return 0;
}
/* result: 0 nothing more, 1 close the panel (Back), 3 exit to Homebrew (second press),
 * 4 return to the title, 5 warp started (close the menu), 6 open the Warp area submenu */
int pm_optui_act(int tab, int row, int mode) {
    int id;
    if (tab == PM_OPT_SUB && warp_warn) return 0; /* the warning page: the panel handles B and A */
    id = row_id(tab, row, mode);
    if (id != armed_id) armed_id = 0;
    if (id >= RW_WMAP) { /* MN6 rest: A warps to the map of this row at its entry (the warp memory is the last one warped) */
        int j = id - RW_WMAP, k = pm_warp_area_map(sub_area, j);
        if (!pm_warp_live(pm_warp_preset_count() + 1 + k, wmap_entry(k))) return 0;
        warp_mem_area = sub_area; warp_mem_row = j;
        return 5;
    }
    if (id >= RW_SUB) { /* PT01c: A warps to the shown preset at entry 0; an area row opens its page (MN6 rest) */
        int r = id - RW_SUB, P = pm_warp_preset_count();
        if (r < P) return pm_warp_live(r + 1, 0) ? 5 : 0;
        sub_area = r - P;
        return 8;
    }
    if (id == RW_PROF) { prof_set((prof_row + 1) % 3); return 0; } /* PROFROW: A cycles */
    if (id >= RW_BOSS) return pm_boss_launch(id - RW_BOSS) ? 5 : 0; /* 5: started, the menu closes */
    if (id >= RW_STAT) { pm_dbgg_stat_step(id - RW_STAT, 1); return 0; }
    if (id >= RW_GIVE) { pm_dbgg_give(id - RW_GIVE); pm_log("options: debug give %d", id - RW_GIVE); return 0; }
    if (id >= RW_DBG) { pm_options_flip(id == RW_DBG + 9 ? 11 : id - RW_DBG + 2); return 0; } /* RW_DBG + 9 is Console level, option row 11 */
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
        case RW_TITLE:
            if (armed_id != RW_TITLE) { armed_id = RW_TITLE; return 0; }
            armed_id = 0;
            return 4;
        case RW_CLOSE: /* MN3b: first A arms, second A closes the game to the HOME Menu (pm_options_ui.c) */
            if (armed_id != RW_CLOSE) { armed_id = RW_CLOSE; return 0; }
            armed_id = 0;
            return 7;
        case RW_EXIT:
            if (armed_id != RW_EXIT) { armed_id = RW_EXIT; pm_log("menu: exit armed, press A again"); return 0; }
            armed_id = 0;
            return 3;
        case RW_WARPAREA: sub_kind = 0; sub_area = -1; warp_warn = 1; pm_log("menu: warp warning"); return 6; /* the menu opens the submenu (PM_OPT_SUB) */
        case RW_BOSSFOLDER: sub_kind = 1; sub_area = -1; return 6; /* MN4: the Boss folder (PM_OPT_SUB, Boss rows) */
        case RW_DZPAD: case RW_DZCS: case RW_RDELAY: case RW_RRATE: pm_optui_step(tab, row, 1, mode); return 0;
    }
    return 0;
}
