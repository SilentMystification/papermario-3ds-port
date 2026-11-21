# ws_WCLIP0 (Haiku): clip census, drec scissor registers, scenario words and 4 drec scenarios (plan_widewin.md 5)
Designer: oracle NATIVE20. Worktree build3ds/wt/WCLIP0, branch WCLIP0. PM_ROW=WCLIP0 PM_LANE=WCLIP0. Start: main. Counter: none (census row).
Objective: a mechanical clip count per drec window. No draw changes: gst.txt and shots stay byte-equal; drec.txt gains 3 lines per draw. Edits (3ds/src, azt.d, bench.d):
1. pm_gpu.c:1881: `pm_stretch_cfg = pm_opt.wide == 2 || pm_debug_has("fullscr");` (scenario word for Fullscreen).
2. pm_warp.c:2093. Before: `msg_printer_set_origin_pos(tp, -30, 220);`. After: `msg_printer_set_origin_pos(tp, uv[2] == 't' ? 350 : -30, 220);` (uishow=tatr).
3. pm_gpu.c: the prototype of drec_sc_put above drec_draw (:699). In drec_draw after the index block (:753): `drec_sc_put(r, &n, d, vb, ix); rec.nregs = n;`.
   The body after sh_scissor (:827; `sh` is at :787). drec_begin after setvbuf (:668): `pm_log_evt("drecsc f%u", pm_frame_count());`. drec_end before
   the `drec f%u draws` line (:688): `clipc f<F> groups <n> over <o>`, one `clipg` line per group, then clear the table.
```c
typedef struct { int gx0, gx1, top, bot; unsigned n, out; int lo, hi; } ClipG; static ClipG clipg[8]; static unsigned clipg_n, clipg_over; /* WCLIP0 */
static void drec_sc_put(NdDrecReg* r, unsigned* n, const NdDraw* d, const uint8_t* vb, const uint16_t* ix) {
    if (*n + 3u < DREC_REGS) { r[*n].reg = 0x065; r[*n].mask = 0xF; r[(*n)++].val = sh.sc[0];
        r[*n].reg = 0x066; r[*n].mask = 0xF; r[(*n)++].val = sh.sc[1] | sh.sc[2] << 16; r[*n].reg = 0x067; r[*n].mask = 0xF; r[(*n)++].val = sh.sc[3] | sh.sc[4] << 16; }
    if (gs.off_active || d->no_uni || !d->npal || !d->nidx || !ix) return;
    const C3D_Mtx* P = &projection; int j = fabsf(P->r[0].x) > fabsf(P->r[1].x) ? 0 : 1; /* the clip row that screen x moves */
    const float a = P->r[j].w, b = P->r[j].x * 400.f + P->r[j].w; if (P->r[j].y != 0.f || b == a) return;
    const int top = 400 - (int)sh.sc[4], bot = 400 - (int)sh.sc[2], v0 = (pm_wide || pm_stretch) ? 0 : 40, v1 = (pm_wide || pm_stretch) ? 400 : 360;
    float lo = 1e9f, hi = -1e9f; for (unsigned i = 0; i < d->nidx; i++) { const short* p = (const short*)(const void*)(vb + 16u * ix[i]); const unsigned k = (unsigned short)p[3];
        if (k >= d->npal) continue; const C3D_Mtx* M = (const C3D_Mtx*)(const void*)d->pal[k];
        const float w = M->r[3].x * p[0] + M->r[3].y * p[1] + M->r[3].z * p[2] + M->r[3].w; if (!(w > 0.f)) continue;
        const float c = (M->r[j].x * p[0] + M->r[j].y * p[1] + M->r[j].z * p[2] + M->r[j].w) / w, sx = (c - a) * 400.f / (b - a);
        if (sx < lo) lo = sx; if (sx > hi) hi = sx; }
    if (lo < (float)v0) lo = (float)v0; if (hi > (float)v1) hi = (float)v1;
    unsigned g = 0; while (g < clipg_n && !(clipg[g].gx0 == gsci_r[0] && clipg[g].gx1 == gsci_r[2] && clipg[g].top == top && clipg[g].bot == bot)) g++;
    if (g == clipg_n) { if (g == 8u) { clipg_over++; return; } clipg[g] = (ClipG){ gsci_r[0], gsci_r[2], top, bot, 0, 0, 9999, -9999 }; clipg_n++; }
    clipg[g].n++; if (hi > lo && (lo < (float)top - 0.5f || hi > (float)bot + 0.5f)) { clipg[g].out++; if ((int)lo < clipg[g].lo) clipg[g].lo = (int)lo; if ((int)hi > clipg[g].hi) clipg[g].hi = (int)hi; }
}
```
   Line: `clipg f%u g %d %d c %d %d n %u out %u bb %d %d` (bb 0 0 when out is 0). Vertex word 3 = the palette index (pm_gpu.c:3643).
4. azt.d/WCLIP0.json, bench.d/WCLIP0.json (form of azt.d/NWS4B.json, bench.d/NWS4B.json; kind azahar_pic; tags azahar det drec). name: debug; until (= the drec event); cap:
   msgw_wide_drec: `preset=yard wide uishow=tatr loglevel=debug det drec=330:1`; `drec f330 draws [1-9]\d* bytes \d+`; 30. msgw_full_drec: the same with `fullscr`
   for `wide`. pause_wide_drec: `preset=toad padscript=wait80,START*3 wide loglevel=debug det drec=116:1`; `drec f116 draws [1-9]\d* bytes \d+`; 25.
   battle_wide_drec: `warp=kmr_03:0 story=ch0_toad_town spmax=1 hammer=0 battle battlef=100 wide det loglevel=debug padscript=wait362,right*4,wait30 drec=380:6:30`;
   `drec f530 draws [1-9]\d* bytes \d+`; 45.
## Tests (build3ds/out/, last 20 lines). Base: `bench.py base <b> --commit $(git merge-base HEAD main)`. 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=WCLIP0 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. `bench.py run spot_mac01_drec --row WCLIP0`; `bench.py cmp spot_mac01_drec --row WCLIP0 --expect drecsc`: `expected-diff PASS`, `gst <n> draws equal`,
   every shot `equal`. `python 3ds/tools/drecsum.py spot_mac01_drec --row WCLIP0`: `drecsum equal`.
3. Run msgw_wide_drec once. Its log has `clipg f330 g 0 319 c 0 359` with out 1 or more (GUESS from msg_draw.c:527-528: the bubble ends at column 382).
Fix rights: compile errors, the scenario JSON. A shot or gst DIFF in test 2, out 0 in test 3, no `clipc`: send the output path to oracle NATIVE20. Budget 8 starts.
Stop: commit `WCLIP0 1: gate` (deletes this page). Hand-back: one SubagentHandback: hash, host, cmp lines, drecsum, every clipg line of test 3.
