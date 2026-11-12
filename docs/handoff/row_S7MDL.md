# Row S7MDL: model records without list runs (render-mode lists as compiled defs, texture setup lists as binds; NATIVEAPI S7)

Model: Opus (pre-approved for the night run: S7MDL is in the user's Opus list).
- Why not Sonnet: the model record path is the largest state producer and has a record cache (KEYMISS keys, nd_recs.c rec_run /
  rec_miss / hit_apply / hit_delta). Replacing the two list runs inside rec_apply changes what a cache miss records as the delta
  that later hits replay. A wrong delta changes every map. The design must prove the cache contents equal, not only one frame.
- Why not smaller: steps A and B below share the cache proof (one keymiss census and one byte set). The C call sites that build
  the texture setup lists (make_texture_gfx, src/model.c:3498) and the custom gfx builders are follow-up Haiku rows written after
  this merges (S7MDL1.., one function group each).

## Facts (ORACLE6 code read, main 8301c42fb)
- The record (nd_recs.c:12-23 comment): R0 render mode index << 16 and texture list index; R3 flags (fog, prim, env, blend, shroud,
  depth fog); R4-R7 values (fog colour and combine entry, fog position, prim and env, blend and shroud c2); R9, R10 panner tile
  size ops; R13 the typed ops after the record (make_texture_gfx output, at most ND_REC_AUX_MAX 96 units, nd_cmd.h:53).
- rec_apply (nd_recs.c:71-) runs: lights, the bind list (nd_bind_run), combine (rec_top), the render mode list `rec_rm(i)` =
  `nd_ops_run(nd_rm_tab[i], nd_rm_n[i], 0)` (:42-46, a full list start: sc_ok 0, sr_ok 0, gv_epoch), fog (rec_fog), matrix,
  panner tiles, billboard; then `rec_aux(l, n)` = `nd_ops_run(l, n, 0)` for R13 (:59-61).
- nd_rm_tab (nd_rec.c:130-150) holds the typed units of ModelRenderModes[i] (61 lists of src/model.c) without syncs.
  S5EMDL1 made those lists S6T defs: 3ds/include/nd_rmdef.h ND_RM_LIST (Gfx_RM*_def, model.c:1264 ND_RM_DEF).
- Value units of the record (R4-R7, R9, R10) already go through rec_t/nd_op_apply: no list. They stay.

## Blocker and base
S7RUN, S6KEY and S6VORD merged (the def key and the value-order rule apply to the RM defs); S7TEX2 merged for step B.
`sh 3ds/tools/wt_new.sh S7MDL`; PM_ROW=S7MDL PM_LANE=S7MDL. Base `git merge-base HEAD main`.

## Design (fixed; the coder chooses the inner steps)
A. Render-mode lists: nd_rm_build also stores `nd_rm_def[i]` = the NdSDef of ModelRenderModes[i] (match the list pointer against
   ND_RM_LIST; a list without a def keeps NULL). rec_rm(i): with a def that analyses to st 1 and has nv == pre (no post values: the
   RM lists hold none), run the list start rules of nd_ops_run (gv_epoch, dlc_on, sc_ok 0, sr_ok 0, gp_prev_op, the streak flushes:
   the same frame S7RUN's nd_gtab_run uses), then nd_scomp_exec_own(def); else nd_ops_run as today (counted).
B. Texture setup (R13): when make_texture_gfx's output for a texture is one or two load groups that S7TEX2's tool rule accepts
   (tile 0 main, tile 1 aux), the texture handle gets its NdBindDesc pair at map load (S7TEX2's decode, run on core 1 once per
   handle, not per frame), and the record carries a pointer to the pair instead of the R13 units; rec_aux then binds through
   S7TEX2's helper (tile 0) and nd_direct_tex1 (S2W, tile 1). Any other R13 content keeps nd_ops_run (counted).
C. Proof of the cache: the keymiss census (pm_keymiss.c, benches keymiss_mac01, keymiss_kzn08, keymiss_hos01) prints the same miss
   and pre counts as the base (the record words that key the cache do not change: R0 keeps the index; B changes R13's content, so
   the key must use the pair pointer where it used the R13 words: write that in the commit). Census line `mdlrec f<n> rmdef <a>
   rmops <b> texb <c> auxops <d>` per 100 frames (pm_log_evt, counts since boot).

## Readers and writers
- R0 index and nd_rm_tab: nd_rm_build, rec_rm, nd_lstcen (census), hit_delta / hd_op (nd_recs.c:121-160: they read the ops a miss
  ran: check that a def run gives the same NdHitD), pm_keymiss.c. R13: appendGfx_model (writer), nd_rec_model (typing), rec_aux,
  hit_apply, the mc key (pm_gpu_model_key). grep `nd_rm_tab\|rec_rm\|rec_aux\|ND_REC_AUX` in 3ds and src before the commit and name
  each reader in the commit message.

## Files and places
3ds/src/nd_recs.c (rec_rm, rec_aux, the census), 3ds/src/nd_rec.c (nd_rm_build, nd_rec_model R13 part), src/model.c (make_texture_gfx
TARGET_3DS part and appendGfx_model's R13 fill only), 3ds/src/pm_keymiss.c only if a key word changes form. No ready row edits them.

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- Bases: spot_mac01_det, mdl5_iwa10_det, sprshade_hos01_pic (all normal), spot_mac00_det (turbo). One list `bench.py run
  spot_mac01_det,mdl5_iwa10_det,sprshade_hos01_pic,keymiss_kzn08 --row S7MDL --why "S7MDL: model records"` (4 starts) and
  spot_mac00_det alone (1 start): every shot byte-equal. The keymiss base value: `bench.py build-at $(git merge-base HEAD main)
  keymiss_kzn08 --row S7MDL` (1 start); the head keymiss line equals it (miss and pre counts; the KEYMISS census at writing:
  kzn_08 6246 misses per 300 frames). mdlrec on spot_mac01_det's log: rmdef above 0, rmops 0. Budget 8 (6 planned).
- CONSOLE: C4 (one batch push after S7; the orchestrator asks the user).

## Stop
A byte difference, a keymiss count that differs from the base, or rmops above 0 on mac_01: stop, commit, hand back (2 tries per
named cause). Delete this row file in the completing commit.
