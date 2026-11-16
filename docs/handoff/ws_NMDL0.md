# ws_NMDL0 (Sonnet, user approved 2026-11-16): the model record and the texture bind lists of src/model.c are built without gbi macros (C1 C2 C5 C7)
Designer: the oracle (SendMessage). Worktree build3ds/wt/NMDL0, branch NMDL0. PM_ROW=NMDL0 PM_LANE=NMDL0.
Start after Merge MODELRM and Merge WSCMB (shared: src/model.c, 3ds/tools/nd_at_test.c, nd_tx_test.c; NdCmbW and ND_CMB_W come from WSCMB).
Why not Haiku: the record writer and 4 readers change in one step, and each emitter form needs its own word proof.
## Objective and counters (line numbers: MODELRM head 639142d791; re-grep after the merge)
- FACT (listcen, main 3771a3edbb): src/model.c build 67, gfxpos 18. FACT (MODELRM branch): model.c gsDPSetCombineMode 210 (SolidCombineModes :121, AlphaTestCombineModes :284), Gfx lines 41.
- In scope: appendGfx_model (:817-1543: recBuf :851, local Gfx tw/fogWords/shRm/sw/fc/fp/pt/bb :980-1495), load_texture_impl (:1583-1632: pm_tex_unloaded :1581,
  handle->gfx :1596 :1627, gSPEndDisplayList :1632), render_transform_group_node and render_transform_group (:2679-2745), make_texture_gfx (:2747 to its end before :3012).
- Out of scope (other rows): is_point_visible (:3932, NTGT), mdl_get_copied_gfx, custom gfx and parade lists (R11, NMDL rows), the geometry list R12 (MAPCONV).
- GUESS pass values: model.c build 67 -> 8 or fewer, gfxpos 18 -> 15 or fewer, data -210, gfxtype -20 or more. The coder writes the real values.
## Readers and writers (grep each again first; list each in the commit text)
- Record writer: model.c appendGfx_model. Record readers: 3ds/src/nd_rec.c nd_rec_model (:221, takes const void*), 3ds/src/nd_recs.c rec_apply, rec_matrix, rec_run
  and the hit path (q is `const Gfx*`: q[k].words.w0/w1), 3ds/src/nd_c2haz.c nd_c2_rec (record pointer words), 3ds/src/nd_cen.c (G_PM_REC arm).
- Bind list writer: make_texture_gfx (via load_texture_impl and the R13 call in appendGfx_model). Readers: nd_tex_set.c nd_bind_run (:552, already uint32_t*),
  nd_model.c nd_tex_handle_set (:109, const void*), model.c (:1449 null test). TextureHandle.gfx is `Gfx*` (include/model.h:126). No reader outside model.c and 3ds/src (FACT grep).
- Combine tables: the only readers are model.c (combWord :1048 :1050, the copy :2841) and nd_recs.c:86 rec_top((const uint32_t*)e).
## Design rules (the coder decides the rest)
- Same bits: every record word, R13 unit and bind unit stays bit-equal to today. The host test proves it word for word. No new record field.
- Record buffer: `uint32_t` pairs, not Gfx. R13 and the bind lists use the NSTREAM3 emitter forms (3ds/include/nd_at.h, nd_tx.h) with ND_NR_AT into a local `uint32_t* nrp`
  (nd_nr.h:11). Add only the O forms the sites need (fog colour, fog position, blend, alpha dither, texture on, other mode, multi-tile loads, TLUT 256, end unit).
- make_texture_gfx takes `uint32_t** nrp`. TextureHandle.gfx becomes `const uint32_t*` (NdNr list, NDO_END unit last). pm_tex_unloaded becomes one END unit.
- SolidCombineModes and AlphaTestCombineModes become NdCmbW tables written with ND_CMB_W (WSCMB form). R4 w1 and the copy at :2841 use them.
- render_transform_group(_node): gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop, gDPPipeSync -> nd_pso_pipe (form of model.c parade: nd_xf_push at main :4954).
- No run-time cost on core 0. No division added (hw_guidelines.md). No toggle, no stub.
## Edit shapes (real code)
Before (src/model.c, main :1585): `gSPTexture((*gfxPos)++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);` After: `NDST_TEX(ND_NR_AT, 0, 0, 0, G_TX_RENDERTILE, G_OFF);` (new O form).
Before (main :1551-1553): `Gfx tw[3]; gDPSetPrimColor(&tw[0], 0, 0, R, G, B, A); ... rPrim = tw[0].words.w1;` After: `rPrim = ND_NR_RGBA(R, G, B, A);` (nd_at.h:18 value).
## Tests that decide
1. New host test 3ds/tests/host.d/nmdl0.sh: for each new O form, the 2 words equal the gbi macro on a local packet (NSTREAM3 test pattern, nd_at_test.c).
   For make_texture_gfx: every format, size, palette and aux case of the function gives the same units old and new. Print `nmdl0 ok <n>`. Seeded fault: one changed word prints FAIL.
2. `sh 3ds/tools/nd_host_test.sh`: `ND HOST TESTS PASS`, `lkey census ok`.
3. One build: rc 0, memcheck margin above 0. listcen into build3ds/out/nmdl0_lc.txt.
4. Gate (at most 6 Azahar starts, base runs not charged): spot_mac01_drec, sbk14_drec, jan09_mesh_drec, fxr_drec. `bench.py base <b>`, `bench.py run <b> --row NMDL0 --why "NMDL0 gate"`,
   `bench.py cmp <b> --row NMDL0 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
## Loop and stop
Group 1: O forms + host test (commit `NMDL0 1`). Group 2: model.c writer, readers, combine tables (commit `NMDL0 2`). Then the build and the gate (commit `NMDL0 3: gate`, deletes this page).
Fix scope: the files named on this page. Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle. Hand-back: commit, listcen totals, gate values, memcheck.
