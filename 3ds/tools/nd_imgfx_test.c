/* PIPEFX host test: nd_imgfx_type (nd_imgfx.h) types the imgfx anim list in place (imgfx.c imgfx_load_anim). The seed (no ND_X0) must fail case 0. */
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "nd_imgfx.h"

static uint32_t g[8];

int main(void) {
    unsigned n;
    g[0] = (0x01u << 24) | (4u << 12) | (4u << 1);
    g[1] = 0x100 + 24;
    g[2] = (0x05u << 24) | 0x000204u;
    g[3] = 0;
    g[4] = 0xDFu << 24;
    g[5] = 0;
    g[6] = NDO_W0(NDO_VTX, 0);
    g[7] = 0x1234;
    n = nd_imgfx_type(g, 4, 0x100, 0x8000);
    if ((g[0] >> 24) != NDO_VTX) { printf("imgfx FAIL case 0\n"); return 1; }
    if (g[1] != 0x8020u) { printf("imgfx FAIL case 1\n"); return 1; }
    if ((g[2] >> 24) != NDO_TRI1) { printf("imgfx FAIL case 2\n"); return 1; }
    if ((g[4] >> 24) != NDO_END) { printf("imgfx FAIL case 3\n"); return 1; }
    if (n != 3u || g[6] != NDO_W0(NDO_VTX, 0) || g[7] != 0x1234u) { printf("imgfx FAIL case 4\n"); return 1; }
    printf("imgfx ok\n");
    return 0;
}
