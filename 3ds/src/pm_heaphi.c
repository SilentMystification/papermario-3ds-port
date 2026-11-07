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

typedef struct { const char* name; HeapNode* head; unsigned used, hi_used, hi_top; } PmHeapHi;
static PmHeapHi hh[4] = {
    { "general", &heap_generalHead, 0, 0, 0 },
    { "sprite", &heap_spriteHead, 0, 0, 0 },
    { "collision", &heap_collisionHead, 0, 0, 0 },
    { "battle", &heap_battleHead, 0, 0, 0 },
};

static PmHeapHi* hi_of(HeapNode* head) {
    for (unsigned i = 0; i < 4; i++) if (hh[i].head == head) return &hh[i];
    return NULL;
}

static void hi_alloc(HeapNode* head, void* p, u32 size) {
    PmHeapHi* h = hi_of(head);
    if (!h || !p) return;
    u32 blk = (size + 15u) & ~15u;
    u32 top = (u32)((u8*)p + blk - (u8*)head);
    h->used += blk;
    if (h->used > h->hi_used) h->hi_used = h->used;
    if (top > h->hi_top) h->hi_top = top;
}

void* __wrap__heap_malloc(HeapNode* head, u32 size) {
    void* p = __real__heap_malloc(head, size);
    hi_alloc(head, p, size);
    return p;
}

void* __wrap__heap_malloc_tail(HeapNode* head, u32 size) {
    void* p = __real__heap_malloc_tail(head, size);
    hi_alloc(head, p, size);
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

void pm_heaphi_line(unsigned frame) {
    for (unsigned i = 0; i < 4; i++) {
        PmHeapHi* h = &hh[i];
        pm_log_d("heaphi f%u %s used %u hi_used %u hi_top %u size %u", frame, h->name, h->used, h->hi_used, h->hi_top,
                 (unsigned)h->head->capacity);
    }
}
