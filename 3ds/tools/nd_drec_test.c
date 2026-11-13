/* nd_drec_test.c: host check of the draw record text format (nd_drec.h / nd_drec.c). */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "nd_drec.h"

static const char expect[] =
    "draw 0 kind 1 n 3\n"
    "r 0c0 f 000f0f0f\n"
    "r 104 3 00000001\n"
    "t 0 type 0000000d dim 00040004 bytes 16 h 7c84dc9477851775\n"
    "v bytes 48 h b9709bed602225a5\n"
    "i bytes 6 h 37f11cf1d36d7afe\n"
    "m 0 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000\n"
    "end\n"
    "draw 1 kind 1 n 3\n"
    "r 0c0 f 000f0f0f\n"
    "r 104 3 00000001\n"
    "t 0 type 0000000d dim 00040004 bytes 16 h 7c84dc9477851775\n"
    "v bytes 48 h 9bc4836d4ef03135\n"
    "i bytes 6 h 37f11cf1d36d7afe\n"
    "m 0 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000 00000000 00000000 00000000 00000000 3f800000\n"
    "end\n";

static NdDrecReg regs[2] = {
    { 0x0C0, 0x0F, 0x000F0F0F },
    { 0x104, 0x03, 0x00000001 },
};

static uint8_t tex0[16];
static uint8_t vtx[48];
static uint16_t idx[3] = { 0, 1, 2 };
static float ident[1][16] = {
    { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
};

static void build(NdDrec* r, uint32_t seq, const uint8_t* v) {
    memset(r, 0, sizeof(*r));
    r->seq = seq;
    r->kind = 1;
    r->n = 3;
    r->regs = regs;
    r->nregs = 2;
    r->tex[0].p = tex0;
    r->tex[0].n = 16;
    r->tex_type[0] = 0x0000000D;
    r->tex_dim[0] = 0x00040004;
    r->vtx.p = v;
    r->vtx.n = 48;
    r->idx.p = idx;
    r->idx.n = 6;
    r->mtx = ident;
    r->nmtx = 1;
}

int main(void) {
    NdDrec a, b;
    FILE* f;
    char got[4096];
    size_t len;
    uint32_t i;

    for (i = 0; i < 16; i++) tex0[i] = (uint8_t)i;
    for (i = 0; i < 48; i++) vtx[i] = (uint8_t)((i * 3) & 0xFF);
    build(&a, 0, vtx);
    vtx[5] = 0xFF;
    build(&b, 1, vtx);

    f = tmpfile();
    if (!f) { printf("drec FAIL\n"); return 1; }
    nd_drec_write(f, &a);
    nd_drec_write(f, &b);
    fflush(f);
    rewind(f);
    len = fread(got, 1, sizeof(got) - 1, f);
    got[len] = 0;
    fclose(f);

    if (len == strlen(expect) && memcmp(got, expect, len) == 0) {
        printf("drec ok 2\n");
        return 0;
    }
    printf("drec FAIL\n%s", got);
    return 1;
}
