# Epic S4: sprites (NPCs, player, partners) draw from records natively

## Goal
Slice S4 of the walker replacement. Same method that worked for map models (S3N, docs/handoff/agent_s3n.md): the producer writes one record per sprite component, a debug switch proves the native state equals the walker state at the same draw, then the record draws natively, then the old sprite words go. This row file replaces plan_native_slices.md rows S4-1 to S4-3 (they used a force-include redirect, not records).

## Where you work (parallel with another code agent)
You run in your own git worktree on branch s4 (created from nds1). Build lane: `PM_LANE=s4 sh 3ds/build.sh`. Azahar runs: `powershell -File 3ds/tools/azt.ps1 <scenario> -Row S4 -Build s4` (or `python 3ds/tools/bench.py run <bench> --row S4 --why "..."` if it takes the lane). Never build without PM_LANE=s4. Never edit files in the main folder C:\Users\Andrew\Documents\GitHub\papermario-3ds-port: only your worktree. The other agent (epic S3L) edits 3ds/src/pm_gbi.c, pm_gpu.c, nd_model.c, nd.h on nds1 in the main folder. To keep the merge small: put your code in NEW files 3ds/src/nd_sprite.c and 3ds/include/nd_sprite.h; in pm_gbi.c and pm_gpu.c add only small hooks (a case arm, a call). Do not edit nd_model.c or nd.h.

## What exists (grep the symbols)
- src/sprite.c `spr_appendGfx_component_flat` (grep `Gfx* texHdr = gMainGfxPos++; /* NP1`): G_PM_TEXI header plus TLUT and load words (pm_gbi.c `case G_PM_TEXI` runs tb_inline). `SPR_RM_LIST` (grep `#define SPR_RM_LIST`): G_PM_RM render mode list. `spr_appendGfx_component`: matrix and the vertex and triangle words (quad or imgfx mesh, src/imgfx.c `imgfx_appendGfx_mesh`).
- The native submit: pm_gpu.c `nd_native_submit(const NdDraw*, int tex)`. Raw ring draws (dynamic Vtx in the gring) already draw natively through nd_raw_native.
- The model pattern to copy: nd_model.c nd_model_state (state from record + compiled tables), nd_comb_decide (pm_gbi.c, combine choice with explicit inputs), pm_tex_slot_find / pm_tex_slot_unit (texture slot by content key, draw-time words), the cache (NdMCache) and the check switches (ndmcheck, ndmcache).

## Rows (one commit each)
S4-1 RECORD + CHECK. Under TARGET_3DS in sprite.c the component writes a record G_PM_SPR (add the op number in include/gbi_custom.h next to G_PM_REC; a new number) with: raster, palette, fmt, siz, w, h, the render mode list index, the matrix pointer, the vertex source (quad or imgfx mesh pointer and count). The old words still follow the record and still draw (the record arm only reads it). Debug switch `sprcheck`: nd_sprite_state(rec) builds the NdDraw state; at the walker's nd_raw_native for that sprite compare all fields (key, TEV, unit 0, uvso, cmul, cadd, zmix). Log `sprcheck f<n> recs <r> eq <e> diff <d> fb <f> why ...` every 300 frames. Gate: Toad Town (spot_mac01_pic with sprcheck on, the NPC strip and Mario on screen) eq above 0, diff 0; battle_start (a battle sprite scene) the same.
S4-2 NATIVE DRAW. nd_sprite_draw(rec, emit): vertices to the ring (word copies), the matrix into pal[0], the cached state (cache per sprite raster + palette + render mode, reset at map load), then nd_native_submit. With sprcheck also compare pal[0] and the index range.
S4-3 SWITCH. The record arm draws natively; the old words after the record are skipped (count words in the record); a sprite nd_sprite_draw refuses runs the old words (fallback, counted). Pictures right by eye: spot_mac01_pic (NPCs), spot_mac00_pic, battle_start.
S4-4 DELETE. When fallback is 0 at those scenes: sprite.c writes no G_PM_TEXI header, no old words for native sprites. Delete what lost its caller.

## Rules
Collision: only your worktree, only your lane. `git status --short` before each edit and commit. Pathspec commits only (`git commit -m ... -- <paths>`). Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push, no attribution. Never commit 3ds/src/map_restore_table.c (run gen_map_restore.py before a build; do not commit the file). Game source edits only inside `#ifdef TARGET_3DS`. hw_guidelines.md; static memory add under 128 KB (the O3DS boot is near its limit). A blocker: take the plan default, write it in docs/handoff/agent_s4.md, continue.

## Done
S4-1 to S4-4 committed on branch s4, gates pass, docs/handoff/agent_s4.md with a STATE block (10 lines or fewer), updated after each commit. Hand back after each row commit with the sprcheck line. The orchestrator merges s4 into nds1.
