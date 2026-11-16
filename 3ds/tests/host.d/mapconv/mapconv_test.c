/* MAPCONV host test: every section of every converted shape (nd_shape_conv output) is loaded with the device loader (the dlcp_* code of 3ds/src/nd_shape.c, cut
 * out by mapconv.sh into the include file named by LOADER_INC) and compared with the compile of the same list: the dlcp_check fields (nops, exit state, groups, geometry
 * ops, vertices, indices), a refusal section against the refusal code, and the pool full load (groups only) against the compile without pools. A list without a
 * section for a variant the device asks for is a failure. usage: mapconv_test <dir with .nds and .dlc> [--seed] */
#define _FILE_OFFSET_BITS 64 /* readdir of a 32 bit build on a 64 bit inode mount (the docker host mounts) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include "ultra64.h"
#include "gbi_custom.h"
#include "nd_op.h"
#include "nd.h"
#define u32 uint32_t /* the loader text of nd_shape.c uses the device type */

static const char* g_dir;
static const char* g_stage = "start"; /* the last step, printed by the fault handler */
static char g_nm[64];
static unsigned g_list, g_var;
static void on_segv(int sig) { char b[200]; int n = snprintf(b, sizeof b, "mapconv FAIL: signal %d map %s list %08x var %u stage %s\n", sig, g_nm, g_list, g_var, g_stage); if (write(2, b, (size_t)n) < 0) _exit(3); _exit(1); }
static u8* g_shape;
static u32 g_size;
#define gMapShapeData g_shape
unsigned pm_dlc_gen = 1;
char pm_shape_name[24];
struct { int on; unsigned dlcp_t; } pm_xf;
static unsigned pm_ticks(void) { return 0; }
static u32 seam_le32(const u8* p) { return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }
static const NdSeamTab* seam_get(void) { return NULL; }
static unsigned pool_v;
static uint8_t* nd_env_valloc(unsigned n, unsigned* first) { *first = 7u + pool_v; pool_v += n + 3u; return (uint8_t*)malloc(n * 16u + 1u); }
static uint16_t* nd_env_ialloc(unsigned n) { return (uint16_t*)malloc(n * 2u + 1u); }
static FILE* test_fopen(const char* path, const char* mode) {
    char p[512];
    if (strncmp(path, "romfs:/shape/", 13) != 0) return NULL;
    snprintf(p, sizeof p, "%s/%s", g_dir, path + 13);
    return fopen(p, mode);
}
#define fopen test_fopen
#include LOADER_INC
#undef fopen

