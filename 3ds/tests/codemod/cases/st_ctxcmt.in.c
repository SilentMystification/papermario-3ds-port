void f(void) {
    a = 1; // a trailing line comment
    gDPSetEnvColor(gMainGfxPos++, 1, 2, 3, 4);
    b(); /* a trailing block comment */
    gDPSetCombineMode(gMainGfxPos++, G_CC_DECALRGBA, G_CC_DECALRGBA);
    // a comment line
    gDPPipeSync(gMainGfxPos++);
    x = /* not a statement start */ gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
}
