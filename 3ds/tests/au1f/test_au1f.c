/* AU1f host test: the new pm_rsp_audio.c and the old copy (pm_rsp_audio_ref.c, renamed by -D) give equal output for random input.
 * Build and run: sh 3ds/tests/au1f/run.sh */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pm_rsp_audio.h"

#define REF(n) ref_##n
void REF(aLoadBufferImpl)(const void*, uint16_t, uint16_t);
void REF(aSaveBufferImpl)(uint16_t, int16_t*, uint16_t);
void REF(aLoadADPCMImpl)(int, const int16_t*);
void REF(aSetLoopImpl)(ADPCM_STATE*);
void REF(aADPCMdecImpl)(uint8_t, ADPCM_STATE, int, uint16_t, uint16_t);
void REF(aResampleImpl)(uint8_t, uint16_t, RESAMPLE_STATE, uint16_t, uint8_t);
void REF(aMixImpl)(uint8_t, int16_t, uint16_t, uint16_t);
void REF(aEnvMixerImpl)(uint8_t, ENVMIX_STATE, int16_t);
void REF(aSetVolumeImpl)(uint8_t, int16_t, int16_t, int16_t);
void REF(aPoleFilterImpl)(uint8_t, int16_t, uint32_t, void*);
void REF(aInterleaveImpl)(void);

static uint32_t rs = 12345;
static uint32_t rnd(void) { rs = rs * 1664525u + 1013904223u; return rs >> 8; }
static int16_t rs16(int mode) {
    uint32_t r = rnd();
    if (mode == 0) return (int16_t)r;
    if (mode == 1) return (int16_t)((r & 1) ? 0x7fff : -0x8000);
    return (int16_t)((int)(r % 2000) - 1000);
}
static int fails;
#define BUFB 3072
static void fill(int mode) {
    int16_t b[BUFB / 2];
    for (int i = 0; i < BUFB / 2; i++) b[i] = rs16(mode);
    aLoadBufferImpl(b, 0, BUFB);
    REF(aLoadBufferImpl)(b, 0, BUFB);
}
static void cmp(const char* what, int iter) {
    int16_t a[BUFB / 2], b[BUFB / 2];
    aSaveBufferImpl(0, a, BUFB);
    REF(aSaveBufferImpl)(0, b, BUFB);
    if (memcmp(a, b, sizeof a)) {
        if (fails++ < 10) printf("FAIL %s iter %d\n", what, iter);
    }
}

