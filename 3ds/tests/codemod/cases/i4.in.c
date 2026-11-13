ND_PSO_DESC(a_pso, 0, 0);
ND_PSO_DESC(b_pso, 1, 1);

void two_entries(void) {
    if (x)
        gDPPipeSync(gMainGfxPos++);
    else
        gDPSetEnvColor(gMainGfxPos++, 1, 2, 3, 4);
    switch (k) {
    case 1:
        gDPPipeSync(gMainGfxPos++); /* keep this note */
        break;
    }
    gDPPipeSync(gMainGfxPos++); gSPSegment(gMainGfxPos++, 2, base);
    nd_rec_dl(D_09000000);
}
