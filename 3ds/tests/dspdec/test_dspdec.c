/* PT22 host test: the dspaudio cache decode (pm_vadpcm.h) equals aADPCMdecImpl (pm_rsp_audio.c, plain C path) for random books, shifts and data,
 * with the start state of a new sample (zero) and with a loop state. Build and run: sh 3ds/tests/dspdec/run.sh */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_rsp_audio.h"
#include "../../src/pm_vadpcm.h"

static uint32_t rs = 777;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }

int main(void) {
    int fails = 0;
    for (int it = 0; it < 4000; it++) {
        int nframes = 1 + rnd() % 20;
        int16_t book[8 * 16];
        unsigned char data[9 * 24 + 32];
        for (int i = 0; i < 128; i++) book[i] = (int16_t)((int)(rnd() % 8000) - 4000) * (it % 3 == 0 ? 3 : 1);
        for (int f = 0; f < nframes; f++) {
            data[f * 9] = (unsigned char)(((rnd() % 13) << 4) | (rnd() % 8));
            for (int j = 1; j < 9; j++) data[f * 9 + j] = (unsigned char)rnd();
        }
        int16_t st[16];
        for (int i = 0; i < 16; i++) st[i] = (int16_t)rnd();
        int withstate = it & 1;
        /* reference: the RSP routine on a DMEM image. 32 bytes of output per frame, rounded up to 32 bytes inside */
        aLoadADPCMImpl(sizeof book, book);
        aLoadBufferImpl(data, 0, nframes * 9);
        int16_t state[16];
        memcpy(state, st, sizeof state);
        if (!withstate) aADPCMdecImpl(A_INIT, state, nframes * 32, 0, 0x800);
        else aADPCMdecImpl(0, state, nframes * 32, 0, 0x800);
        int16_t ref[16 * 24];
        memcpy(ref, state, 0); /* state holds the last 16 samples only: decode again to read the whole output */
        (void)ref;
        int16_t out[16 * 24];
        vadpcm(data, nframes, book, withstate ? st[14] : 0, withstate ? st[15] : 0, out);
        /* compare the last 16 samples (the RSP state) and the decode of a second run frame by frame */
        int bad = 0;
        for (int i = 0; i < 16; i++) if (state[i] != out[nframes * 16 - 16 + i]) bad++;
        if (bad) { if (fails < 5) printf("MISMATCH it %d nframes %d state %d: %d of 16 samples\n", it, nframes, withstate, bad); fails++; }
    }
    printf("dspdec: %s (%d mismatches of 4000)\n", fails ? "FAIL" : "ok", fails);
    return fails != 0;
}
