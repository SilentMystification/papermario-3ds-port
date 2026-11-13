/* nd_fhash_test.c: host test of nd_fhash.c (FRAMEHASH1). Prints "fhash ok 6/6" or "fhash FAIL <case> <hash>" and exits 1. */
#include <stdio.h>
#include <stdint.h>
#include "nd_fhash.h"

static int fails;

static void check_words(const char* name, const uint32_t* w, uint32_t n, uint64_t want_h, int want_ret) {
    uint64_t h = ND_FH_INIT;
    int ret = nd_fh_words(&h, w, n);
    if (h != want_h || ret != want_ret) {
        printf("fhash FAIL %s %016llx\n", name, (unsigned long long)h);
        fails++;
    }
}

int main(void) {
    static const uint32_t A[12] = { 0x00012340, 0x000F0085, 0x00400040, 0x801F0082, 0x00000006, 0x00000000,
                                    0x80001000, 0x000F0227, 0x00000040, 0x000F0203, 0x00000001, 0x000F022F };
    uint32_t B[12], C[12], C2[12];
    uint64_t h;
    unsigned i;
    for (i = 0; i < 12; i++) { B[i] = A[i]; C[i] = A[i]; C2[i] = A[i]; }
    B[0] = 0x00055550; B[6] = 0x80002000; B[8] = 0x00000080; /* only masked bits differ */
    C[4] = 0x00000007;
    C2[6] = 0x00001000;

    check_words("A", A, 12, 0xe1f5449bd8d7a601ull, 1);
    check_words("B", B, 12, 0xe1f5449bd8d7a601ull, 1);
    check_words("C", C, 12, 0x821500b103d744e2ull, 1);
    check_words("C2", C2, 12, 0xef582e03fd998881ull, 1);
    check_words("A11", A, 11, 0, 0);

    h = nd_fh_bytes(ND_FH_INIT, "abc", 3);
    if (h != 0xe71fa2190541574bull) {
        printf("fhash FAIL bytes %016llx\n", (unsigned long long)h);
        fails++;
    }

    if (fails) return 1;
    printf("fhash ok 6/6\n");
    return 0;
}
