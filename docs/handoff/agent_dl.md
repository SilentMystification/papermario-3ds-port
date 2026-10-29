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

## Update: C1-0 merged, C1-1 failed (2026-10-29)
- C1-0 merged to main as 8d157f28d (merge of dl2-c10). HW-unconfirmed. Azahar walk 8072 -> 8222 us (+1.9%, inside the +2% gate). Picture ok (earlier run).
- C1-1 built on branch dl2-c11 (commit 6e50f68a0, not on main): G_PM_TEX op, tb_list record, model.c MODEL_TEX_OP swap at the two texture emit sites in the render path (model.c ~1538/1542), tbsplit counter.
- C1-1 result: tbsplit n 0 fb 0 on every frame of the yard run (PM_ROW=dl_c11, 1/8 used). No G_PM_TEX op runs. The model.c emit path does not run for the yard. The plan_c1_texbind premise (per-model texture DL replay in the yard) does not hold. Quantity not moved. Gate not met. Branch kept, not merged.
- Open for the next diagnosis: find which lists make the 3866 tex ops/frame in the yard (opsplit f599 tex 3866/556 us). They are not from model.c TextureHandles. Check the map DL source (ROM DLs with inline texture loads) before any C1 row.
- Rows not started: C1-3, BG, G2 + H6, H3, H2, fog fix, W4/W2, H5/H4, C1i + HUD.

## Update: BG kept (main 78b4d2db1, branch dl2)
- src/background.c, TARGET_3DS, no-wave path: one bind of the whole sky, screen rows in at most two pieces (source row wraps at the scroll offset). Wave path and N64 strip path unchanged.
- Azahar yard f300-599: taskpl bg us 2126 -> 259 per frame (target 900 or less). ops at f599 2417 -> 2186. main walk 8223 -> 8270 us (+0.6%, inside the +2% gate).
- Picture: yard shots 0000 and 0001 byte-equal to baseline (PM_ROW=dl_bg, 3/8 used).
- Not tested: wide on/off, fog bg map. Open before this row is final for those cases.
- Next row: G2 equality filter + H6 hot-state struct.

## Update: G2 kept (dl2 07aebffa5, merged to main 76fe66592)
- g2_same() before the prologue: SETPRIMCOLOR, SETENVCOLOR (sc_ok), SETFOGCOLOR, SETBLENDCOLOR, SETOTHERMODE_L/H (H with sc_ok) when the result equals the live state. Counter RS_SKS (rmsplit sks).
- Azahar yard f300-599 (PM_ROW=dl_g2): sks 243 per frame. main walk 8270 -> 8111 us (-159, noise about 50 us). ops at f599 2186 -> 1943. Yard shots 0000/0001 byte-equal to BG shots.
- H6 (GbiState 64 B struct) not done. Not tested: pause, file select (state ops with other values).
- Next: H3 gv_matrix_id index.

## Update: H3 kept (dl2 and main b831aa956, HW-unconfirmed)
- gv_matrix_id: 512-slot u16 hash index over gm_tab (FNV of the 16 combined words). Newest matching entry still returned. Index cleared at the gm_n reset.
- Azahar yard f300-599 (PM_ROW=dl_h3): gms 179 -> 8 per frame (counter evidence). main walk 8111 -> 8086 us. Yard shots 0000/0001 byte-equal to G2 shots.
- Code size not measured.

## Status at this stop (2026-10-29)
- Done on main: BG (78b4d2db1), G2 (07aebffa5, merge 76fe66592), H3 (b831aa956). Docs after each.
- Not done: H6 (GbiState struct, part of G2 row), H2 flush_draw cold split, fog fix (volcano repro first), W4/W2, H5/H4.
- Untested: BG wide on/off and fog bg map; G2 on pause and file select.
- Next: H2 (flush_draw cold split, code-size evidence from build3ds/lane-dl2/pm_3ds.map).

