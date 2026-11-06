# Row DRAW2: S3L wide and code 2, then the S4-1 sprite key fault (main folder, main lane)

## Start state
Branch nds1, main folder. S3L zmark passed (gates1.md gate 6: ndmcheck diff 0, zmark refusals 647 -> 0). Read docs/handoff/agent_s3n.md (STATE top, S3L lines, B4 to B10), docs/handoff/agent_s4.md (STATE), docs/handoff/gates1.md, docs/handoff/row_S3L.md and docs/handoff/row_S4.md (rules and gates).
Another code agent (row MENU2) works in this same folder at the same time on 3ds/src/pm_warp.c, pm_options.c, pm_options_ui.c, pm_main.c, pm_input.c and the menu functions of pm_gpu.c. Do not edit those. It builds in lane menu; you build in the main lane. Keep every edit buildable and commit quickly.

## Rows (one commit each)
1. S3L wide (class c). The refusal: pm_wide && !off_active && (ui_ortho || matrix with m[0][3] == m[1][3] == m[2][3] == 0) (pm_gpu.c pm_gpu_grange_native_would). Read what the walker CPU path does for such vertices (draw_tri_cpu; the F07 code near pm_gpu.c `F07-7` and `x *= 1.25f`). If for the non-ui_ortho case the result is a fixed X scale and offset of the clip position, give the native draw a post matrix variant with that map (rebuild_3d view.r[0] values without the 1.25). If it is a per-vertex edge test, keep the class as named fallback `wide_edge` with the reason in agent_s3n.md. Gate: wide refusal count (ndmr) drops at iwa_10, ndmcheck diff 0, iwa_10 and mac_01 pictures right by eye.
2. S3L residual code 2 (R13 with a bind: LOADTLUT and tile state): decode into the desc with nd_words. Gate: code 2 count drops at mac_00 (422 per window), ndmcheck diff 0.
3. S4-1 sprcheck fault (gates1.md gate 5): `sprcheck f300 recs 3121 eq 0 diff 2080 dfield 1` (field 1 = template key). Print the first 10 differing keys field by field (native key vs walker key at nd_raw_native), find the field, fix nd_sprite / the record (src/sprite.c TARGET_3DS) for it (max 2 tries per field). Gate: sprcheck eq above 0, diff 0 at mac_01.
Then S4-2 to S4-4 as in row_S4.md.

## Commands
iwa_10 runs: switches of spot_mac01_pic (`python 3ds/tools/bench.py show spot_mac01_pic`) with warp=iwa_10:0 pos=-662:0:587 (or bench spot_iwa10_* when bench.py list shows it), add ndmcheck or sprcheck as needed. Build: `python 3ds/tools/gen_map_restore.py; PM_ROW=DRAW2 sh 3ds/build.sh`; restore map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Status line in 3ds/logs/perf/status_DRAW2.txt before each Azahar start. Budget reset allowed: powershell -NoProfile -File 3ds/run_azahar.ps1 -ResetBudget DRAW2.

## Rules
`git status --short` before each edit and commit. Pathspec commits only. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`, `bench.py build-at`. No toggles (debug switches allowed), no stubs, no git push, no console push, no attribution. No vertex shader change (seam rules). Static memory add under 128 KB. Update agent_s3n.md / agent_s4.md STATE after each commit. Hand back after each row with the counter lines.
