#define SET_COLORS(r, g, b) \
    gDPSetEnvColor(gMainGfxPos++, r, g, b, 255); \
    gDPPipeSync(gMainGfxPos++)

void f(void) {
    SET_COLORS(1, 2, 3);
    nd_val_env(4, 5, 6, 7);
}
