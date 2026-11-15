Gfx D[] = {
    gsSPEndDisplayList(),
};
void pal(void) {
    gSPDisplayList(gMainGfxPos++, D);
    nd_draw_list(A, ND_SELF);
}
