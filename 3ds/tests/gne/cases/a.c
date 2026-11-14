/* CONV2C2 writer 1 cases (3ds/tests/gne). Shapes of the rules in ws_CONV2C2.md; the desc line is copied from src/effects/blast.c:4. */
ND_PSO_DESC(pe_blast_131, 0xfc267ec0, 0x1334fed2, 0x1e183cf0, 0x0c184b50, 0x00000005, 0x00000000, 0x00000000, 0x00000000, 1, 0, 1, 0, 0, 0, 1, 0, 3, 0, 0, 0);

/* 1: a name (balloon.c:107) */
void case_name(void) {
    nd_draw_list(D_09001508_395B78, pe_blast_131);
}

/* 2: a name with designators (aura.c:21-26, :356) */
Gfx* D_E0076E90[] = {
    [FX_AURA_CAPTURE]   D_090023E0_37A360,
    [FX_AURA_RED]       D_090023E0_37A360,
    [FX_AURA_BLUE]      D_09002440_37A3C0,
    [FX_AURA_GOLD]      D_09002440_37A3C0
};
void case_array(int type) {
    nd_draw_list(D_E0076E90[type], pe_blast_131);
}

/* 3: a local (blast.c:103, :132) */
void case_local(void) {
    Gfx* dlist = D_090017B0_37F180;
    nd_draw_list(dlist, pe_blast_131);
}

/* 4: a ternary (dust.c:166) */
void case_tern(int i) {
    nd_draw_list((i % 2) ? D_09000E18_33FE18 : D_09000E38_33FE38, pe_blast_131);
}

/* 5: a 2D array (lil_oink.c:29-33, :202) */
Gfx* lil_oink_FramesGfx[][3] = {
    [LIL_OINK_TYPE_BLACK]       { D_09003358_4025A8, D_09003370_4025C0, D_09003388_4025D8 },
    [LIL_OINK_TYPE_WHITE]       { D_09003358_4025A8, D_09003370_4025C0, D_09003388_4025D8 },
    [LIL_OINK_TYPE_PINK]        { D_09003358_4025A8, D_09003370_4025C0, D_09003388_4025D8 }
};
void case_2d(void* data, int i) {
    nd_draw_list(lil_oink_FramesGfx[data->type[i]][data->gfxFrame[i]], pe_blast_131);
}

/* 6: ND_SELF (no line) */
void case_self(void) {
    nd_draw_list(D_09002D70_401FC0, ND_SELF);
}

/* 7: a call (skip) */
void case_call(void) {
    nd_draw_list(foo(1), pe_blast_131);
}

/* 8: no desc line (skip) */
void case_nodesc(void) {
    nd_draw_list(D_09000000_1, pe_nodesc_1);
}
