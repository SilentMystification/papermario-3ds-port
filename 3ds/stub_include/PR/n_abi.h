#ifndef PM_N_ABI_SHADOW_H
#define PM_N_ABI_SHADOW_H
#include_next <PR/n_abi.h>
#include "pm_rsp_audio.h"
#undef n_aNoop
#undef n_aADPCMdec
#undef n_aPoleFilter
#undef n_aEnvMixer
#undef n_aInterleave
#undef n_aLoadBuffer
#undef n_aResample
#undef n_aSaveBuffer
#undef n_aSetVolume
#undef n_aLoadADPCM
#define n_aNoop(pkt, outp, b, c) aDisable(pkt, outp, b, c);
#define n_aADPCMdec(pkt, s, f, c, a, d) aADPCMdec(pkt, s, f, c, a, d);
#define n_aPoleFilter(pkt, f, g, t, s) aPoleFilter(pkt, f, g, t, s);
#define n_aEnvMixer(pkt, f, t, s) aEnvMixer(pkt, f, t, s);
#define n_aInterleave(pkt) aInterleave(pkt);
#define n_aLoadBuffer(pkt, c, d, s) aLoadBuffer(pkt, c, d, s);
#define n_aResample(pkt, s, f, p, i, o) aResample(pkt, s, f, p, i, o);
#define n_aSaveBuffer(pkt, c, s, d) aSaveBuffer(pkt, c, s, d);
#define n_aSetVolume(pkt, f, v, t, r) aSetVolume(pkt, f, v, t, r);
#define n_aLoadADPCM(pkt, c, d) aLoadADPCM(pkt, c, d);
#endif
