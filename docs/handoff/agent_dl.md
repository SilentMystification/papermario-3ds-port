# agent_dl handoff (lane 2, after C2)

## State
- Worktree: C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-dl2, branch dl2. Build: PM_LANE=dl2 sh 3ds/build.sh. Assets copied (git-ignored).
- Main commit a6c525673 = C2 (cherry-pick of dl2 85d2d5b92). Kept.
- C1: NOT started. Blocker: plan_dl_architecture C1 needs the list of walker writes the texture DL makes (make_texture_gfx, model.c ~3219-3420: LoadTLUT, SetTile, LoadBlock/LoadTile, SetTileSize, SetTImg, combine, othermode). Those writes hit tile_*, blk[], ldrec[], tlut_pal[], td[], cur_tile, pm_gpu_flush and pm_tex_set_wrap (pm_gbi.c 3190-3290). The record approach needs an Opus plan with that list. Not in any plan file yet.
- C2 result (dl_c2, Azahar yard, frames 300-599): ops/frame 3576 -> 3236; main walk avg 9812 -> 9512 us (-301 us, plan 760 us). Fallback count 0 of 68.
- C2 first try (equality skip, no effect when state differs) gave 0 skips of 68. Reason: the texture DL before each render mode list changes cycle type or filter bits. Do not retry the skip.
- Debug tools kept: rmsplit line (pm_phase.c), RS_RMN/RS_RMSK counters (pm_port.h).

## Budget
- PM_ROW=dl_c2: 5/8 used.

## Next steps
1. Opus plan for C1 (texture bind record, write list above). Then C1 code agent.
2. G1 sync skip (opf[256] before the prologue, count as sk n). Unblocked. Test: yard opsplit sync n to about 0.
3. Then order v2: H1 cold split, BG single bind, G2 + H6, H3, C1i + HUD, H2, fog fix, W4/W2, H5/H4.

## Commands
- Test: PM_LANE=dl2 PM_ROW=dl_<row> powershell -NoProfile -File 3ds/run_azahar.ps1 -Force -Timeout 90 -LockWait 180 -Capture -Until 'opsplit f(59[9]|[6-9][0-9][0-9]|[0-9]{4,})\b' -Debug "fast preset=yard phase fixedseed shotmode=4:80"
- Log: build3ds/lane-dl2/last_log.txt. Shot: build3ds/lane-dl2/last_shot.png. Use -Force, or the run returns a cached result.
- Average: grep "taskpl f" log | awk '{f=substr($3,2)+0; if(f>=300&&f<=599){b+=$11; m+=$12; n++}} END{print b/n, m/n}'
- Before a build: python 3ds/tools/gen_map_restore.py; after: git checkout -- 3ds/src/map_restore_table.c in the worktree.

## Update: G1 kept (main 9268569ae)
- G1 sync skip: ops/frame 3236 -> 2417, sync 819 -> 0. Main walk avg 9512 -> 8072 us (-1440 us). Picture ok. PM_ROW=dl_g1 at 1/8.
- Next: H1 cold split of gbi_run_body (pm_gbi_cold.c, -Os). Test: walk not slower, picture ok.
- C1: check docs/handoff/plan_c1_texbind.md; take it when present.

## Update: H1 kept (main 3449398b0, dl2 ddfea6e6d)
- H1 cold split: gbi_run_body 0x5cac -> 0x56e8 (-1476 B). Rare paths in gbi_cold_* helpers (cold, noinline). Compiled in pm_gbi.c, not pm_gbi_cold.c with -Os (deviation).
- Azahar yard f300-599: main walk 8072 -> 8088 us (noise). ops 2417 unchanged. Picture ok (PM_ROW=dl_h1 at 1/8).
- Not reached: 8 KB target. Remaining inline cases: TEXRECT, SETTILE, LOADBLOCK, LOADTILE, LOADTLUT, SETTILESIZE tail (~3166-3420). Next H1 step if wanted: move the tile/texrect tails into cold-or-noinline helpers.
- Worktree dl2 is detached at main. Build outputs in build3ds/lane-dl2.
- Next rows in order: BG single bind, G2 + H6, H3, C1i + HUD, H2, fog fix, W4/W2, H5/H4. C1 still waits on docs/handoff/plan_c1_texbind.md (not present).

## Update: H1b and C1-0 (not kept on main)
- H1b (tile cases SETTIMG..SETTILESIZE to gbi_tile_op): gbi_run_body 0x531c, gbi_tile_op 6 KB. Walk 8174 us vs 8088 (+86). Reverted, not committed.
- C1-0 (tex/combine cases to tex_op, noinline): gbi_run_body 0x4c24 (19.5 KB), tex_op 0x1f14 (7.9 KB). Walk 8222 us vs 8072 (+150). Ops 2417 unchanged. Picture ok. Not on main. Saved as branch dl2-c10 (commit in the dl2 worktree repo).
- Cause: one extra noinline call per tex and tile op. C1-1 needs tex_op. Next: find out whether the call cost or the switch layout costs the walk (e.g. always_inline into a second switch vs a jump table).
- Next: C1-1 needs C1-0 accepted or the same handler split on main; C1-3 after C1-1.
- Remaining rows (BG, G2/H6, H3, H2, fog, W4/W2, H5/H4) not started in this session.
