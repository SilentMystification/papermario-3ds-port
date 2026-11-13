#include "nd_api.h"
#include <stdio.h>
#include <stddef.h>

_Static_assert(sizeof(NdPsoDesc) == 44, "NdPsoDesc 44 bytes");
_Static_assert(offsetof(NdPsoDesc, tex_on) == 32, "u8 fields at 32");

ND_PSO_DESC(t_desc, .cw0 = 0xFC121824u, .cw1 = 0xFF33FFFFu, .oml = 0x00552048u, .geom = 0x00220405u, .tex_on = 1);

int main(void)
{
    printf("psodesc ok size %u cw0 %08x oml %08x tex_on %u\n", (unsigned)sizeof(NdPsoDesc), (unsigned)t_desc.cw0,
           (unsigned)t_desc.oml, (unsigned)t_desc.tex_on);
    return 0;
}
