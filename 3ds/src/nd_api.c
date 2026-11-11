/* nd_api.c: the pso table of the native draw API (nd_api.h). Core 1 interns a description (nd_pso_n64); core 0 reads it by id (nd_pso_get).
 * An entry is written once and never changes, so a returned id stays valid for the run.
 */
#include <string.h>
#include "nd_api.h"

void pm_log_e(const char* fmt, ...);
void pm_fatal_stop(const char* msg);

static NdPsoN64 pso_tab[ND_PSO_MAX];
static unsigned pso_n, pso_last;

static __attribute__((cold, noinline)) void pso_full(void) {
    pm_log_e("napi: pso table full (%u)", ND_PSO_MAX);
    pm_fatal_stop("napi: pso table full");
}

NdPso nd_pso_n64(const NdPsoN64* d) {
    if (pso_last < pso_n && memcmp(&pso_tab[pso_last], d, sizeof(NdPsoN64)) == 0) return (NdPso)pso_last;
    for (unsigned i = 0; i < pso_n; i++) {
        if (memcmp(&pso_tab[i], d, sizeof(NdPsoN64)) == 0) { pso_last = i; return (NdPso)i; }
    }
    if (pso_n == ND_PSO_MAX) pso_full();
    memcpy(&pso_tab[pso_n], d, sizeof(NdPsoN64));
    pso_last = pso_n;
    return (NdPso)pso_n++;
}

const NdPsoN64* nd_pso_get(NdPso id) {
    return &pso_tab[id];
}
