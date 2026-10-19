/* F06-2: pm_interp_lerp(t) pairs this frame with the previous one (same node id, same leaf count, same leaf index) and builds
 * a table: dest Mtx* -> lerped MtxF (res = (1 - t) * old + t * new). Unpaired leaves are not in the table. pm_gbi.c load_mtx reads it.
 * F06-1: frame interpolation record (PaperBoat method, FrameInterpolation.cpp). Switch `fps60` / pm_opt.fps60, read at the first start.
 * Every guMtxF2L between pm_interp_start and pm_interp_stop adds a leaf (node id, leaf index, dest Mtx*, MtxF).
 * Node id = hash(parent id, a, b, occurrence index). Root node "game_logic". No picture change. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ultra64.h"
#include "pm_port.h"

#define MAX_LEAVES 4096
#define MAX_NODES 1024
#define MAX_DEPTH 16
#define RT_SIZE 8192
#define PT_SIZE 8192
#define NT_SIZE 2048

typedef struct {
    unsigned node, idx;
    int ni;
    Mtx* dest;
    float mf[4][4];
} Leaf;

typedef struct {
    unsigned id, parent, a, b, nleaves;
} Node;

int pm_interp_on = -1;
int pm_interp_repl;
/* The tables sit on the heap and exist only with fps60. A 700 KB BSS growth moved gDisplayContext past 0x01400000, the end of the
 * is_host range in pm_gbi.c: the background list was then "bad dl" and the picture changed with the switch off. */
static Leaf *leaves, *pleaves;
static Node *nodes, *pnodes;
typedef struct { const Mtx* key; int idx; } RT;
static RT* rtab;
static float (*repl)[4][4];
static short* ptab; /* prev leaf index + 1 by hash(node, idx) */
static short* ntab; /* prev node index + 1 by node id */
static int nleaf, nnode, depth, recording, pnleaf, pnnode;
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
    if (!leaves) {
        leaves = malloc(2 * MAX_LEAVES * sizeof(Leaf) + 2 * MAX_NODES * sizeof(Node) + RT_SIZE * sizeof(RT) +
                        MAX_LEAVES * 64 + (PT_SIZE + NT_SIZE) * sizeof(short));
        if (!leaves) { pm_log("f06: no memory for the tables, fps60 off"); pm_interp_on = 0; return; }
        pleaves = leaves + MAX_LEAVES;
        nodes = (Node*)(pleaves + MAX_LEAVES);
        pnodes = nodes + MAX_NODES;
        rtab = (RT*)(pnodes + MAX_NODES);
        repl = (float (*)[4][4])(rtab + RT_SIZE);
        ptab = (short*)(repl + MAX_LEAVES);
        ntab = ptab + PT_SIZE;
    }
    nleaf = 0;
    nnode = 0;
    depth = 0;
    nodes[nnode] = (Node){ hash4(0, 0x67616d65u, 0x6c6f6769u, 0), 0, 0x67616d65u, 0x6c6f6769u, 0 }; /* "game_logic" */
    stack[0] = nnode++;
    recording = 1;
}

/* F06-2: replacement table. Open addressing, keys are the new frame dest pointers. */

const float (*pm_interp_find(const void* m))[4] {
    unsigned h = ((unsigned)m >> 3) * 2654435761u >> 19;
    for (int i = 0; i < RT_SIZE; i++, h = (h + 1) & (RT_SIZE - 1)) {
        if (rtab[h].key == m) return repl[rtab[h].idx];
        if (!rtab[h].key) return 0;
    }
    return 0;
}

void pm_interp_lerp(float t) {
    int paired = 0;
    memset(rtab, 0, RT_SIZE * sizeof(RT));
    if (!recording || !pnleaf) return;
    memset(ptab, 0, PT_SIZE * sizeof(short));
    memset(ntab, 0, NT_SIZE * sizeof(short));
    for (int i = 0; i < pnnode; i++) {
        unsigned h = pnodes[i].id & (NT_SIZE - 1);
        while (ntab[h]) h = (h + 1) & (NT_SIZE - 1);
        ntab[h] = (short)(i + 1);
    }
    for (int i = 0; i < pnleaf; i++) {
        unsigned h = hash4(pleaves[i].node, pleaves[i].idx, 0, 0) & (PT_SIZE - 1);
        while (ptab[h]) h = (h + 1) & (PT_SIZE - 1);
        ptab[h] = (short)(i + 1);
    }
    for (int i = 0; i < nleaf; i++) {
        Leaf* l = &leaves[i];
        int ok = 0, j;
        unsigned h = l->node & (NT_SIZE - 1);
        for (; ntab[h]; h = (h + 1) & (NT_SIZE - 1))
            if (pnodes[ntab[h] - 1].id == l->node) { ok = pnodes[ntab[h] - 1].nleaves == nodes[l->ni].nleaves; break; }
        if (!ok) continue;
        h = hash4(l->node, l->idx, 0, 0) & (PT_SIZE - 1);
        for (; ptab[h]; h = (h + 1) & (PT_SIZE - 1)) {
            Leaf* o = &pleaves[ptab[h] - 1];
            if (o->node != l->node || o->idx != l->idx) continue;
            float (*r)[4] = repl[paired];
            for (j = 0; j < 16; j++) r[j >> 2][j & 3] = (1.f - t) * o->mf[j >> 2][j & 3] + t * l->mf[j >> 2][j & 3];
            unsigned k = ((unsigned)l->dest >> 3) * 2654435761u >> 19;
            while (rtab[k].key && rtab[k].key != l->dest) k = (k + 1) & (RT_SIZE - 1);
            if (!rtab[k].key) paired++; /* a dest written twice keeps one slot, the later leaf wins */
            else { memcpy(repl[rtab[k].idx], r, sizeof(repl[0])); break; }
            rtab[k].key = l->dest;
            rtab[k].idx = paired - 1;
            break;
        }
    }
}

void pm_interp_stop(void) {
    if (!recording) return;
    recording = 0;
    memcpy(pleaves, leaves, nleaf * sizeof(Leaf));
    memcpy(pnodes, nodes, nnode * sizeof(Node));
    pnleaf = nleaf;
    pnnode = nnode;
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
    l->ni = stack[depth];
    l->idx = n->nleaves++;
    l->dest = (Mtx*)m;
    memcpy(l->mf, mf, sizeof(l->mf));
}