static const void* ref_vtx(uint32_t w1, unsigned n) {
    if (!n || n > 64 || w1 < (u32)(uintptr_t)g_shape || w1 - (u32)(uintptr_t)g_shape + 16u * n > g_size) return NULL;
    return (const void*)(uintptr_t)w1;
}
static unsigned long n_ok, n_ref, n_fail;
static void bad(const char* nm, u32 list, unsigned var, const char* what) {
    printf("mapconv FAIL %s list %08x var %u: %s\n", nm, (unsigned)list, var, what);
    n_fail++;
}
/* NULL when equal: the compile r and the loaded o. rel: compare vertex and index data (both made with the pools: vbase and indices relative to vfirst) */
static const char* diff(const NdObj* r, const NdObj* o, int rel) {
    unsigned k;
    if (r->nops != o->nops || r->exit_am != o->exit_am || r->exit_om != o->exit_om || r->ngrp != o->ngrp || r->ngop != o->ngop || r->nvtx != o->nvtx || r->nidx != o->nidx || r->nins != o->nins) return "counts";
    for (k = 0; k < r->ngrp; k++) {
        NdGrp a = o->grp[k], b = r->grp[k];
        a.vbase -= o->vfirst; b.vbase -= r->vfirst;
        if (memcmp(&a, &b, sizeof a) != 0) return "group";
    }
    if (r->ngop && (memcmp(r->gop, o->gop, r->ngop * 8u) != 0 || memcmp(r->gat, o->gat, r->ngop * 2u) != 0)) return "gop";
    if (rel) {
        if (r->nvtx && memcmp(r->vtx, o->vtx, r->nvtx * 16u) != 0) return "vtx";
        for (k = 0; k < r->nidx; k++) if ((u16)(o->idx[k] - o->vfirst) != (u16)(r->idx[k] - r->vfirst)) return "idx";
    }
    return NULL;
}
static void free_loaded(NdObj* o) { free(o->grp); free(o->gop); free(o->gat); free(o->vtx); free(o->idx); memset(o, 0, sizeof *o); }
static int do_map(const char* nm, int seed) {
    char path[600];
    FILE* f;
    u8 hdr[16];
    u32 size, nrel, k;
    u8* nds;
    NdSeamTab t;
    int seeded = 0;
    snprintf(path, sizeof path, "%s/%s.nds", g_dir, nm);
    f = fopen(path, "rb");
    if (!f || fread(hdr, 1, 16, f) != 16 || memcmp(hdr, "NDSH", 4) != 0) { printf("mapconv FAIL %s: no .nds\n", nm); n_fail++; return 0; }
    size = seam_le32(hdr + 8); nrel = seam_le32(hdr + 12);
    nds = (u8*)malloc(nrel * 4u + size);
    if (!nds || fread(nds, 1, nrel * 4u + size, f) != nrel * 4u + size) { printf("mapconv FAIL %s: short .nds\n", nm); n_fail++; return 0; }
    fclose(f);
    g_size = size;
    g_shape = (u8*)malloc(size + 16u);
    memcpy(g_shape, nds + nrel * 4u, size);
    memset(g_shape + size, 0, 16u);
    for (k = 0; k < nrel; k++) { u8* fl = g_shape + seam_le32(nds + 4u * k); u32 v = seam_le32(fl) + (u32)(uintptr_t)g_shape; memcpy(fl, &v, 4); }
    memset(&t, 0, sizeof t);
    if (nd_seam_build(&t, g_shape, size) != 0) { printf("mapconv FAIL %s: seam\n", nm); n_fail++; return 0; }
    snprintf(g_nm, sizeof g_nm, "%s", nm);
    pm_dlc_gen++;
    snprintf(pm_shape_name, sizeof pm_shape_name, "%s", nm);
    for (k = 0; k < t.nlist; k++) {
        const NdSeamList* sl = &t.list[k];
        const u32 loff = sl->addr - (u32)(uintptr_t)g_shape;
        g_list = loff; g_stage = "words";
        static uint32_t words[2 * (ND_DLC_MAXOPS + 1)];
        unsigned nops, var;
        int noend;
        for (nops = 0; nops < ND_DLC_MAXOPS && loff + 8u * (nops + 1u) <= size; nops++) {
            const u8* p = g_shape + loff + 8u * nops;
            words[2 * nops] = seam_le32(p); words[2 * nops + 1] = seam_le32(p + 4);
            if ((words[2 * nops] >> 24) == NDO_END) break;
        }
        noend = nops >= ND_DLC_MAXOPS || loff + 8u * (nops + 1u) > size;
        for (var = 0; var < 2; var++) {
            NdObj r, o;
            NdDlcEnv env;
            int refuse = 0, rc, pre;
            const char* d;
            if (var == 1 && !sl->cnt) continue;
            env.vtx = ref_vtx; env.valloc = nd_env_valloc; env.ialloc = nd_env_ialloc;
            env.tp = var ? t.tp + sl->first : NULL; env.ntp = var ? sl->cnt : 0u;
            memset(&o, 0, sizeof o);
            g_var = var; g_stage = "compile";
            pool_v = (k * 331u + var * 17u) & 0x3fffu; /* a different first vertex number per list: the rebase is tested */
            rc = noend ? NDR_LONG : nd_dlc_compile(&r, words, &env);
            g_stage = "load";
            if (seed && !seeded && rc == NDR_OK && r.nidx) { /* the seeded fault: one index byte of this section changed in the loaded file */
                pre = dlcp_load(&o, loff, var, 0, &refuse);
                if (pre == 1) {
                    u32 j;
                    free_loaded(&o);
                    for (j = 0; j < dlcp_n; j++) if (dlcp_idx[j].list == loff && dlcp_idx[j].var == var) { dlcp_buf[dlcp_idx[j].off + dlcp_idx[j].size - 1u] ^= 1u; seeded = 1; }
                }
            }
            pre = dlcp_load(&o, loff, var, 0, &refuse);
            n_ok++;
            if (rc != NDR_OK) {
                n_ref++;
                if (pre != 2 || refuse != rc) bad(nm, loff, var, "refusal section missing or wrong code");
                continue;
            }
            if (pre != 1) { bad(nm, loff, var, pre == 0 ? "missing section" : pre == 2 ? "refusal for a compilable list" : "load fault"); nd_dlc_free(&r); continue; }
            g_stage = "diff";
            d = diff(&r, &o, 1);
            if (d) bad(nm, loff, var, d);
            free_loaded(&o);
            nd_dlc_free(&r);
            if (var == 0) { /* the pool full load against the compile without pools and without T points (the device path of NDR_FULL) */
                NdObj r2, o2;
                NdDlcEnv e2 = { ref_vtx, NULL, NULL, NULL, 0 };
                memset(&o2, 0, sizeof o2);
                if (nd_dlc_compile(&r2, words, &e2) != NDR_OK) { bad(nm, loff, var, "env2 compile refused"); continue; }
                g_stage = "nopool";
                pre = dlcp_load(&o2, loff, 0, 1, &refuse);
                if (pre != 1) bad(nm, loff, var, "nopool load");
                else {
                    d = diff(&r2, &o2, 0);
                    if (d) bad(nm, loff, var, d);
                    free(o2.grp); free(o2.gop); free(o2.gat);
                }
                nd_dlc_free(&r2);
            }
        }
    }
    nd_seam_free(&t);
    free(nds); free(g_shape);
    pm_dlc_gen++;
    return seeded;
}
int main(int argc, char** argv) {
    DIR* d;
    struct dirent* de;
    int seed = argc > 2 && strcmp(argv[2], "--seed") == 0, seeded = 0;
    unsigned maps = 0;
    if (argc < 2) return 2;
    g_dir = argv[1];
    signal(SIGSEGV, on_segv);
    d = opendir(g_dir);
    if (!d) { printf("mapconv FAIL: no directory\n"); return 1; }
    while ((de = readdir(d)) != NULL) {
        size_t l = strlen(de->d_name);
        char nm[64];
        if (l < 5 || strcmp(de->d_name + l - 4, ".nds") != 0 || l - 4 >= sizeof nm) continue;
        memcpy(nm, de->d_name, l - 4); nm[l - 4] = 0;
        seeded |= do_map(nm, seed && !seeded);
        maps++;
        if (seed && seeded) break;
    }
    closedir(d);
    if (n_fail) { printf("mapconv FAIL %lu of %lu sections\n", n_fail, n_ok); return 1; }
    if (!maps) { printf("mapconv FAIL: no shapes\n"); return 1; }
    printf("mapconv ok %lu (maps %u refusals %lu)\n", n_ok, maps, n_ref);
    return 0;
}