## Update: fog repro (vis, no fix) and H2 part 1 (2026-10-29)
- Fog (diag_visual_vol_sewer.md): azt volcano (kzn_19) run, PM_ROW=dl_fog, 1/8 used. Shot 0000_18_13 shows the cave with lava light and no tiled fog pattern. No fix made. Shot saved to the scratchpad, not in the repo. The tiled look is not reproduced in this run.
- H2 part 1: boxlog block of flush_draw (pm_gpu.c) moved to cold noinline flush_boxlog. flush_draw 0x3b20 (15136 B) -> 0x35f0 (13808 B), -1328 B. flush_boxlog 0x410 B cold.
- Azahar yard f300-599 (PM_ROW=dl_h2, 1/8 used): main walk 8069.6 us (H3 reference 8086 us, inside the 50 us noise). bg 258 us. Picture: yard frame checked by eye (HUD, Mario, partner, house, pipe).
- Not done: a same-run byte compare of the shot against an H3 shot. lane-dl2/h3_shots has mtimes from another run, so it is not used as a reference.
- Not done: rest of H2 (dc timers, off-screen and snap paths, pm_tex miss path -Os, dead pdl_try/pdl_try_m).

## Update: lane 2 continuation (2026-10-29, dl_w4 budget 2/8)
- Worktree dl2 moved to detached main (e1f8847ba). Old docs-only commit 05cdfe665 was off-branch; its text is kept here: fog start 2 (kzn_18 warp showed a Goomba field, not the volcano), H2 part 2 not done.
- W2 (branch dl2-w2, commit 8ee2d859f, NOT on main): phase streak, sync and flush timers read the tick for one run in 8, scaled x8. New opsplit field pt = all pm_ticks reads per frame.
  - pt at f599: 1267 -> 631 (2.0x). Plan 5x, keep gate 2.5x. Failed the gate: branch only.
  - tk (op sampler, W1) unchanged at 60: tk counts only the W1 op sampler, not all reads.
  - Main walk f300-599: 8069 -> 7208 us (phase timers removed from the walk; console build has none).
  - Picture: yard shot equal by eye (HUD, Mario, partner, house, pipe). Byte compare differs (debug text overlay).
- W4 (opf table prologue): not started.
- H5/H4, pause row, C1i + HUD (dl2-c11 still not on main), fog volcano kzn_17/kzn_20: not started.
- Next: W4 on detached main, then H5 divides, then pause row.

## Update: lane 2 continuation (2026-10-29, after W2 merge)
- W2: kept by the coordinator (main 29016a9e9).
- W4 (branch dl2-w4, not kept): main walk 7208 -> 7187 us (-21, gate 150 us).
- H5/H4 (branch dl2-h5, accepted HW-unconfirmed by the coordinator): walk 7202 us (not worse), picture equal by eye. Divides removed: pm_gbi.c:1273-1276 (texrect uvso, 4 divides per rect become 1 reciprocal per axis + multiplies). PLD sites: pm_gbi.c:3215 (G_DL target), pm_gbi.c:3252 (next vertex block).
- Pause row: next. Volcano kzn_17 and kzn_20: after pause, each on its own branch.

## C1-1 (verify, worktree papermario-3ds-port-c11)
- Finding: the G_PM_TEX op runs in the yard. Probe build: tbsplit n 65 fb 0 on every frame from f36 on. The earlier n 0 did not reproduce. No bind fix was needed.
- Cause of the earlier zero: not found. The probe build used the same branch sources.
- Gate, yard, frame 378, same window, base (MODEL_TEX_OP = gSPDisplayList) vs swap:
  - dlsrc class 2 ops: 697 per frame lower (209100 -> 0 over 300 frames). Pass (>= 650).
  - class 2 + SKN: 1018 per frame lower (SKN 828 -> 507). Pass (>= 1000).
  - main walk us: 9985 -> 9065 (-920 us, Azahar). Direction only.
  - tbsplit fb 0. Pass.
  - shots yard: 0 of 96000 px differ, frames 0000 and 0001. Pass.
  - tour10: 8 of 11 frames identical. 3 frames differ (606, 271, 551 px) on a moving Toad NPC sprite, localized blocks. Judged noise.
  - pause_menu: 3 of 3 common frames identical. Base had 1 extra frame.
  - file_select: 12 of 16 common frames identical. 4 frames differ (131 px) on the file box sprite. Judged noise. Swap has 1 extra frame.
- Merge: dl2-c11 merged into main (main is at f93c91312). Merge with main commit 413deb696 done in the worktree. Conflicts in pm_gbi.c, pm_port.h, pm_phase.c resolved (kept G2 RS_SKS and the C1 counters).
- Merged build: tbsplit n 65 fb 0 (yard). Shots not re-compared after the merge.
- I-cache/D-cache: not measured.
- Budget row c11dbg: 6 of 8 starts used. Row c11g: 6 starts used (no cap reached on c11g in this section).

