/* F11-1: port settings. options.txt in the working folder (sdmc:/3ds/PaperMario). One key=value per line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pm_port.h"

PmOpt pm_opt = {0, 0, 1, 0, 0, 0}; /* fps60, wide, dbg_hud, dbg_stats, dbg_nofog, dbg_nolight */

static int* opt_field(const char* key) {
    if (!strcmp(key, "fps60")) return &pm_opt.fps60;
    if (!strcmp(key, "wide")) return &pm_opt.wide;
    if (!strcmp(key, "hud")) return &pm_opt.dbg_hud;
    if (!strcmp(key, "stats")) return &pm_opt.dbg_stats;
    if (!strcmp(key, "nofog")) return &pm_opt.dbg_nofog;
    if (!strcmp(key, "nolight")) return &pm_opt.dbg_nolight;
    return NULL;
}

void pm_options_write(void) {
    FILE* f = fopen("options.tmp", "w");
    if (!f) {
        pm_log("options: cannot write options.tmp");
        return;
    }
    fprintf(f, "version=1\nfps60=%d\nwide=%d\nhud=%d\nstats=%d\nnofog=%d\nnolight=%d\n", pm_opt.fps60, pm_opt.wide, pm_opt.dbg_hud,
            pm_opt.dbg_stats, pm_opt.dbg_nofog, pm_opt.dbg_nolight);
    if (fclose(f) != 0) {
        pm_log("options: write of options.tmp failed");
        return;
    }
    unlink("options.txt"); /* FAT rename does not replace an existing file */
    if (rename("options.tmp", "options.txt") != 0) pm_log("options: rename to options.txt failed");
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
                pm_log("options: skip line '%s'", line);
                continue;
            }
            *eq = 0;
            if (!strcmp(line, "version")) continue;
            fld = opt_field(line);
            v = strtol(eq + 1, &end, 10);
            if (!fld || end == eq + 1 || *end || (v != 0 && v != 1)) {
                pm_log("options: skip key '%s' value '%s'", line, eq + 1);
                continue;
            }
            *fld = (int)v;
        }
        fclose(f);
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
    int* f = row == 0 ? &pm_opt.fps60 : row == 1 ? &pm_opt.wide : row == 2 ? &pm_opt.dbg_hud : row == 3 ? &pm_opt.dbg_stats
           : row == 4 ? &pm_opt.dbg_nofog : row == 5 ? &pm_opt.dbg_nolight : NULL;
    return f ? *f : 0;
}

void pm_options_flip(int row) {
    int* f = row == 0 ? &pm_opt.fps60 : row == 1 ? &pm_opt.wide : row == 2 ? &pm_opt.dbg_hud : row == 3 ? &pm_opt.dbg_stats
           : row == 4 ? &pm_opt.dbg_nofog : row == 5 ? &pm_opt.dbg_nolight : NULL;
    if (!f) return;
    *f = !*f;
    changed = 1;
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
