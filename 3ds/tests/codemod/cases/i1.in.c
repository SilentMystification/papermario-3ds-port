ND_PSO_DESC(efx_pso, 0, 0);

void efx_appendGfx(void* effect) {
    gDPPipeSync(gMainGfxPos++);
    gSPSegment(gMainGfxPos++, 0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
    gSPMatrix(gMainGfxPos++, &gDisplayContext->matrixStack[gMatrixListPos++],
              G_MTX_PUSH | G_MTX_MUL | G_MODELVIEW);
    gDPSetPrimColor(gMainGfxPos++, 0, 0, 255, 128, data->alpha, 0);
    gDPSetEnvColor(gMainGfxPos++, 10, 20, 30, 40);
    nd_rec_dl(D_09000000_ABCDEF);
    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
    gDPPipeSync(gMainGfxPos++);
}
