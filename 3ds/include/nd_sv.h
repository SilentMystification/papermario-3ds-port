/* nd_sv.h: the native value records of the stream state units (NSET1). Core 1 (nd_sv_enc) reads the applied state and writes 2 value words; core 0 runs the applier nd_sv_X on those words. */ /* NSET1 */
#ifndef ND_SV_H /* NSET1 */
#define ND_SV_H /* NSET1 */
#include <stdint.h> /* NSET1 */
#include "nd_api.h" /* NSET1 */
#define ND_SV_SYNC 0x100u /* NSET1 */
struct NdDs; /* NSET1 */
NdNapiRun nd_sv_enc(const struct NdDs* s, uint32_t w0, uint32_t w1, uint32_t* v); /* NSET1: core 1, returns the applier and writes v[0], v[1] */
unsigned nd_sv_sync(uint32_t w0); /* NSET1: 1 when the old setter ends with nd_comb_sync */
void nd_sv_nop(const uint32_t* r); void nd_sv_omh(const uint32_t* r); void nd_sv_oml(const uint32_t* r); void nd_sv_geom(const uint32_t* r); void nd_sv_prim(const uint32_t* r); /* NSET1 */
void nd_sv_env(const uint32_t* r); void nd_sv_fog(const uint32_t* r); void nd_sv_blend(const uint32_t* r); void nd_sv_fill(const uint32_t* r); void nd_sv_primz(const uint32_t* r); /* NSET1 */
void nd_sv_sci(const uint32_t* r); void nd_sv_tex(const uint32_t* r); void nd_sv_nlt(const uint32_t* r); void nd_sv_lcol(const uint32_t* r); void nd_sv_fogf(const uint32_t* r); /* NSET1 */
void nd_sv_seg(const uint32_t* r); void nd_sv_conv(const uint32_t* r); void nd_sv_keyr(const uint32_t* r); void nd_sv_keygb(const uint32_t* r); void nd_sv_comb(const uint32_t* r); /* NSET1 */
void nd_sv_timg(const uint32_t* r); void nd_sv_tile(const uint32_t* r); void nd_sv_tsz(const uint32_t* r); void nd_sv_tlut(const uint32_t* r); void nd_sv_tlutn(const uint32_t* r); /* NSET1 */
#endif /* NSET1 */
