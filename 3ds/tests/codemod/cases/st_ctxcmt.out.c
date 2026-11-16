void f(void) {
    a = 1; // a trailing line comment
    nd_val_env(1, 2, 3, 4);
    b(); /* a trailing block comment */
    nd_pso_cc(G_CC_DECALRGBA, G_CC_DECALRGBA);
    // a comment line
    x = /* not a statement start */ gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
}
