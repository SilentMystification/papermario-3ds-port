ND_PSO_DESC(efx_pso, 0, 0);

void efx_appendGfx(void* effect) {
    nd_val_seg(0x09, VIRTUAL_TO_PHYSICAL(((EffectInstance*)effect)->shared->graphics));
    nd_xf_push(&gDisplayContext->matrixStack[gMatrixListPos++], G_MTX_PUSH | G_MTX_MUL | G_MODELVIEW);
    nd_val_prim(0, 0, 255, 128, data->alpha, 0);
    nd_val_env(10, 20, 30, 40);
    nd_draw_list(D_09000000_ABCDEF, efx_pso);
    nd_xf_pop(G_MTX_MODELVIEW);
}
