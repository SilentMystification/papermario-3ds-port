#ifndef ND_SCSIG_H
#define ND_SCSIG_H
/* HWFIX: the flag word of sc_sig_now (nd_comb.c): each flag has its own bit, vprim from bit 9 */
static inline unsigned nd_sc_flags(int tex_on, int use_t1, int t1_valid, int dual_on, int box_prod, int box_pat, int box_folded, int comb_set, int comb_alpha_shade, int vprim) {
    return (unsigned)(tex_on | use_t1 << 1 | t1_valid << 2 | dual_on << 3 | box_prod << 4 | box_pat << 5 | box_folded << 6 | comb_set << 7 | comb_alpha_shade << 8) | (unsigned)vprim << 9;
}
#endif
