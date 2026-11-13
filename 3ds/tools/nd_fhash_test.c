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

/* FRAMEHASH2: nd_fh_frame over a 64 KB fixture at physical 0x20000000 (pa outside: NULL). Target role 0 for the colour buffers 0x20008000 and 0x20009000. */
static uint8_t mem[0x10000];
static int fx_ok;

static const void* fx_map(uint32_t pa, uint32_t n, void* ctx) {
    (void)ctx;
    if (pa < 0x20000000u || (uint64_t)pa + n > 0x20010000ull) return NULL;
    return mem + (pa - 0x20000000u);
}

static uint32_t fx_role(uint32_t pa, void* ctx) {
    (void)ctx;
    return (pa == 0x20008000u || pa == 0x20009000u) ? 0u : 1u;
}

static void fx_fill(void) {
    unsigned i;
    for (i = 0; i < 0x10000u; i++) mem[i] = (uint8_t)(i * 7u + 3u);
    mem[0x400] = 0; mem[0x401] = 0; mem[0x402] = 1; mem[0x403] = 0; mem[0x404] = 2; mem[0x405] = 0; /* u16 indices {0, 1, 2} */
}

/* the draw as 11 writes (2 words each): base, buffer 0 offset and config, index config, count, unit 0 on, size, format, colour, draw */
static void fx_words(uint32_t base, uint32_t colour, uint32_t tex, uint32_t* w) {
    /* value, register: CONFIG2 = component count 3 (bits 28-31) and stride 16 (bits 16-23); index config = u16, offset 0x400 */
    const uint32_t wr[11][2] = {
        { base >> 3, 0x200 }, { 0x00000100u, 0x203 }, { 0x30100000u, 0x205 }, { 0x80000400u, 0x227 },
        { 0x00000003u, 0x228 }, { 0x00000001u, 0x080 }, { 0x00080008u, 0x082 }, { tex >> 3, 0x085 },
        { 0x00000000u, 0x08E }, { colour >> 3, 0x11D }, { 0x00000000u, 0x22F },
    };
    unsigned k;
    for (k = 0; k < 11; k++) {
        w[2 * k] = wr[k][0];
        w[2 * k + 1] = 0x000F0000u | wr[k][1];
    }
}

static uint64_t fx_run(const uint32_t* w) {
    NdFhEnv env = { fx_map, fx_role, NULL };
    uint64_t h = ND_FH_INIT;
    fx_ok = nd_fh_frame(&h, w, 22, &env);
    return h;
}

#define FH_EXPECT(k, cond) do { if (!(cond)) { printf("fhframe FAIL S%d\n", (k)); return 1; } } while (0)

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
    h = ND_FH_INIT; /* a partial walk: the return value only (the row gives no hash) */
    if (nd_fh_words(&h, A, 11) != 0) {
        printf("fhash FAIL A11 %016llx\n", (unsigned long long)h);
        fails++;
    }

    h = nd_fh_bytes(ND_FH_INIT, "abc", 3);
    if (h != 0xe71fa2190541574bull) {
        printf("fhash FAIL bytes %016llx\n", (unsigned long long)h);
        fails++;
    }

    {
        uint32_t w[22], w5[22], w6[22];
        uint64_t base, again, x;
        unsigned k;
        fx_words(0x20000000u, 0x20008000u, 0x20001000u, w);
        fx_words(0x20000000u, 0x20009000u, 0x20001000u, w5); /* S5: colour buffer 0x20009000 */
        fx_words(0x20002000u, 0x20008000u, 0x20003000u, w6); /* S6: base and texel moved by +0x2000 */

        fx_fill(); /* S1: same words and bytes, second run equal */
        base = fx_run(w);
        FH_EXPECT(1, fx_ok);
        again = fx_run(w);
        FH_EXPECT(1, fx_ok && again == base);

        fx_fill(); /* S2: vertex 1 byte at 0x20000110 changed: differs */
        mem[0x110] ^= 0xFFu;
        x = fx_run(w);
        FH_EXPECT(2, fx_ok && x != base);

        fx_fill(); /* S3: vertex 4 (not drawn) byte at 0x20000140 changed: equal */
        mem[0x140] ^= 0xFFu;
        x = fx_run(w);
        FH_EXPECT(3, fx_ok && x == base);

        fx_fill(); /* S4: texel byte at 0x20001004 changed: differs */
        mem[0x1004] ^= 0xFFu;
        x = fx_run(w);
        FH_EXPECT(4, fx_ok && x != base);

        fx_fill(); /* S5: colour buffer 0x20009000, role 0: equal */
        x = fx_run(w5);
        FH_EXPECT(5, fx_ok && x == base);

        fx_fill(); /* S6: the vertex, index and texel bytes copied to +0x2000 (vertex 0x2100, index 0x2400, texel 0x3000) */
        for (k = 0; k < 0x30u; k++) mem[0x2100u + k] = mem[0x100u + k];
        for (k = 0; k < 6u; k++) mem[0x2400u + k] = mem[0x400u + k];
        for (k = 0; k < 0x100u; k++) mem[0x3000u + k] = mem[0x1000u + k];
        x = fx_run(w6);
        FH_EXPECT(6, fx_ok && x == base);
        printf("fhframe ok 6/6\n");
    }

    if (fails) return 1;
    printf("fhash ok 6/6\n");
    return 0;
}
