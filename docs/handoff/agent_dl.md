# agent_dl handoff (W1 done; C1 blocked on plan; order from plan_source_opts.md)

## State
- Commit a1a3ced7d: W1 tick calibration (pm_gbi.c). Yard f599 class sum 11618 us against taskpl 12199 us (-4.8%). Ops 3576 unchanged. Picture ok.
- C1: NOT started. Blocker: the texture DL (handle->gfx) writes walker state that later draws read: timg/timg_fmt/timg_siz/timg_stride, tile_fmt/siz/tmem/pal/wrap/shift/sz, st0_ep and pm_gpu_flush/pm_tex_set_wrap (G_SETTILE), blk[] and ldrec[] (G_LOADBLOCK, G_LOADTILE), tlut_pal[], td[n].pal, cur_tile and pm_gpu_flush (G_LOADTLUT), GP stats. A one-op replay must write the same state in the same order. Opus must list those writes in the plan first.
- Guideline (docs/handoff/hw_guidelines.md): TexBind record = 64 bytes, aligned to 32, two lines. Handler small, fallback path cold/noinline.
- Not done: C1, C2 (RM), G-SYNC, BG, C1i + HUD, P1 marks + gbiprof census, G-EQ, ANIM, volcano fog A, W4+W2, W3, W6, GLYPH, pause lag, SPRITE, row (a).
- Azahar budget: row w1 4/8. Row c1 0/8.
- Tree: src/map_restore_table.c modified (user file, never commit; restore from .bak after builds). 3ds/tools/azt.ps1 and azt_scenarios.json untracked, not this lane's.

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=dl sh 3ds/build.sh`, then `cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c`.
- Yard test (W1 form): `PM_LANE=dl PM_ROW=<row> powershell -NoProfile -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Capture -Until 'opsplit f(59[9]|[6-9][0-9][0-9]|[0-9]{4,})\b' -Debug "fast preset=yard phase fixedseed shotmode=4:80"`. Log: build3ds/lane-dl/last_log.txt.
- taskpl average frames 300-599: fields $11 (bg) and $12 (main) of `taskpl f` lines.

## Next steps
1. Opus revises the C1 plan with the exact replay list and the 64-byte record layout.
2. C1 code agent, PM_ROW=c1, test: class 2 ops down 1000 or more, picture equal, pause text and file select equal. Commit message line on cache behaviour.
3. Then plan_source_opts.md order: C2 = RM, G-SYNC, BG, C1i + HUD, P1 marks and gbiprof census, G-EQ, ANIM, fog A, W4+W2, W3, W6, GLYPH, pause lag, SPRITE, row (a).

## Order v2 (supersedes step 3)
C1, C2 = RM, G1 sync skip (opf flag table), H1 cold split of gbi_run_body (pm_gbi_cold.c), BG single bind, G2 equality filter + H6 hot-state struct, H3 matrix index, C1i + HUD, H2 flush_draw cold split, fog fix, W4/W2, H5 divides + H4 prefetch, GLYPH/R5, pause lag, SPRITE, then row (a) if yard total > 38 ms. Count skips as `sk n` in opsplit.
C1 lives on main tree only (no branch, no worktree). Not started.
