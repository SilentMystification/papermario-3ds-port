/* GD1: pm_interp_open with the otab/nocc occurrence table (3ds/src/pm_interp.c) against the old full scan of all nodes.
 * Same calls go to the library and to the reference. Per frame the node count and every node id are compared.
 * Prints `iocc frames 5 nodes <sum> capn <x> capd <y> bad <b>`. Exit 1 when bad > 0, capn == 0 or capd == 0.
 * --fault: XOR 1 into the reference id 7 of frame 0 (bad must be above 0). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "pm_port.h"

void pm_interp_close(void);
void pm_interp_stop(void);
unsigned pm_interp_node_id(int i);
int pm_interp_node_n(void);

#define T_MAX_NODES 1024
#define T_MAX_DEPTH 16
#define T_FRAMES 5
#define T_STEPS 4000

PmOpt pm_opt;
int pm_debug_has(const char* word) { return strcmp(word, "fps60") == 0; }
void pm_gpu_kc_cut(void) { }
void pm_interp_drop_keys(void) { }
void pm_log(const char* fmt, ...) { (void)fmt; }
void pm_log_d(const char* fmt, ...) { (void)fmt; }
void pm_log_e(const char* fmt, ...) { (void)fmt; }

/* A copy of hash4 from pm_interp.c. */
static unsigned hash4(unsigned p, unsigned a, unsigned b, unsigned occ) {
    unsigned h = 2166136261u;
    unsigned v[4] = { p, a, b, occ };
    for (int i = 0; i < 4; i++) h = (h ^ v[i]) * 16777619u, h ^= h >> 15;
    return h;
}

/* Reference: the old open and close (pm_interp.c:59-73 before GD1), with a full scan of the nodes. */
typedef struct { unsigned id, parent, a, b; } RNode;
static RNode rn[T_MAX_NODES];
static int rnnode, rdepth, rskip, rrec;
static int rstack[T_MAX_DEPTH];
static unsigned capn, capd;

static void r_start(void) {
    rnnode = 0;
    rdepth = 0;
    rskip = 0;
    rn[0] = (RNode){ hash4(0, 0x67616d65u, 0x6c6f6769u, 0), 0, 0x67616d65u, 0x6c6f6769u };
    rstack[0] = rnnode++;
    rrec = 1;
}

static void r_open(unsigned a, unsigned b) {
    if (!rrec) return;
    if (rnnode >= T_MAX_NODES) { rskip++; capn++; return; }
    if (rdepth >= T_MAX_DEPTH - 1) { rskip++; capd++; return; }
    unsigned parent = rn[rstack[rdepth]].id, occ = 0;
    for (int i = 0; i < rnnode; i++)
        if (rn[i].parent == parent && rn[i].a == a && rn[i].b == b) occ++;
    rn[rnnode] = (RNode){ hash4(parent, a, b, occ), parent, a, b };
    rstack[++rdepth] = rnnode++;
}

static void r_close(void) {
    if (!rrec) return;
    if (rskip) rskip--;
    else if (rdepth > 0) rdepth--;
}

int main(int argc, char** argv) {
    int fault = argc > 1 && strcmp(argv[1], "--fault") == 0;
    unsigned r = 1, nodes = 0, bad = 0;
    for (int f = 0; f < T_FRAMES; f++) {
        pm_interp_start();
        r_start();
        for (int s = 0; s < T_STEPS; s++) {
            r = r * 1103515245u + 12345u;
            unsigned v = r >> 16;
            if (v % 8 < 4) {
                unsigned a = 1 + (v >> 3) % 3, b = (v >> 5) % 4;
                pm_interp_open(a, b);
                r_open(a, b);
            } else {
                pm_interp_close();
                r_close();
            }
        }
        if ((int)pm_interp_node_n() != rnnode) bad++;
        for (int i = 0; i < rnnode; i++) {
            unsigned rid = rn[i].id;
            if (fault && f == 0 && i == 7) rid ^= 1u;
            if (pm_interp_node_id(i) != rid) bad++;
        }
        nodes += (unsigned)rnnode;
        pm_interp_stop();
        rrec = 0;
    }
    printf("iocc frames %d nodes %u capn %u capd %u bad %u\n", T_FRAMES, nodes, capn, capd, bad);
    return (bad > 0 || capn == 0 || capd == 0) ? 1 : 0;
}
