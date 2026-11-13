void f(Gfx** p) {
    gSPMatrix(*p, mtx, G_MTX_PUSH);
    gDPSetPrimColor(gMainGfxPos, 0, 0, 1, 2, 3, 4);
    gSPPopMatrix(gMainGfxPos++ , G_MTX_MODELVIEW);
}
