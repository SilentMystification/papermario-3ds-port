/* nd_core_test.c: host test of the S1-2 frame driver (plan_native_slices S1-2, host test "legacy only").
 * A frame of 3 LEGACY records must give 3 walker calls in record order, with the list pointer and the byte count of each. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "nd.h"

/* walker stub: records each call */
static int calls;
static void* call_list[8];
static unsigned call_bytes[8];
void pm_gbi_run(void* dl, unsigned nbytes) {
    call_list[calls] = dl;
    call_bytes[calls] = nbytes;
    calls++;
}

/* phase hooks: the test runs with phase off, so the tick and the record are not read */
int pm_ph_on;
void pm_pf_nd_add(unsigned long long ticks, unsigned legacy) { (void)ticks; (void)legacy; }
uint64_t svcGetSystemTick(void) { return 0; }

int main(void) {
    static uint32_t a[4], b[8], c[2];
    nd_frame_begin();
    nd_bridge_cut(a, 16);
    nd_bridge_cut(b, 32);
    nd_bridge_cut(c, 8);
    nd_emit();
    assert(calls == 3);
    assert(call_list[0] == a && call_bytes[0] == 16);
    assert(call_list[1] == b && call_bytes[1] == 32);
    assert(call_list[2] == c && call_bytes[2] == 8);
    /* a new frame with no records runs nothing */
    calls = 0;
    nd_frame_begin();
    nd_emit();
    assert(calls == 0);
    puts("nd_core legacy only: PASS");
    return 0;
}
