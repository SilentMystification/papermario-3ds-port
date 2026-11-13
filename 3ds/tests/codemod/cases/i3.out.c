void world_draw(void) {
    nd_xf_push(VIRTUAL_TO_PHYSICAL(&gDisplayContext->matrixStack[gMatrixListPos++]), G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    nd_rec_dl(D_0900AAAA);
    gSPDisplayList(gMainGfxPos++, D_0900BBBB);
    nd_xf_pop(G_MTX_MODELVIEW);
}
