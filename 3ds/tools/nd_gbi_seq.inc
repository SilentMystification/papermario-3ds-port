/* nd_gbi_seq.inc: one sequence of every macro the S2 files use (main_loop.c, cam_main.c, background_gfx.c, background.c), included by nd_gbi_test.c
 * (with nd_gbi.h) and nd_gbi_ref.c (plain gbi.h). Writes the list at gp, returns the end. */
extern Mtx seq_mtx;
extern Vp seq_vp;
extern unsigned char seq_img[8192];
Gfx* ND_SEQ_NAME(Gfx* gp) {
    gDPPipeSync(gp++);
    gSPMatrix(gp++, &seq_mtx, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(gp++, &seq_mtx, G_MTX_MODELVIEW | G_MTX_MUL | G_MTX_PUSH);
    gSPPopMatrix(gp++, G_MTX_MODELVIEW);
    gSPViewport(gp++, &seq_vp);
    gDPSetColorImage(gp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 320, 0x80200000);
    gDPSetDepthImage(gp++, 0x80300000);
    gSPTexture(gp++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPPerspNormalize(gp++, 0x1234);
    gSPClipRatio(gp++, FRUSTRATIO_2);
    gSPSetGeometryMode(gp++, G_ZBUFFER | G_SHADE | G_CULL_BACK);
    gSPClearGeometryMode(gp++, G_SHADE | G_FOG);
    gDPSetTexturePersp(gp++, G_TP_PERSP);
    gDPSetTextureLUT(gp++, G_TT_RGBA16);
    gDPSetTextureLOD(gp++, G_TL_TILE);
    gDPSetTextureFilter(gp++, G_TF_BILERP);
    gDPSetTextureDetail(gp++, G_TD_CLAMP);
    gDPSetTextureConvert(gp++, G_TC_FILT);
    gDPSetScissor(gp++, G_SC_NON_INTERLACE, 8, 16, 312, 224);
    gDPSetRenderMode(gp++, G_RM_AA_ZB_OPA_SURF, G_RM_AA_ZB_OPA_SURF2);
    gDPSetCycleType(gp++, G_CYC_2CYCLE);
    gDPSetCombineMode(gp++, G_CC_MODULATERGBA, G_CC_PASS2);
    gDPSetCombineKey(gp++, G_CK_NONE);
    gDPSetColorDither(gp++, G_CD_DISABLE);
    gDPSetAlphaCompare(gp++, G_AC_THRESHOLD);
    gDPSetDepthSource(gp++, G_ZS_PRIM);
    gDPPipelineMode(gp++, G_PM_NPRIMITIVE);
    gDPSetPrimColor(gp++, 5, 6, 0x11, 0x22, 0x33, 0x44);
    gDPSetEnvColor(gp++, 0x55, 0x66, 0x77, 0x88);
    gDPSetBlendColor(gp++, 1, 2, 3, 4);
    gDPSetFillColor(gp++, 0xAABBCCDD);
    gDPSetPrimDepth(gp++, 0x7FFF, 0);
    gDPFillRectangle(gp++, 10, 20, 100, 200);
    gDPLoadTextureTile(gp++, seq_img, G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, 64, 0, 0, 31, 31, 0, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
    gDPLoadTLUT_pal256(gp++, seq_img);
    gSPTextureRectangle(gp++, 0, 0, 320 << 2, 240 << 2, G_TX_RENDERTILE, 0, 0, 1 << 10, 1 << 10);
    gSPSegment(gp++, 4, 0x01234567);
    gSPDisplayList(gp++, seq_img);
    gDPNoOp(gp++);
    gDPFullSync(gp++);
    gSPEndDisplayList(gp++);
    return gp;
}
