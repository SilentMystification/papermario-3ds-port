ND_PSO_DESC(D_X, 1, 2, 3);

void geo_f(void* x) {
    nd_rec_dl(D_X);
    gDPSetCycleType(gMainGfxPos++, G_CYC_1CYCLE);
}
