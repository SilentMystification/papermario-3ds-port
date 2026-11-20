/* nd_xf_link_stub.c: host link stubs for nd_xf.c (NDR5). The host units that link nd_xf.c do not link nd_walk.c (pm_gbi_cur_op) or
 * nd_int.h's matrix code (nd_xf_load, nd_xf_popb). The transform run is not called by these units. */
unsigned pm_gbi_cur_op;
void nd_xf_load(unsigned w0, unsigned w1, unsigned hdr) { (void)w0; (void)w1; (void)hdr; }
void nd_xf_popb(unsigned w1) { (void)w1; }
__attribute__((weak)) int pm_ph_on; /* GD0B: the timer hooks of NDT_B and NDT_E in nd_xf.c (weak, as in nd_rec.c) */
__attribute__((weak)) void pm_gpu_ndt_push(void) {}
__attribute__((weak)) void pm_gpu_ndt_pop(int k) { (void)k; }
