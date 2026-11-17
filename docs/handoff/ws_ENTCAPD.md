# ws_ENTCAPD (Haiku): the 2 darkness stencil list sites get a captured entry. C1 -2, C2 -2 at most
Designer: the oracle (SendMessage). Worktree build3ds/wt/ENTCAPD, PM_ROW=ENTCAPD PM_LANE=ENTCAPD. Main at release: 743c94e8ff.
Start: after `Merge ENTSRC`. File: src/screen_render_util.c (no other page edits it).
## Objective
- Sites (FACT grep): screen_render_util.c:157 and :181, NDN(Gfx_DarknessStencilQuad), both in appendGfx_darkness_stencil.
  The list is an itab table (FACT ntab_inherit.txt, build 43b35ab465). GUESS: kgr_01 (dark_kgr01 scene) draws both sites.
- Form as ws_ENTCAPE: capture form, capture entcapd_kgr01_cap (`warp=kgr_01:0 wide loglevel=debug det drec=150:2 entcap entcapt`,
  until `drec f151 draws ...`, pm_gpu.c:618), entapply.py, ntab_ent.d/ENTCAPD.txt.
- New gate bench entcapd_kgr01_drec (the same words without the 2 switches).
## Design (docs/handoff/ws_ENTCAPD.patch, 4 files: the source, ntab_ent.d/ENTCAPD.txt, azt.d/ENTCAPD.json, bench.d/ENTCAPD.json)
FACT (oracle, 023db9ca81): `git apply --check` OK. listcen on the patched tree: build 384, gfxpos 87, recdl 13 (capture form).
## Edit shape (:157). Capture form: `gSPDisplayList(gMainGfxPos++, NDN(Gfx_DarknessStencilQuad));` -> `nd_rec_dl(NDN(Gfx_DarknessStencilQuad));`
After entapply: `nd_draw_list(NDN(Gfx_DarknessStencilQuad), pe_screen_render_util_157);` and its ND_PSO_DESC line.
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_ENTCAPD.patch | git apply -`. Commit the 4 files: `ENTCAPD 1: capture form of 2 darkness sites`.
2. `sh 3ds/build.sh > build3ds/out/entcapd_b1.txt 2>&1`. `python 3ds/tools/bench.py run entcapd_kgr01_cap --row ENTCAPD --why "capture"`.
3. Log path = `log_path` of the last runs.jsonl line of entcapd_kgr01_cap. `python 3ds/tools/entcap.py <log> > build3ds/out/entcapd_cap.txt`,
   `python 3ds/tools/entapply.py build3ds/out/entcapd_cap.txt > build3ds/out/entcapd_apply.txt`.
4. Each site: `entapply site` or `entapply so`: keep. Any other case: put the original line back. Name it in the hand-back.
5. Commit `ENTCAPD 2: captured entries of <n> sites`. `python 3ds/tools/bench.py base entcapd_kgr01_drec`.
6. `sh 3ds/tools/rowloop.sh --row ENTCAPD --bench entcapd_kgr01_drec,spot_mac01_drec --why "ENTCAPD gate"`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0, 2 cmp byte-equal. listcen build and gfxpos fall by n, recdl as main (11).
- Seeded fault (new bench): in screen_render_util.c `nd_pso_cc(PM_CC_CONST_1, PM_CC_CONST_1);` (the line before :155) -> remove it
  (uncommitted). `rowloop.sh --row ENTCAPD --bench entcapd_kgr01_drec --no-host`: cmp DIFF. Undo with git checkout of the file.
## What you may edit to fix a fail
Only the 4 files. A DIFF of the real edit: stop, send build3ds/out/loop_ENTCAPD_cmp_<bench>.txt to the oracle.
Budget: 3 builds, 5 Azahar starts.
## Stop and hand-back
Last commit `ENTCAPD 3: gate. entcapd_kgr01_drec, spot_mac01_drec byte-equal; fault DIFF; build <b> gfxpos <g>` deletes the page and
patch. One SubagentHandback: commits, applied and reverted sites with the reason, LOOP lines, listcen totals.
