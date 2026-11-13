void f(void) {
    gDPSetEnvColor(gMainGfxPos++, /* red */ 1, 2, 3, 4);
    gSPSegment /* seg */ (gMainGfxPos++, 1, base);
    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW); /* ok: the comment is outside */
    gDPPipeSync(gMainGfxPos++) /* before the semicolon */;
    foo(gSPMatrix(gMainGfxPos++, m, 0));
    // gDPSetEnvColor(gMainGfxPos++, 9, 9, 9, 9);
    puts("gDPSetEnvColor(gMainGfxPos++, 8, 8, 8, 8);");
}
