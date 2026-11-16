/* LKEY host test: nd_lkey_units, nd_lkey_tab, nd_lkey_end (nd_lkey.h). A moved list key gives its device units (nu, no end unit); a plain list gives itself. */
#include <stdio.h>
#include <stdint.h>
#include "nd_lkey.h"

#define A 0x00100000u
/* the device units of a moved list: VTX, TRI2, NOP (nu 3). The 4th unit stands for the next array in rodata (a far VTX): never read. */
static const uint32_t mu[] = { ND_OP_VTX_W0(4, 0), A, NDO_W0(NDO_TRI2, 0), 0, ND_NOP_A, 0, ND_OP_VTX_W0(8, 0), 0x00be0000u };
static const NdNDev md = { &md, 0u, mu, 3u, 2u };
/* a plain list: VTX, END */
static const uint32_t pl[] = { ND_OP_VTX_W0(3, 0), A, NDO_W0(NDO_END, 0), 0 };

static unsigned vtx_count(const uint32_t* u, unsigned n) { /* the reader form of ent_vtx_range: stop at n or at NDO_END */
    unsigned v = 0;
    for (unsigned k = 0; k < n; k++) {
        if ((u[2u * k] >> 24) == NDO_END) break;
        if ((u[2u * k] >> 24) == NDO_VTX) v++;
    }
    return v;
}

int main(void) {
    unsigned n = 0;
    uint32_t buf[2u * 8u];
    const uint32_t* u = nd_lkey_units(&md, &md, 2048u, &n);
    if (u != mu || n != 3u || vtx_count(u, n) != 1u) { printf("lkey FAIL case 0\n"); return 1; }
    u = nd_lkey_units(pl, 0, 2048u, &n);
    if (u != pl || n != 2048u || vtx_count(u, n) != 1u) { printf("lkey FAIL case 1\n"); return 1; }
    u = nd_lkey_units(&md, &md, 2u, &n);
    if (u != mu || n != 2u) { printf("lkey FAIL case 2\n"); return 1; }
    const NdGTab tm = { (const uint32_t*)(const void*)&md, 0, 0, NDG_F_PARTS | NDG_F_MOVED, 0, 0 };
    const NdGTab tp = { pl, 0, 0, 0, 0, 0 };
    u = nd_lkey_tab(&tm, 4096u, &n);
    if (u != mu || n != 3u) { printf("lkey FAIL case 3\n"); return 1; }
    u = nd_lkey_tab(&tp, 4096u, &n);
    if (u != pl || n != 4096u) { printf("lkey FAIL case 4\n"); return 1; }
    u = nd_lkey_end(&md, &md, buf, 8u);
    if (u != buf || buf[0] != mu[0] || buf[4] != ND_NOP_A || (buf[6] >> 24) != NDO_END || vtx_count(u, 8u) != 1u) { printf("lkey FAIL case 5\n"); return 1; }
    u = nd_lkey_end(pl, 0, buf, 8u);
    if (u != pl) { printf("lkey FAIL case 6\n"); return 1; }
    printf("lkey ok\n");
    return 0;
}