int main(void) {
    for (int it = 0; it < 20000; it++) {
        int mode = it % 3;
        fill(mode);
        /* envmixer: random start state, random rates (also zero and tiny), random targets */
        {
            int16_t v = rs16(0), t = rs16(0), r = rs16(0), vd = rs16(mode), vw = (it & 4) ? 0 : rs16(mode);
            int16_t rate_h = (it & 8) ? 0 : (int16_t)(rnd() % 200 - 100), rate_l = rs16(0), tgt = rs16(mode);
            int16_t rate_h2 = (it & 16) ? 0 : (int16_t)(rnd() % 40 - 20);
            uint8_t fl = (it & 32) ? 0x01 : 0; /* A_INIT is 1 */
            uint8_t st1[40], st2[40];
            for (int i = 0; i < 40; i++) st1[i] = st2[i] = (uint8_t)rnd();
            int32_t* p1 = (int32_t*)st1; (void)p1;
            /* settable state: t[2] rate[2] as int32, tgt int16 x2, vol dry, vol wet */
            int32_t tt[2] = { (int32_t)(rnd() << 8), (int32_t)(rnd() << 8) }, rr[2] = { (int32_t)(rate_h << 8), (int32_t)(rate_h2 << 8) };
            int16_t tg[2] = { tgt, rs16(mode) };
            memcpy(st1, tt, 8); memcpy(st1 + 8, rr, 8); memcpy(st1 + 16, tg, 4);
            memcpy(st1 + 20, &vd, 2); memcpy(st1 + 22, &vw, 2);
            memcpy(st2, st1, 40);
            aSetVolumeImpl(0x08 | 0x02, v, vd, vw); REF(aSetVolumeImpl)(0x08 | 0x02, v, vd, vw);
            aSetVolumeImpl(0x04 | 0x02, v, vd, vw); REF(aSetVolumeImpl)(0x04 | 0x02, v, vd, vw);
            aSetVolumeImpl(0x08, tgt, rate_h, rate_l); REF(aSetVolumeImpl)(0x08, tgt, rate_h, rate_l);
            aSetVolumeImpl(0x04, tgt, rate_h2, rate_l); REF(aSetVolumeImpl)(0x04, tgt, rate_h2, rate_l);
            (void)t;
            aEnvMixerImpl(fl, (void*)st1, r);
            REF(aEnvMixerImpl)(fl, (void*)st2, r);
            cmp("envmix", it);
            if (memcmp(st1, st2, 24)) { if (fails++ < 10) printf("FAIL envmix state iter %d\n", it); }
        }
        /* mix */
        { int16_t g = (it % 7 == 0) ? -0x8000 : rs16(mode); aMixImpl(0, g, 0x170, 0x4E0); REF(aMixImpl)(0, g, 0x170, 0x4E0); cmp("mix", it); }
        /* resample */
        {
            uint16_t pitch = (uint16_t)(rnd() % 0xC000);
            if (it % 5 == 0) pitch = 0x4000;
            int16_t s1[16], s2[16];
            for (int i = 0; i < 16; i++) s1[i] = s2[i] = rs16(mode);
            uint8_t of = (uint8_t)(rnd() & 3);
            uint16_t inofs = 0x4E0 + (uint16_t)(rnd() % 8) * 2 * 0;
            aResampleImpl(0, pitch, s1, inofs, of); REF(aResampleImpl)(0, pitch, s2, inofs, of);
            cmp("resample", it);
            if (memcmp(s1, s2, sizeof s1)) { if (fails++ < 10) printf("FAIL resample state iter %d\n", it); }
        }
        /* adpcm */
        {
            int16_t book[8 * 2 * 8];
            for (int i = 0; i < 128; i++) book[i] = (it & 1) ? rs16(2) : rs16(0);
            aLoadADPCMImpl(sizeof book, book); REF(aLoadADPCMImpl)(sizeof book, book);
            ADPCM_STATE s1, s2, lp;
            for (int i = 0; i < 16; i++) s1[i] = s2[i] = lp[i] = rs16(mode);
            aSetLoopImpl(&lp); REF(aSetLoopImpl)(&lp);
            uint8_t fl = (uint8_t)(it % 3 == 0 ? 1 : it % 3 == 1 ? 2 : 0);
            int nb = 9 * 2 * (1 + (int)(rnd() % 11)) ; (void)nb;
            aADPCMdecImpl(fl, s1, 9 * 4 * 2, 0x4E0, 0x170); REF(aADPCMdecImpl)(fl, s2, 9 * 4 * 2, 0x4E0, 0x170);
            cmp("adpcm", it);
            if (memcmp(s1, s2, sizeof s1)) { if (fails++ < 10) printf("FAIL adpcm state iter %d\n", it); }
        }
        /* pole filter */
        {
            int16_t st1 = rs16(mode), st2; st2 = st1; int16_t g = rs16(mode);
            aPoleFilterImpl(it & 1, g, 0, &st1); REF(aPoleFilterImpl)(it & 1, g, 0, &st2);
            cmp("pole", it);
            if (st1 != st2) { if (fails++ < 10) printf("FAIL pole state iter %d\n", it); }
        }
        aInterleaveImpl(); REF(aInterleaveImpl)(); cmp("interleave", it);
    }
    printf(fails ? "FAILED %d\n" : "OK: new and old give equal output (20000 rounds, 7 ops)\n", fails);
    return fails != 0;
}
