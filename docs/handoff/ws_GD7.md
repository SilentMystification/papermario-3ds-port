# ws_GD7 (Sonnet): status bar draw memo: an unchanged draw_box, HUD rect or number copies its last stream range (GAMEDRAW row 7)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.4, 4). Worktree build3ds/wt/GD7, branch GD7. PM_ROW=GD7 PM_LANE=GD7. Start: main 353b31c792 or later.
WAIT: start on the user's Sonnet yes and oracle NATIVE17's yes for the stream range copy (nd_rec.c is its recorder). Do not edit nd_rec_alt or nd_rec_model.
Why Sonnet: the memo key must hold every input of a draw call and the hit must repeat its side effects; a census names what is missing (a design loop, as ws_DO1).
Objective (FACT live_cap2_170.txt): c1G mode 5.50 ms per sampled step, c1u mode 357 units, c1v mode 24 alt units, c1x mode 1.19 ms. mode is the status bar:
state_drawUI_world (src/state_world.c:21) -> update_status_bar (src/inventory.c:571): 2 draw_box (:756-757, src/draw_box.c:351: about 40 setter units,
texture loads and rects each), about 12 HUD elements (:782-901, hud_element.c:637-879) and 2 numbers (status_bar_draw_number :471). The values do not change
while the bar stands. Counter: console c1G mode, c1u mode, core 1 busy; GUESS -2 to -4 ms core 1 per step. Stream bytes unchanged.
## Design
- New file 3ds/src/nd_rmemo.c: `int nd_rmemo_begin(unsigned site, const void* key, unsigned n)` and `void nd_rmemo_end(void)`. begin hashes (site, key bytes),
  compares the key, the NdDs before (memcmp of nd_ds) and the state the census names. Hit: copy the stream range the call wrote last step (the other
  context's stream stays intact until rerecorded: ws_DO1 fact), patch its address words, set nd_ds to the stored after value, repeat the side effects,
  return 1 (the caller skips its body). Miss: return 0; end stores the range, the after state and the side effects. 32 entries on the heap.
- Call sites: draw_box (key: its arguments; only when fpDrawContents, rotScaleMtx and outMtx are NULL and rot and scale are 0), hud_element_draw_rect
  (key: the NdHudRec after it is built, hud_element.c:736-753 and its pieces), status_bar_draw_number (key: its arguments and the digit HUD element state).
- Side effects a hit repeats (FACT code read): draw_box writes matrices to gDisplayContext->matrixStack[gMatrixListPos++] (draw_box.c:472, :497, :789) and the
  stream points at them: store the matrices and write them to the new slots, advance gMatrixListPos, patch the pointer words. The census lists others.
## Steps
1. `GD7 1`: the full path always runs. In drec frames, on a key hit, build the copy into a scratch buffer and memcmp it with the real range; memcmp nd_ds
   after and the side-effect state (gMatrixListPos, the matrix slots). A difference logs `rmemo f<n> site <s> diff <off>` (NdDs offset by offsetof, or
   `blk <word>`, or `mtx <k>`). Add the field to the key, the patch list or the side effects; rerun. 3 rounds per site, then send the lines to oracle NATIVE18.
   A site that keeps a diff after 3 rounds stays on the full path (note it in the commit).
2. `GD7 2: gate`: the hit path copies. The drec-frame compare stays.
## Checklist (hw_guidelines.md 2, 3, 7): heap table (32 x (key 400 B + range up to 2 KB), about 80 KB GUESS), one sequential copy per hit, no division.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. Step 1: hudfb_drec, spot_mac01_drec byte-equal; `grep -c "rmemo f.*diff"` 0 for the kept sites; note the hit counts. Step 2: the same 2 benches byte-equal.
Fix rights: compile errors; key, patch and side-effect fields from a census line (3 rounds per site). A DIFF in step 2, a crash: stop, send the path
to oracle NATIVE18.
Stop: `GD7 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, cmp lines, rmemo lines, the sites kept and the fields added.