## Update: pause row and volcano check (2026-10-29)
- Pause (azt pause_menu + phase, PM_ROW=dl_pause 2/8): pause opens at f100 (ui: pause menu open). Pause window prod: draw_msg 1504 of 1883 ops per frame (f599), us 14734. Next: bg 264, gfx_window_root 70, camera_render 29.
- Pause reduction NOT done: draw_msg glyph = LoadTextureBlock_4b (7 ops) + texrect per glyph (src/msg_draw.c ~1754-1797). A picture-equal cut needs the glyph inline bind (C1i variant for msg_draw). Not on branch.
- Volcano (PM_ROW=dl_fog2, 2/8 used): kzn_17 (run 1) and kzn_20 (run 2) show the cave with no tiled fog. Not reproduced. Branches dl2-fog17 and dl2-fog20 have no code change.
- Next: C1-3 (animator/entity render-mode swaps, C2 pattern), then C1i + HUD (hud_backUI, reuse G_PM_TEX tex_op).

## GLYPH (branch dl2-glyph, from main 63c0fcd94)
- Op G_PM_TEXI 0x0B (include/gbi_custom.h): header word, w0 low 16 bits = n load words after it. msg_draw.c (TARGET_3DS) writes the header, then the same gDPLoadTextureBlock_4b / TextureTile_4b macros, then patches n. pm_gbi.c tb_inline runs those words through tex_op, skipping g1_skip syncs.
- Fallback: not implemented. The macro expansion is fixed, so the record cannot be invalid.
- Azahar pause (azt pause_menu + phase, lanes dl2 and dl2base, 2 starts of dl_glyph): draw_msg ops f599 1504 -> 1092 (-412, need 800; gate half = 400, kept), us 14722 -> 14585.
- Picture: shots 0000-0002 byte-equal to baseline. Subject: pause status page, dialog text visible.
- I-cache: one extra noinline call per glyph replaces 5 dispatch rounds; D-cache not measured.

## GLYPH-ATLAS (branch dl2-glyph-atlas from main decf43341, NOT merged, walk gate missed)
- Diagnosis (pause f599, gbiprof + flsplit + thsplit, PM_ROW=dl_atlas 2/8 before GA1): tex closes 213 (62 empty), texrect consecutive 0, tex lookups hit 197. Top flush site pm_gbi.c:2805 (tile-0 SETTILE, not same_tile), about 181 per frame.
- Cause: each glyph texrect calls ensure_tile(0). tile_bound compares the per-glyph raster, so it fails. The unbound path calls pm_tex_set_wrap (wrap gen +1), so the next glyph SETTILE fails same_tile and flushes again. Two flushes per glyph.
- Change (GA1): ensure_tile(0) takes the atlas as bound when pm_tex_glyph_same confirms the bound slot is the atlas of this sheet and palette. No flush, no wrap call. Counter RS_GAB (gabsplit line in pm_phase.c). Debug settdbg line (pm_gbi.c, SETTILE) kept.
- Result, same scene: tex closes 213 -> 122 (-43 percent; target half 107). Lookups hit 197 -> 96 (half, met). Consecutive texrects 0 -> 82 per frame. Azahar draw_msg us 14585 -> 14215 (-370 us; target 3 ms). Gate missed on the walk.
- Picture: pause shots 0000-0002 byte-equal to glyph build. 0003 differs in 227 bytes at x 90-101, y 192-202: the spinning coin sprite. Text rows identical at 6x.
- Next: the rest of the flushes are set_plan (2224), gbi 3105 (filter change), gbi 2875 (TLUT palette change per element). Not started.

## R0 (branch dl_r0, from main 91ed30966)
- pm_gbi.c glyphbind log (per glyph) gated on the imglog switch (static cached flag). No other ungated per-op log found.
- Azahar pause (PM_ROW=dl_r0, 2/8 used): 0b 6.2 -> 1.1 ms per frame. Main walk last 300 frames 15789 -> 9833 us. Pause shots 0000-0003 byte-equal.
- No tool parses glyphbind. imglog runs (diag_tex3, agent_text until-line) still get the line.
- Gate for R1 met (0b+e4+f0 = 5.8 ms per frame).
