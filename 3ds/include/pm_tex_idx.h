/* pm_tex_idx.h: chained hash index over a table of records (HC9A). pm_tex.c keys the live texture slots by their content key
 * (Slot.shash) so pm_tex_slot_find walks one chain instead of the whole slot table (1024 x 136 B). Links hold record + 1; 0 ends a chain.
 * A record is linked once, under the bucket of its key, and unlinked with the same key. Masks only, no division. */
#ifndef PM_TEX_IDX_H
#define PM_TEX_IDX_H
#include <stdint.h>

static inline unsigned pmidx_bucket(uint32_t key, unsigned mask) { return (key ^ key >> 16) & mask; }

/* add record i (its key already set) at the head of bucket b. The record must not be linked. */
static inline void pmidx_link(unsigned short* head, unsigned short* next, unsigned b, int i) {
    next[i] = head[b];
    head[b] = (unsigned short)(i + 1);
}

/* remove record i from bucket b; a record that is not linked is left alone. */
static inline void pmidx_unlink(unsigned short* head, unsigned short* next, unsigned b, int i) {
    unsigned short* p = &head[b];
    while (*p && *p != i + 1) p = &next[*p - 1];
    if (*p) *p = next[i];
    next[i] = 0;
}

#endif
