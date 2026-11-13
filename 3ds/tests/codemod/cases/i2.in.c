ND_PSO_DESC(blur_pso, 1, 2);

void blur_appendGfx(void* effect) {
    s32 uls = 4;
    gSPDisplayList(gMainGfxPos++, D_09000918_3544C8);
    gDPSetTileSize(gMainGfxPos++, 1, uls, ult, uls + 128, ult + 256);
    if (flag) {
        gSPDisplayList(gMainGfxPos++, preset->dlist);
    }
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0);
    gDPSetKeyR(gMainGfxPos++, 1, 2, 0);
}
