void f(void) {
    nd_draw_list(NDN(A), ND_SELF);
    nd_draw_list(B, ND_SELF);
    gSPDisplayList(gMainGfxPos++, C);
}
