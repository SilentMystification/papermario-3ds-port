/* MEM-B v3: heap high-water. The N64 first-fit heaps (src/43F0.c) are re-created with _heap_create at map and battle load.
 * A link wrap of _heap_create (CMake --wrap) walks the old list of that head (the last values of the instance), keeps the
 * process maxima, and resets the instance counters. pm_heaphi_line walks every head every 60 frames (cold, print time only):
 *   used  = sum over allocated nodes of (length + sizeof(HeapNode))
 *   top   = max over allocated nodes of (node end - head), the extent the heap needs
 *   free  = largest free block
 * inst_* is this instance (reset at re-create), proc_* is the process maximum. Sizing uses proc_top. */
#include <stdio.h>
#include "pm_port.h"
#include "common_structs.h"

extern HeapNode heap_generalHead, heap_spriteHead, heap_collisionHead, heap_battleHead;
HeapNode* __real__heap_create(HeapNode* addr, u32 size);
unsigned pm_tex_used(void);
unsigned pm_tex_peak(void);
unsigned pm_tex_pool_peak(void);
void pm_gpu_gv_stats(unsigned o[4]);

typedef struct { const char* name; HeapNode* head; u32 cur_used, cur_top, cur_free; u32 inst_used, inst_top, inst_free; u32 proc_used, proc_top, proc_free; } PmHeapHi;
static PmHeapHi hh[4] = {
    { "general", &heap_generalHead },
    { "sprite", &heap_spriteHead },
    { "collision", &heap_collisionHead },
    { "battle", &heap_battleHead },
};

static void heap_walk(PmHeapHi* h, u32* used, u32* top, u32* freemax) {
    u32 u = 0, t = 0, f = 0;
    HeapNode* base = h->head;
    for (HeapNode* n = base; n; n = n->next) {
        u32 blk = n->length + sizeof(HeapNode);
        if (n->allocated) {
            u += blk;
            u32 e = (u32)((u8*)n + blk - (u8*)base);
            if (e > t) t = e;
        } else if (n->length > f) f = n->length;
    }
    *used = u; *top = t; *freemax = f;
}

static void heap_note(PmHeapHi* h) {
    u32 u, t, f;
    heap_walk(h, &u, &t, &f);
    h->cur_used = u; h->cur_top = t; h->cur_free = f;
    if (u > h->inst_used) h->inst_used = u;
    if (t > h->inst_top) h->inst_top = t;
    if (f > h->inst_free) h->inst_free = f;
    if (u > h->proc_used) h->proc_used = u;
    if (t > h->proc_top) h->proc_top = t;
    if (f > h->proc_free) h->proc_free = f;
}

/* Wrap of _heap_create: a re-create of a known head walks the old instance first, then resets its counters. */
HeapNode* __wrap__heap_create(HeapNode* addr, u32 size) {
    for (unsigned i = 0; i < 4; i++) {
        PmHeapHi* h = &hh[i];
        if ((u8*)h->head == (u8*)addr && h->head->capacity) heap_note(h); /* the old instance: last walk */
    }
    HeapNode* r = __real__heap_create(addr, size);
    for (unsigned i = 0; i < 4; i++) {
        PmHeapHi* h = &hh[i];
        if ((u8*)h->head == (u8*)addr) { h->cur_used = h->cur_top = h->cur_free = 0; h->inst_used = h->inst_top = h->inst_free = 0; }
    }
    return r;
}

void pm_heaphi_line(unsigned frame) {
    for (unsigned i = 0; i < 4; i++) {
        PmHeapHi* h = &hh[i];
        if (!h->head->capacity) continue; /* not created in this scene */
        heap_note(h);
        pm_log_d("heaphi f%u %s used %u top %u free %u inst_top %u proc_used %u proc_top %u proc_free %u size %u", frame, h->name,
                 h->cur_used, h->cur_top, h->cur_free, h->inst_top, h->proc_used, h->proc_top, h->proc_free, (unsigned)h->head->capacity);
    }
    /* poolhi: the existing high marks of the port pools (tex slot peak and bytes, gv palette peak). */
    unsigned gv[4] = { 0, 0, 0, 0 };
    pm_gpu_gv_stats(gv);
    pm_log_d("poolhi f%u tex_used %u tex_peak %u tex_slots %u gv_palup %u gv_palmax %u", frame, pm_tex_used(), pm_tex_peak(),
             pm_tex_pool_peak(), gv[0], gv[1]);
}
