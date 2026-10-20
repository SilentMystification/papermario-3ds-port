/* F11-1: port settings. options.txt in the working folder (sdmc:/3ds/PaperMario). One key=value per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pm_port.h"
#include "pm_build.h"

int pm_debug_value(const char* key, char* out, unsigned n);

PmOpt pm_opt = {0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0}; /* fps60, wide, dbg_hud, dbg_stats, dbg_nofog, dbg_nolight, o3ds_z, log, loglevel (warn), fixedseed, dpad_mode (multi), warp (off) */

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
    if (!f) {
        pm_log_e("options: cannot write options.tmp");
        return;
    }
    fprintf(f, "version=1\nfps60=%d\nwide=%d\nhud=%d\nstats=%d\nnofog=%d\nnolight=%d\no3dsz=%d\nlog=%d\nloglevel=%d\nfixedseed=%d\ndpad_mode=%s\n", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
            pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, pm_opt.o3ds_z, pm_opt.log, pm_opt.loglevel, pm_opt.fixedseed, dpad_name[pm_opt.dpad_mode]);
    if (pm_opt.warp > 0) fprintf(f, "warp=%s\n", pm_warp_preset_name(pm_opt.warp - 1));
    for (int i = 0; i < PM_NIN; i++) if (pm_map[i] != map_def[i]) fprintf(f, "map.%s=%s\n", pm_in_name[i], pm_n64_name(pm_map[i]));
    if (fclose(f) != 0) {
        pm_log_e("options: write of options.tmp failed");
        return;
    }
    unlink("options.txt"); /* FAT rename does not replace an existing file */
    if (rename("options.tmp", "options.txt") != 0) pm_log_e("options: rename to options.txt failed");
}

void pm_options_init(void) {
    char line[64];
    int have = 0;
    FILE* f = fopen("options.txt", "r");
    if (f) {
        have = 1;
        while (fgets(line, sizeof(line), f)) {
            char* eq = strchr(line, '=');
            char* end;
            long v;
            int* fld;
            line[strcspn(line, "\r\n")] = 0;
            if (!line[0]) continue;
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
            if (!strcmp(line, "warp")) { /* UI1a: warp=<preset name> */
                int m;
                for (m = 0; m < pm_warp_preset_count() && strcmp(eq + 1, pm_warp_preset_name(m)); m++) {}
                if (m == pm_warp_preset_count()) pm_log_w("options: skip key '%s' value '%s'", line, eq + 1); else pm_opt.warp = m + 1;
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
    {   /* L02: a log switch wins over options.txt. The panel and the switches use pm_log_set_level and pm_log_set_file. */
        char v[16];
        if (pm_debug_value("loglevel", v, sizeof v)) pm_opt.loglevel = pm_log_level(); else pm_log_set_level(pm_opt.loglevel);
        if (pm_debug_value("log", v, sizeof v)) pm_opt.log = pm_log_file(); else pm_log_set_file(pm_opt.log);
    }
    pm_log("options: fps60=%d wide=%d hud=%d stats=%d nofog=%d nolight=%d (%s)", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
           pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, have ? "read from options.txt" : "defaults");
    pm_log_switch("nofog", pm_opt.dbg_nofog); /* a debug word that the code reads each frame */
    pm_log_switch("nolight", pm_opt.dbg_nolight);
    if (!have) pm_options_write();
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

/* Text to game message (0-9 = 0x10, A = 0x21, a = 0x41, '-' = 0x0D, space = 0xF7, end = 0xFD). Other characters become a space. */
const unsigned char* pm_options_msg(const char* t, int slot) {
    static unsigned char m[8][40];
    unsigned char* o = m[slot & 7];
    int n = 0;
    for (; *t && n < 38; t++, n++) {
        char c = *t;
        o[n] = c >= '0' && c <= '9' ? 0x10 + (c - '0') : c >= 'A' && c <= 'Z' ? 0x21 + (c - 'A') : c >= 'a' && c <= 'z' ? 0x41 + (c - 'a') : c == '-' ? 0x0D : 0xF7;
    }
    o[n] = 0xFD;
    return o;
}

/* BN1: "Build <n><suffix>" for the Options panel footer. */
const unsigned char* pm_options_build_msg(void) { return pm_options_msg("Build " PM_BUILD_STR, 7); }

/* UI1a: the rows of the Options panel in tabs. Both the file select panel and the Select menu use the same value and action code.
 * Tab 0 General, tab 1 Controls, tab 2 Debug. The last row of each tab is Back. */
static const char* const lvl_name[4] = { "Error", "Warn", "Info", "Debug" };
int pm_optui_rows(int tab) { return tab == 0 ? 3 : tab == 1 ? PM_NIN + 3 : 10; }
static const char* const gen_lbl[2] = { "60 FPS", "Widescreen" };
static const char* const dbg_lbl[7] = { "HUD", "Stats", "No fog", "No light", "Logging", "Log level", "Fixed seed" };
const char* pm_optui_label(int tab, int row) {
    if (row == pm_optui_rows(tab) - 1) return "Back";
    if (tab == 0) return gen_lbl[row];
    if (tab == 1) return row == 0 ? "D-PAD" : row <= PM_NIN ? pm_in_name[row - 1] : "Reset controls";
    return row < 7 ? dbg_lbl[row] : row == 7 ? "Warp to" : "Warp now";
}
const char* pm_optui_value(int tab, int row) {
    if (row == pm_optui_rows(tab) - 1) return NULL;
    if (tab == 0) return pm_options_get(row) ? "On" : "Off";
    if (tab == 1) return row == 0 ? pm_dpad_mode_name() : row <= PM_NIN ? pm_n64_name(pm_map[row - 1]) : NULL;
    if (row == 8) return NULL;
    if (row == 7) return pm_opt.warp ? pm_warp_preset_name(pm_opt.warp - 1) : "Off";
    return row == 5 ? lvl_name[pm_options_get(7) & 3] : pm_options_get(row + 2) ? "On" : "Off"; /* debug rows 0 to 6 are option rows 2 to 8 */
}
/* result: 0 nothing more, 1 close the panel (Back), 2 leave the file select now (Warp now) */
int pm_optui_act(int tab, int row) {
    if (row == pm_optui_rows(tab) - 1) return 1;
    if (tab == 0) { pm_options_flip(row); return 0; }
    if (tab == 1) {
        if (row == 0) pm_dpad_mode_next();
        else if (row <= PM_NIN) { pm_map[row - 1] = pm_n64_next(pm_map[row - 1]); pm_options_touch(); pm_log("options: map %s = %s", pm_in_name[row - 1], pm_n64_name(pm_map[row - 1])); }
        else { pm_map_reset(); pm_options_touch(); pm_log("options: controls reset"); }
        return 0;
    }
    if (row < 7) { pm_options_flip(row + 2); return 0; }
    if (row == 7) { pm_opt.warp = (pm_opt.warp + 1) % (pm_warp_preset_count() + 1); pm_options_touch(); return 0; }
    if (pm_opt.warp == 0) return 0;
    pm_options_close();
    pm_warp_now(pm_opt.warp - 1);
    return pm_warp_pending ? 2 : 0;
}
