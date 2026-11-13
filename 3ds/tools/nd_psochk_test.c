/* NPSO4: nd_psochk_cmp on three cases. Prints "psochk ok 3/3" or "psochk FAIL <case>", exit 1 on a fail. */
#include <stdio.h>
#include <string.h>
#include "nd_psochk.h"

static void base_desc(NdPsoDesc* d) {
    memset(d, 0, sizeof *d);
    d->cw0 = 0xFC121824u; d->cw1 = 0xFF33FFFFu; d->omh = 0x00000000u; d->oml = 0x00552048u; d->geom = 0x00220405u;
}

int main(void) {
    NdPsoDesc s, w; char out[96];
    int r, ok = 0;

    base_desc(&s); base_desc(&w);
    w.oml = 0x00552078u;
    r = nd_psochk_cmp(&s, &w, out, sizeof out);
    if (r == 1 && strcmp(out, "oml stated 00552048 walker 00552078") == 0) ok++;
    else { printf("psochk FAIL oml (r %d out '%s')\n", r, out); return 1; }

    base_desc(&s); base_desc(&w);
    r = nd_psochk_cmp(&s, &w, out, sizeof out);
    if (r == 0) ok++;
    else { printf("psochk FAIL equal (r %d out '%s')\n", r, out); return 1; }

    base_desc(&s); base_desc(&w);
    w.tex_on = 1; w.plod = 0x10;
    r = nd_psochk_cmp(&s, &w, out, sizeof out);
    if (r == 2 && strcmp(out, "tex_on stated 00 walker 01") == 0) ok++;
    else { printf("psochk FAIL tex_on (r %d out '%s')\n", r, out); return 1; }

    printf("psochk ok %d/3\n", ok);
    return 0;
}
