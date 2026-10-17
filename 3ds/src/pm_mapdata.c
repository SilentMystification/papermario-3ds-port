/* Slice MS01 (class E, debug tool, port layer, no game logic): measure which static data of a map changes while the map
 * is loaded. Switch `dataprobe`. Reads the ranges file ms_ranges.txt of the SD app folder (made by
 * 3ds/tools/map_data_sizes.py from the link map of the same build): lines "<map id|@machi|@dgb> <hex addr> <size> <d|b> <symbol>".
 * kind d = .data (pristine = copy taken at the first entry of the map), kind b = .bss (pristine = zero).
 * Called from __wrap_load_map_by_IDs (pm_map.c): compares the map that is left, then the map that is entered (dirty on entry). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_port.h"

typedef struct { unsigned addr, size; char kind; char owner[8]; char sym[48]; } PmRange;
typedef struct { char id[8]; unsigned char* d; unsigned n; unsigned chg_max; } PmPristine;

static PmRange* rg;
static int rg_n, probe_on = -1;
static PmPristine pr[400];
static int pr_n;
static char cur[8];

static int owns(const PmRange* r, const char* id) {
    if (!strcmp(r->owner, id)) return 1;
    if (!strcmp(r->owner, "@machi")) return !strncmp(id, "mac_", 4);
    if (!strcmp(r->owner, "@dgb")) return !strncmp(id, "dgb_", 4);
    return 0;
}

static void load_ranges(void) {
    FILE* f = fopen("ms_ranges.txt", "r");
    char line[128];
    int cap = 0;
    if (!f) { pm_log("dprobe: no ms_ranges.txt"); probe_on = 0; return; }
    while (fgets(line, sizeof line, f)) cap++;
    rewind(f);
    rg = calloc(cap, sizeof(PmRange));
    while (rg && fgets(line, sizeof line, f)) {
        PmRange* r = &rg[rg_n];
        char k;
        if (sscanf(line, "%7s %x %u %c %47s", r->owner, &r->addr, &r->size, &k, r->sym) == 5) { r->kind = k; rg_n++; }
    }
    fclose(f);
    pm_log("dprobe: %d ranges", rg_n);
}

static void probe_one(const char* phase, PmPristine* p, const char* id) {
    unsigned off = 0, i, j, bytes = 0, syms = 0, total = 0;
    for (i = 0; i < (unsigned)rg_n; i++) {
        const PmRange* r = &rg[i];
        const unsigned char* cur_b = (const unsigned char*)r->addr;
        unsigned chg = 0;
        if (!owns(r, id)) continue;
        total += r->size;
        for (j = 0; j < r->size; j++) {
            unsigned char was = r->kind == 'd' ? p->d[off + j] : 0;
            if (cur_b[j] != was) chg++;
        }
        if (r->kind == 'd') off += r->size;
        if (chg) {
            pm_log("dprobe: %s %s %c %s size %u chg %u", phase, id, r->kind, r->sym, r->size, chg);
            bytes += chg;
            syms++;
        }
    }
    pm_log("dprobe: %s %s sum syms %u bytes %u of %u", phase, id, syms, bytes, total);
    if (bytes > p->chg_max) p->chg_max = bytes;
}

void pm_mapdata_probe(const char* id) {
    int i, k;
    unsigned off = 0, i2;
    PmPristine* p = NULL;
    if (probe_on < 0) { probe_on = pm_debug_has("dataprobe"); if (probe_on) load_ranges(); }
    if (!probe_on || !rg) return;
    for (k = 0; k < pr_n; k++) if (!strcmp(pr[k].id, cur)) { probe_one("leave", &pr[k], cur); break; }
    for (i = 0; i < pr_n; i++) if (!strcmp(pr[i].id, id)) p = &pr[i];
    if (p) { probe_one("entry", p, id); }
    else if (pr_n < 400) {
        p = &pr[pr_n++];
        memset(p, 0, sizeof *p);
        strncpy(p->id, id, 7);
        for (i2 = 0; i2 < (unsigned)rg_n; i2++) if (owns(&rg[i2], id) && rg[i2].kind == 'd') p->n += rg[i2].size;
        p->d = malloc(p->n ? p->n : 1);
        if (!p->d) { pm_log("dprobe: out of memory %s %u", id, p->n); pr_n--; return; }
        for (i2 = 0; i2 < (unsigned)rg_n; i2++) if (owns(&rg[i2], id) && rg[i2].kind == 'd') { memcpy(p->d + off, (void*)rg[i2].addr, rg[i2].size); off += rg[i2].size; }
        pm_log("dprobe: first %s data %u", id, p->n);
    }
    strncpy(cur, id, 7);
    cur[7] = 0;
}

/* MS01: what the N64 overlay DMA did for the data of a map. Called from __wrap_dma_copy (pm_map.c) when load_map_by_IDs
 * copies the (empty) overlay of a linked map. First entry: keep a copy of the .data block (the pristine value). Later entries:
 * copy it back. The .bss block is zeroed (an initialised zero is .bss here, on the N64 it is .data). */
extern PmMapRst pm_map_rst[];
extern const int pm_map_rst_n;

static int rst_owns(const char* owner, const char* id) {
    if (!strcmp(owner, id)) return 1;
    if (!strcmp(owner, "machi")) return !strncmp(id, "mac_", 4);
    if (!strcmp(owner, "dgb")) return !strncmp(id, "dgb_", 4);
    return 0;
}

void pm_mapdata_probe_after(const char* id) {
    int k;
    if (probe_on > 0 && rg) for (k = 0; k < pr_n; k++) if (!strcmp(pr[k].id, id)) probe_one("after", &pr[k], id);
}

void pm_mapdata_enter(const char* id) {
    int i;
    unsigned bz = 0, bd = 0;
    for (i = 0; i < pm_map_rst_n; i++) {
        PmMapRst* r = &pm_map_rst[i];
        unsigned n = r->d1 - r->d0;
        if (!rst_owns(r->id, id)) continue;
        if (n) {
            if (!r->save) {
                r->save = malloc(n);
                if (r->save) memcpy(r->save, r->d0, n);
                else pm_log("mapdata: out of memory %s %u", r->id, n);
            } else {
                memcpy(r->d0, r->save, n);
                bd += n;
            }
        }
        if (r->b1 > r->b0) { memset(r->b0, 0, r->b1 - r->b0); bz += r->b1 - r->b0; }
    }
    if (pm_debug_has("dataprobe")) {
        pm_log("mapdata: %s restored data %u bss %u", id, bd, bz);
        pm_mapdata_probe_after(id); /* what is still different from the first entry after the restore: changers not in map_restore.txt */
    }
}
