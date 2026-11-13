void f(Gfx** p) {
    gSPMatrix(*p, mtx, G_MTX_PUSH);
    gDPSetPrimColor(gMainGfxPos, 0, 0, 1, 2, 3, 4);
    nd_xf_pop(G_MTX_MODELVIEW);
}
