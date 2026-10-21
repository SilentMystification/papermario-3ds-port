#ifndef PM_VADPCM_H
#define PM_VADPCM_H
/* PT22: the VADPCM decode of the dspaudio cache. Same arithmetic as aADPCMdecImpl (pm_rsp_audio.c, plain C path). Host test: sh 3ds/tests/dspdec/run.sh */
#include <stdint.h>
/* ---------- VADPCM decode (same arithmetic as aADPCMdecImpl, scalar) ---------- */
static inline int16_t clamp16(int v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : (int16_t)v; }
static void vadpcm(const unsigned char* in, int nframes, const int16_t* book, int16_t h2, int16_t h1, int16_t* out) {
    for (int f = 0; f < nframes; f++) {
        int shift = in[0] >> 4, ti = in[0] & 0xF;
        const int16_t* tbl0;
        const int16_t* tbl1;
        if (ti > 7) ti = 7;
        tbl0 = book + ti * 16;
        tbl1 = tbl0 + 8;
        in++;
        for (int half = 0; half < 2; half++) {
            int16_t ins[8];
            for (int j = 0; j < 4; j++) {
                ins[j * 2] = (int16_t)((((int)(*in >> 4) << 28) >> 28) << shift);
                ins[j * 2 + 1] = (int16_t)((((int)(*in & 0xF) << 28) >> 28) << shift);
                in++;
            }
            for (int j = 0; j < 8; j++) {
                int acc = tbl0[j] * h2 + tbl1[j] * h1 + (ins[j] << 11);
                for (int k = 0; k < j; k++) acc += tbl1[(j - k) - 1] * ins[k];
                acc >>= 11;
                int16_t o = clamp16(acc);
                out[j] = o;
            }
            /* the history of a block of 8 is the last two samples of the block before (as aADPCMdecImpl): not updated inside the block */
            h2 = out[6];
            h1 = out[7];
            out += 8;
        }
    }
}

#endif
