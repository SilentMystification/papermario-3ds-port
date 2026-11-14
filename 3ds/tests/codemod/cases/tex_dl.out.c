ND_PSO_DESC(D_X, 1, 2, 3);

void tex_f(void* x) {
    nd_rec_dl(D_X);
    nd_tex_tile(G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_RENDERTILE, 0, G_TX_WRAP, 0, 0, G_TX_WRAP, 0, 0);
    nd_tex_tsz(G_TX_RENDERTILE, 0, 0, 127, 15);
}
