/* nd_api_test.c: host test of the native draw pso table (nd_api.c). */
#include <stdio.h>
#include <stdlib.h>
#include "nd_api.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL nd_api line %d\n", __LINE__); fails++; } } while (0)

void pm_log_e(const char* fmt, ...) { (void)fmt; }
void pm_fatal_stop(const char* msg) { printf("FAIL nd_api fatal %s\n", msg); exit(1); }

int main(void) {
    NdPsoN64 a = { 0xfc121824u, 0xff33fffeu, 0x00504340u, 0u, 0x3000u };
    NdPsoN64 b = a;
    b.oml = 0x00552048u;
    NdPsoN64 c;
    unsigned i;
    CHECK(nd_pso_n64(&a) == 0);
    CHECK(nd_pso_n64(&b) == 1);
    CHECK(nd_pso_n64(&a) == 0);
    CHECK(nd_pso_get(1)->oml == 0x00552048u);
    for (i = 2; i < ND_PSO_MAX; i++) {
        c = a;
        c.cw1 = i;
        CHECK(nd_pso_n64(&c) == i);
    }
    c = a;
    c.cw1 = 7;
    CHECK(nd_pso_n64(&c) == 7);
    CHECK(nd_pso_n64(&b) == 1);
    if (fails) return 1;
    printf("nd_api ok\n");
    return 0;
}
