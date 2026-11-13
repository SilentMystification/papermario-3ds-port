ND_PSO_DESC(blur_pso, 1, 2);

void blur_appendGfx(void* effect) {
    s32 uls = 4;
    nd_draw_list(D_09000918_3544C8, blur_pso);
    nd_val_tile_size(1, uls, ult, uls + 128, ult + 256);
    if (flag) {
        nd_draw_list(preset->dlist, blur_pso);
    }
    gDPSetColorImage(gMainGfxPos++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH, 0);
    gDPSetKeyR(gMainGfxPos++, 1, 2, 0);
}
