/* nd_drec.c: text writer for NdDrec (see 3ds/include/nd_drec.h). */
#include <string.h>
#include "nd_drec.h"

uint64_t nd_drec_fnv(const void* p, uint32_t n) {
    const unsigned char* b = (const unsigned char*)p;
    uint64_t h = 0xcbf29ce484222325ULL;
    uint32_t i;
    for (i = 0; i < n; i++) {
        h ^= b[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

int nd_drec_in_span(uintptr_t a, const NdDrecSpan* s, uint32_t ns) {
    uint32_t i;
#ifdef ND_DREC_SEED_FAULT
    for (i = 0; i < ns; i++) if (s[i].n && a - s[i].lo <= s[i].n) return 1; /* DRECTEX seeded fault: the end byte counts */
#else
    for (i = 0; i < ns; i++) if (s[i].n && a - s[i].lo < s[i].n) return 1;
#endif
    return 0;
}

void nd_drec_write(FILE* f, const NdDrec* r) {
    uint32_t i, u, k, j;
    uint32_t bits;
    float fv;

    fprintf(f, "draw %u kind %u n %u\n", (unsigned)r->seq, (unsigned)r->kind, (unsigned)r->n);
    for (i = 0; i < r->nregs; i++) {
        fprintf(f, "r %03x %x %08x\n", (unsigned)r->regs[i].reg, (unsigned)r->regs[i].mask, (unsigned)r->regs[i].val);
    }
    for (u = 0; u < 3; u++) {
        if (r->tex[u].n > 0) {
            fprintf(f, "t %u type %08x dim %08x bytes %u h ", (unsigned)u,
                    (unsigned)r->tex_type[u], (unsigned)r->tex_dim[u], (unsigned)r->tex[u].n);
            if (r->tex_gpu[u]) fprintf(f, "gpu\n");
            else fprintf(f, "%016llx\n", (unsigned long long)nd_drec_fnv(r->tex[u].p, r->tex[u].n));
        }
    }
    fprintf(f, "v bytes %u h %016llx\n", (unsigned)r->vtx.n,
            (unsigned long long)nd_drec_fnv(r->vtx.p, r->vtx.n));
    fprintf(f, "i bytes %u h %016llx\n", (unsigned)r->idx.n,
            (unsigned long long)nd_drec_fnv(r->idx.p, r->idx.n));
    for (k = 0; k < r->nmtx; k++) {
        fprintf(f, "m %u", (unsigned)k);
        for (j = 0; j < 16; j++) {
            fv = r->mtx[k][j];
            memcpy(&bits, &fv, sizeof(bits));
            fprintf(f, " %08x", (unsigned)bits);
        }
        fprintf(f, "\n");
    }
    fprintf(f, "end\n");
}
