/* MEM-B step 1: heap high-water marks. The N64 first-fit heaps (src/43F0.c) are wrapped at link time
 * (CMake --wrap of _heap_malloc, _heap_malloc_tail, _heap_free). Per head: used bytes (block size on malloc,
 * header length on free), hi_used, and hi_top (the highest block end above the head, the fragmentation-relevant
 * number). pm_heaphi_line prints one heaphi line per heap every 300 frames (pm_main.c). Cost: a few adds per call. */
#include <stdio.h>
#include "pm_port.h"
#include "common_structs.h"

extern HeapNode heap_generalHead, heap_spriteHead, heap_collisionHead, heap_battleHead;
void* __real__heap_malloc(HeapNode* head, u32 size);
void* __real__heap_malloc_tail(HeapNode* head, u32 size);
u32 __real__heap_free(HeapNode* heapNodeList, void* addrToFree);

typedef struct { const char* name; HeapNode* head; unsigned used, hi_used, hi_top, hi_tail, hi_top_max; } PmHeapHi;
static PmHeapHi hh[4] = {
    { "general", &heap_generalHead, 0, 0, 0, 0, 0 },
    { "sprite", &heap_spriteHead, 0, 0, 0, 0, 0 },
    { "collision", &heap_collisionHead, 0, 0, 0, 0, 0 },
    { "battle", &heap_battleHead, 0, 0, 0, 0, 0 },
};

static PmHeapHi* hi_of(HeapNode* head) {
    for (unsigned i = 0; i < 4; i++) if (hh[i].head == head) return &hh[i];
    return NULL;
}

static void hi_alloc(HeapNode* head, void* p, u32 size, int tail) {
    PmHeapHi* h = hi_of(head);
    if (!h || !p) return;
    u32 blk = (size + 15u) & ~15u;
    u32 top = (u32)((u8*)p + blk - (u8*)head);
    h->used += blk;
    if (h->used > h->hi_used) h->hi_used = h->used;
    if (top > h->hi_top) h->hi_top = top;
    /* MEM-B v2: a tail allocation is placed at the end; its extent is the distance from the end, the sprite heap sizing number */
    if (tail && p) { u32 ext = h->head->capacity - (u32)((u8*)p - (u8*)head); if (ext > h->hi_tail) h->hi_tail = ext; }
}

void* __wrap__heap_malloc(HeapNode* head, u32 size) {
    void* p = __real__heap_malloc(head, size);
    hi_alloc(head, p, size, 0);
    return p;
}

void* __wrap__heap_malloc_tail(HeapNode* head, u32 size) {
    void* p = __real__heap_malloc_tail(head, size);
    hi_alloc(head, p, size, 1);
    return p;
}

u32 __wrap__heap_free(HeapNode* head, void* data) {
    PmHeapHi* h = hi_of(head);
    HeapNode* hdr = data ? (HeapNode*)data - 1 : NULL;
    u32 len = 0, was = 0;
    if (h && hdr) { was = hdr->allocated; len = hdr->length; }
    u32 r = __real__heap_free(head, data);
    if (h && was && h->used >= len) h->used -= len;
    return r;
}

unsigned pm_tex_used(void);
unsigned pm_tex_peak(void);
unsigned pm_tex_pool_peak(void); /* pm_tex.c: highest live slot count */
void pm_gpu_gv_stats(unsigned o[4]);

/* The live bytes of a heap: walk the node list (allocated blocks' lengths). The wrapper counter misses _heap_realloc
 * and split leftovers, so this walk is the used figure. Walks at print time only (every 60 frames). */
static u32 heap_live(HeapNode* head, u32* ext) {
    u32 sum = 0, top = 0;
    for (HeapNode* n = head; n; n = n->next) {
        if (!n->allocated) continue;
        sum += n->length;
        u32 e = (u32)((u8*)n + sizeof(HeapNode) + n->length - (u8*)head);
        if (e > top) top = e;
    }
    *ext = top;
    return sum;
}

void pm_heaphi_line(unsigned frame) {
    for (unsigned i = 0; i < 4; i++) {
        PmHeapHi* h = &hh[i];
        if (!h->head->capacity) continue; /* not created in this scene */
        u32 ext = 0, live = heap_live(h->head, &ext);
        h->used = live;
        if (live > h->hi_used) h->hi_used = live;
        h->hi_top = ext; /* the extent of the live blocks now (walk): the number that sizes a heap, not the wrapper's */
        h->hi_top_max = ext > h->hi_top_max ? ext : h->hi_top_max;
        pm_log_d("heaphi f%u %s used %u hi_used %u hi_top %u hi_top_max %u hi_tail %u size %u", frame, h->name, h->used, h->hi_used, h->hi_top,
                 h->hi_top_max, h->hi_tail, (unsigned)h->head->capacity);
    }
    /* MEM-B v2 poolhi: the existing high marks of the port pools (tex slot peak and bytes, gv palette peak). */
    unsigned gv[4] = { 0, 0, 0, 0 };
    pm_gpu_gv_stats(gv);
    pm_log_d("poolhi f%u tex_used %u tex_peak %u tex_slots %u gv_palup %u gv_palmax %u", frame, pm_tex_used(), pm_tex_peak(),
             pm_tex_pool_peak(), gv[0], gv[1]);
}
