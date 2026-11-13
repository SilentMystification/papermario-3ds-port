void world_draw(void) {
    gDPPipeSync(gMainGfxPos++);
    gSPMatrix(gMainGfxPos++, VIRTUAL_TO_PHYSICAL(&gDisplayContext->matrixStack[gMatrixListPos++]), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    nd_rec_dl(D_0900AAAA);
    gSPDisplayList(gMainGfxPos++, D_0900BBBB);
    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
}
