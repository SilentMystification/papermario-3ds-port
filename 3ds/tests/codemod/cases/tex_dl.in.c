ND_PSO_DESC(D_X, 1, 2, 3);

void tex_f(void* x) {
    nd_rec_dl(D_X);
    gDPSetTile(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 0, 0, G_TX_RENDERTILE, 0, G_TX_WRAP, 0, 0, G_TX_WRAP, 0, 0);
    gDPSetTileSize(gMainGfxPos++, G_TX_RENDERTILE, 0, 0, 127, 15);
}
