/* F11-1: port settings. options.txt in the working folder (sdmc:/3ds/PaperMario). One key=value per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pm_port.h"
#include "pm_build.h"

int pm_debug_value(const char* key, char* out, unsigned n);

PmOpt pm_opt = {0, 0, 1, 0, 0, 0, 0, 1, 1, 0}; /* fps60, wide, dbg_hud, dbg_stats, dbg_nofog, dbg_nolight, o3ds_z, log, loglevel (warn), fixedseed */

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
    fprintf(f, "version=1\nfps60=%d\nwide=%d\nhud=%d\nstats=%d\nnofog=%d\nnolight=%d\no3dsz=%d\nlog=%d\nloglevel=%d\nfixedseed=%d\n", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
            pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight, pm_opt.o3ds_z, pm_opt.log, pm_opt.loglevel, pm_opt.fixedseed);
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

/* BN1: "Build <n><suffix>" as a game message (0-9 = 0x10, A = 0x21, a = 0x41, '-' = 0x0D, space = 0xF7, end = 0xFD) for the Options panel footer. */
const unsigned char* pm_options_build_msg(void) {
    static unsigned char m[40];
    const char* t = "Build " PM_BUILD_STR;
    int n = 0;
    for (; *t && n < 38; t++, n++) {
        char c = *t;
        m[n] = c >= '0' && c <= '9' ? 0x10 + (c - '0') : c >= 'A' && c <= 'Z' ? 0x21 + (c - 'A') : c >= 'a' && c <= 'z' ? 0x41 + (c - 'a') : c == '-' ? 0x0D : 0xF7;
    }
    m[n] = 0xFD;
    return m;
}
