/* pm_tex_idx_test.c: host test of the slot content index (pm_tex_idx.h, used by pm_tex.c pm_tex_slot_find, HC9A).
 * A table of 1024 records (the SLOTS of pm_tex.c) with load, evict and reinsert over more than 1024 loads. After every step the index
 * lookup (chain walk, the rule of pm_tex_slot_find) must equal a reference scan in slot order for every key in the pool, and the chains
 * must hold exactly the live records. Usage: pm_tex_idx_test (no arguments). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "pm_tex_idx.h"

#define SLOTS 1024
#define SH_N 2048
#define KEYS 300
#define STEPS 60000

typedef struct { uint32_t key; unsigned stamp; int live; } Rec;
static Rec rec[SLOTS];
static unsigned short head[SH_N], next[SLOTS];
static uint32_t pool[KEYS];
static unsigned stamp_ctr;
static int fails;
static unsigned loads, evicts, reinserts, lookups;

/* the accept rule of pm_tex_slot_find (live, key, and the clean and dim checks, here a fixed pattern) */
static int accept(int i, uint32_t key) { return rec[i].live && rec[i].key == key && (i % 5) != 1; }

static int find_index(uint32_t key) {
    int best = -1; unsigned best_stamp = 0;
    if (!key) return -1;
    for (int i = head[pmidx_bucket(key, SH_N - 1u)] - 1; i >= 0; i = next[i] - 1) {
        if (!accept(i, key)) continue;
        if (best >= 0 && (rec[i].stamp < best_stamp || (rec[i].stamp == best_stamp && i < best))) continue;
        best = i; best_stamp = rec[i].stamp;
    }
    return best;
}

static int find_ref(uint32_t key) {
    int best = -1; unsigned best_stamp = 0;
    if (!key) return -1;
    for (int i = 0; i < SLOTS; i++) {
        if (!accept(i, key)) continue;
        if (best >= 0 && rec[i].stamp < best_stamp) continue;
        best = i; best_stamp = rec[i].stamp;
    }
    return best;
}

static void check_all(const char* what, unsigned step) {
    int live = 0, linked = 0;
    for (int i = 0; i < SLOTS; i++) live += rec[i].live;
    for (int b = 0; b < SH_N; b++)
        for (int i = head[b] - 1; i >= 0; i = next[i] - 1) {
            linked++;
            if (!rec[i].live || pmidx_bucket(rec[i].key, SH_N - 1u) != (unsigned)b) { if (fails < 20) printf("FAIL step %u %s: chain %d holds record %d live %d\n", step, what, b, i, rec[i].live); fails++; }
            if (linked > SLOTS) { printf("FAIL step %u %s: chain loop\n", step, what); fails++; return; }
        }
    if (linked != live) { if (fails < 20) printf("FAIL step %u %s: linked %d live %d\n", step, what, linked, live); fails++; }
    for (int k = 0; k < KEYS; k++) {
        lookups++;
        int a = find_index(pool[k]), r = find_ref(pool[k]);
        if (a != r) { if (fails < 20) printf("FAIL step %u %s: key %08x index %d ref %d\n", step, what, pool[k], a, r); fails++; }
    }
}

static void drop(int i) {
    pmidx_unlink(head, next, pmidx_bucket(rec[i].key, SH_N - 1u), i);
    rec[i].live = 0;
    evicts++;
}

static void load(int i, uint32_t key) {
    static unsigned used[SLOTS];
    if (used[i]++) reinserts++; /* the slot held a texture before: a reinsert after an evict */
    rec[i].key = key; rec[i].stamp = ++stamp_ctr; rec[i].live = 1;
    pmidx_link(head, next, pmidx_bucket(key, SH_N - 1u), i);
    loads++;
}

int main(void) {
    srand(12345);
    /* keys: a pool where some keys differ only in the high bits, some collide in the bucket, some repeat */
    for (int k = 0; k < KEYS; k++) pool[k] = k % 7 == 0 ? (uint32_t)k * 0x10001u : (uint32_t)rand() * 2654435761u | 1u;
    for (int k = 0; k < 9; k++) pool[KEYS - 1 - k] = pool[k * 3]; /* repeated keys: equal copies in one chain */
    for (unsigned step = 0; step < STEPS; step++) {
        int slot = -1;
        for (int i = 0; i < SLOTS; i++) if (!rec[i].live) { slot = i; break; }
        int r = rand() % 10;
        if (slot >= 0 && (r < 6 || r == 9)) {
            load(slot, pool[rand() % KEYS]);
        } else {
            int i = rand() % SLOTS; /* evict a random live slot and load the same index again */
            if (rec[i].live) drop(i);
            load(i, pool[rand() % KEYS]);
        }
        if (rand() % 4 == 0) { /* a use: the hit stamps the record */
            int i = rand() % SLOTS;
            if (rec[i].live) rec[i].stamp = ++stamp_ctr;
        }
        if (step % 97 == 0 || step > STEPS - 50) check_all("step", step);
    }
    /* full refill: 2048 loads into a table of 1024, so every slot is evicted and reloaded at least once */
    for (unsigned n = 0; n < 2 * SLOTS; n++) {
        int i = (int)(n % SLOTS);
        if (rec[i].live) drop(i);
        load(i, pool[n % KEYS]);
        check_all("refill", n);
    }
    printf("pm_tex_idx_test: loads %u evicts %u reinserts %u lookups %u live %d\n", loads, evicts, reinserts, lookups, 0);
    if (loads <= SLOTS) { printf("FAIL: fewer than 1024 loads\n"); fails++; }
    printf(fails ? "PM_TEX_IDX TEST FAIL (%d)\n" : "PM_TEX_IDX TEST PASS\n", fails);
    return fails != 0;
}
