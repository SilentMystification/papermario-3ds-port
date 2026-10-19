/* F06-1: frame interpolation record (PaperBoat method, FrameInterpolation.cpp). Switch `fps60` / pm_opt.fps60, read at the first start.
 * Every guMtxF2L between pm_interp_start and pm_interp_stop adds a leaf (node id, leaf index, dest Mtx*, MtxF).
 * Node id = hash(parent id, a, b, occurrence index). Root node "game_logic". No picture change. */
#include <stdio.h>
#include <string.h>
#include "ultra64.h"
#include "pm_port.h"

#define MAX_LEAVES 4096
#define MAX_NODES 1024
#define MAX_DEPTH 16

typedef struct {
    unsigned node, idx;
    Mtx* dest;
    float mf[4][4];
} Leaf;

typedef struct {
    unsigned id, parent, a, b, nleaves;
} Node;

int pm_interp_on = -1;
static Leaf leaves[MAX_LEAVES];
static Node nodes[MAX_NODES];
static int nleaf, nnode, depth, recording;
static int stack[MAX_DEPTH];
static unsigned frames, tot_leaves, tot_nodes, peak, peak_nodes;

static unsigned hash4(unsigned p, unsigned a, unsigned b, unsigned occ) {
    unsigned h = 2166136261u;
    unsigned v[4] = { p, a, b, occ };
    for (int i = 0; i < 4; i++) h = (h ^ v[i]) * 16777619u, h ^= h >> 15;
    return h;
}

void pm_interp_open(unsigned a, unsigned b) {
    if (!recording || nnode >= MAX_NODES || depth >= MAX_DEPTH - 1) return;
    unsigned parent = nodes[stack[depth]].id, occ = 0;
    for (int i = 0; i < nnode; i++)
        if (nodes[i].parent == parent && nodes[i].a == a && nodes[i].b == b) occ++;
    nodes[nnode] = (Node){ hash4(parent, a, b, occ), parent, a, b, 0 };
    stack[++depth] = nnode++;
}

void pm_interp_close(void) {
    if (recording && depth > 0) depth--;
}

void pm_interp_start(void) {
    if (pm_interp_on < 0) pm_interp_on = pm_debug_has("fps60") || pm_opt.fps60;
    if (!pm_interp_on) return;
    nleaf = 0;
    nnode = 0;
    depth = 0;
    nodes[nnode] = (Node){ hash4(0, 0x67616d65u, 0x6c6f6769u, 0), 0, 0x67616d65u, 0x6c6f6769u, 0 }; /* "game_logic" */
    stack[0] = nnode++;
    recording = 1;
}

void pm_interp_stop(void) {
    if (!recording) return;
    recording = 0;
    tot_leaves += nleaf;
    tot_nodes += nnode;
    if ((unsigned)nleaf > peak) peak = nleaf;
    if ((unsigned)nnode > peak_nodes) peak_nodes = nnode;
    if (++frames % 600 == 0) {
        pm_log("f06 rec leaves=%u nodes=%u peak=%u (table %d) peak_nodes=%u", tot_leaves / 600, tot_nodes / 600, peak, MAX_LEAVES, peak_nodes);
        tot_leaves = tot_nodes = 0;
    }
}

void pm_interp_record(float mf[4][4], void* m) {
    if (!recording || nleaf >= MAX_LEAVES) return;
    Node* n = &nodes[stack[depth]];
    Leaf* l = &leaves[nleaf++];
    l->node = n->id;
    l->idx = n->nleaves++;
    l->dest = (Mtx*)m;
    memcpy(l->mf, mf, sizeof(l->mf));
}
