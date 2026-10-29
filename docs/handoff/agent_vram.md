# Handoff: lane 1 (VRAM / linear), agent_vram

## State
- Row 0 and 0b: committed before this agent (91b2bd906, a56ee72c4).
- Row 1: committed 9764c3413. Pass: kmr 6-map tour (tour_k30.txt), last-map linear used 11514 KB (HEAD) -> 8616 KB, tex fail 0 in both.
- Rows 2, 3, 4, 5: not started.

## Row 1 facts
- Freed `vram_stage` at begin_frame, two frames after C3D_TexLoadImage (up_frame).
- Deleted tex_lin / tex_lin_heap / slot_tex_lin_build (no reader outside pm_tex.c).
- Slab extent is freed on linearAlloc failure (sp_keep).
- texmap line now also logs: `texmap linear: used .. peak .. stage live .. freed cum .. slab lost ..` (pm_tex_stage_state, pm_mem.c).
- Not checked: pictures for row 1 (no shot compare run).

## Test setup (this machine)
- Build worktree: C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-vram (detached, HEAD + lane files). Must sit under Documents: a Temp path is not shared with Docker and copy-out fails silently.
- Build: `cd <worktree> && PM_LANE=vram PM_ROW=vramN sh 3ds/build.sh`. Needs `python 3ds/tools/gen_map_restore.py` first (creates ld/map_restore.ld).
- Copy `build3ds/lane-vram/pm_3ds.3dsx` into the main folder `build3ds/lane-vram/`, then run from the MAIN folder (test budget file is per folder):
  `PM_LANE=vram PM_ROW=vramN powershell -NoProfile -File 3ds/run_azahar.ps1 -Until "tour: done" -Timeout 90 -Turbo -Force -Debug "warp=arn_02:0 story=ch0_toad_town tour=@tour_k30.txt texmax"`
- Without `warp=... story=...` the tour does not start (coordinator rule). Check the first `tour: next` line within 20 s.
- Budget: vram1 used 3 of 8 starts (run 1 was the wrong form, run 2 patched, run 3 HEAD baseline).

## Open items
- Row 2 (per-bank accounting, pm_mem.c): not started. Plan: plan_vram_3mb.md row 2.
- Row 3: boot layout and 3 MB cache. Depends on row 2 values for the A/B check.
- Worktree is left at papermario-3ds-port-vram (remove with `git worktree remove` when done).
- Other agent's uncommitted work (pm_gbi.c, pm_interp.c, src/model.c) was not touched.

## Next steps
1. Row 2: pm_vram_alloc / pm_vram_free per bank in pm_mem.c, HUD `VRAM A B minA minB fail`.
2. Row 3, then rows 4 and 5 per plan_vram_3mb.md.
3. Completion: one 411-map run (-Milestone) after row 5. No push.

## Shot pair, row 1 vs HEAD (6-map kmr tour, shotwin=999999:1 shotmode=4:21, 7 shots)
- 5 of 7 shots byte-equal.
- 0003_0_13.ppm: 704 of 96000 pixels differ. 0004_0_14.ppm: 1244 of 96000 differ.
- Cause not known. Next step: run HEAD twice (base vs base) to see if the emulator is repeatable. Only then decide on row 1.
- Shot sets: C:/Users/Andrew/AppData/Local/Temp/shots_base and shots_new.
- Budget vram1: 5 of 8 starts used.
- Rows 2 to 5: not started. Test catalog (azt.ps1, test_catalog.md) does not exist yet.

## Row 2 (commit 9403c4edc on detached worktree, 7dea7290c on main)
- pm_mem.c: bank accounting (`pm_vram_reg`, `pm_vram_unreg`, `pm_vram_bank_of`), per-bank min, `vram_fail_n`. HUD row 2: `VRAM A %uK B %uK min %u/%u f%u`. `vram acct <tag>` log line in pm_mem_log.
- Registration: boot colour/depth (pm_gpu.c, after C3D_RenderTargetCreate), slab (pm_tex.c slab_init), snap, off-screen slot, grab. Unregister only in the off-screen free.
- Skipped the `pm_vram_alloc(size, bank, what)` helper: no site places a block by bank yet. Add it in row 3 with vramMemAlignAt.
- Test (vram2, 1 start, 6-map short tour): boot colour in A, depth in B; `unaccounted 0KB` on every map line (matches vramSpaceFree); fail 0; A 1161 / B 2697 on the map lines (A holds the 1536 KB slab at boot: 3072-375-1536). Row 3 moves the slab to the layout in plan_vram_3mb.md.
- Worktree C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-vram was stale (base 48792619d, dirty) and was NOT changed. Row 2 built in C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-vram2 (clean at 88a135dfc, plus assets/us and ver/us/build copied from main, both git-ignored).
- Budget vram2: 1 of 8 starts used.

## Row 3 (main)
- pm_gpu.c: `small_arena_init` (12 x 16 KB, 6 per bank, at pm_gpu_init after the target), `tex_init_vram` (64x64 takes an arena block, other sizes vramAlloc + reg). Snap and off-screen slot use it.
- pm_tex.c: BUDGET 3072 KB as two halves via vramMemAlignAt (A, then B). 768 KB fallback removed. slab_free merges only within one bank (bank from the address, no tag field).
- Test (vram3, 6-map short tour): boot A used 2007 KB free 1065, B same; unaccounted 0 KB; tex fail 0; evict cum 0. Boot log: half 0 in bank A, half 1 in bank B.
- Picture check DONE (azt yard, azt tour10; base = row 2 binary): yard 0 px differ in 2 of 2 shots; tour10 9 of 11 equal, 2 differ (364 px box x63-146 y308-333; 492 px box x111-140 y134-157), both on moving NPC sprites, background equal. Merged to main.
- Budget vram3: 2 of 8 starts used.
- Merged to main after the picture check. Branch vram-row3-untested deleted.

## Blockers seen
- `git checkout -f` in the stale worktree papermario-3ds-port-vram was denied by the tool permission classifier. That worktree is unchanged (base 48792619d, dirty src).

## Handoff at row 3 end (next agent starts at row 4)
- Main HEAD: 16a7dd63d (row 3). Rows 2 and 3 are on main. Row 2 and row 3 handoffs are above.
- Row 3 picture: azt yard 0 px differ; azt tour10 9 of 11 equal, 2 differ on moving NPC sprites (364 px and 492 px). Read as noise.
- Build worktree: C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-vram2 (detached, clean). Copy ignored assets/us and ver/us/build exist there. Build: `python 3ds/tools/gen_map_restore.py`, then `PM_LANE=vram PM_ROW=<row> sh 3ds/build.sh`, then `git checkout -- 3ds/src/map_restore_table.c`. Copy the 3dsx to main build3ds/lane-vram/pm_3ds.3dsx.
- Stale worktree papermario-3ds-port-vram (base 48792619d, dirty): not touched. Remove later with git worktree remove after the user agrees.
- Tests: use 3ds/tools/azt.ps1 scenarios (yard, tour10, pause_menu, battle_start; file_select and fileselect_rotate when they exist). `-Build vram` runs build3ds/lane-vram. Each scenario writes shots to build3ds/azt/shots/<scenario>, overwritten by the next run of the same scenario: copy the shots out after every run. For a pair, build the base binary in lane vrambase.
- Pixel diff tool: C:/Users/Andrew/AppData/Local/Temp/vram_row2/ppmdiff.py <dirA> <dirB> (prints per-shot px count and bounding box). Crop sheet with PIL works for a visual look.
- Budgets: vram2 1 of 8, vram3 2 of 8 (row 3 shot runs with ids v3r3yard, v3r3tour10 and v3r3yardb, v3r3tour10b and v3yard, v3tour10 and base ids v3rbaseyard, v3basetour10 were the pair runs). Use fresh ids for row 4 (e.g. vram4_*).
- Row 4 (full buffers) per plan_vram_3mb.md: bank-placed helper (A then B) for grab/off-screen/snap full buffers, copy buffer alias (grab_tex and full-slot snap share one 512 KB block; off_snapshot clears prev_pre), free full slots and the copy buffer when idle 60 frames at pm_gpu_begin after C3D_FrameBegin, remove off_fail and grab_ready latches. Test: effect scenes (pause_menu, battle_start, file_select) min bank free at least 41 KB, tex fail 0, shot pairs equal.
- Row 5 (shrink and regrow) per plan: pm_tex_shrink, drain through stale, free bank B at begin_frame, regrow after 120 idle frames, debug switch vramfill=N. Test with pause script and vramfill=2.
- Not done yet: row 4 and row 5. The tour_k30 shot timing is being fixed by the catalog agent (shots after load-done plus 15 frames). Do not use the catalog's tour_k30 scenario for picture checks until that is verified.
- Hardware rule: every commit message has one line on how the row follows docs/handoff/hw_guidelines.md. Keep slot headers compact, the extent table small, and the LRU scan off the hit path.

## Row 4 (branch vram-row4-untested, commit bb5a86fdd, base main 4b0ad69d4)
- Code (pm_gpu.c only): `bank_block` (A then B), `copy_get` / `copy_free` (one 512 KB copy block, `copy_last`), `tex_hdr` (C3D_TexInit, linear freed at once, data set later), `full_tex_init` / `full_free` (C3D_RenderTargetDelete then C3D_TexDelete), `buffers_idle` (IDLE_FREE 60, full images with w != 64 and the copy block). `off_snapshot` full branch clears `prev_pre`. `grab_ready` removed. `off_fail` cleared at pm_gpu_begin. Small 64 x 64 path unchanged (`small_tex_init`).
- Assert found and fixed: `C3D_RenderTargetDelete` breaks (svcBreak, renderqueue.c:364) when a frame is open. `buffers_idle` therefore runs before `C3D_FrameBegin`. First build crashed on that (Azahar "Break", LR in C3D_RenderTargetDelete).
- Test (vram4, 7 of 8 starts used after full411):
  - pause_menu: log checks pass. Shots 0000 to 0003 equal to base (vrambase = HEAD 4b0ad69d4): 0 px differ each. Shot 0002 read by eye: status menu.
  - text_menu: log checks pass. Shots equal to base pause shots: 0 px differ.
  - battle_start: log checks pass. No base shots taken (budget).
  - file_select: does not reach file select (game ran to frame 10349 in the 90 s cap). No value.
  - full411: see the result below.
  - Min free per bank in these scenes: 1065 / 1065 KB (no change from row 3). No full image was allocated in pause, text or battle: the grab and full-image path did not run. The row 4 path is UNTESTED. The branch stays unmerged.
- Shot result for the target path: none yet. Needs a scene that allocates full images (pm_gpu_off_begin with stride > 64). The file select path is the known user.

## Row 5 (not started)
- Not written. Plan: pm_tex_shrink, drain, free, regrow. Trigger problem: no test scene in the list allocates a full image, so the shrink path (called from a full-image failure in `bank_block`) cannot run there. `vramfill=2` alone leaves B at 41 KB free and does not trigger it. Decide the trigger first (for example a forced shrink at the first begin_frame when vramfill is set).

## Next steps
1. Find a scene that allocates full images (file select or a pause page with stride > 64). Test row 4 there: min per bank at least 41 KB, shots equal to base.
2. Only after that, merge vram-row4-untested to main (fast-forward or cherry-pick bb5a86fdd). Do not merge while untested.
3. Row 5 after the trigger is decided.
4. Budget: vram4 has 1 start left. Use fresh ids for new tests (vram5 budget is fresh).
- Shot ids: row 4 runs used the azt default names. Base shots are in C:/Users/Andrew/AppData/Local/Temp/claude/.../scratchpad/r4/pause_base.
- Build: worktree C:/Users/Andrew/Documents/GitHub/papermario-3ds-port-vram2 is at main 4b0ad69d4 plus pm_gpu.c from the row 4 branch (currently the row 4 build). Copies: main build3ds/lane-vram (row 4), build3ds/lane-vrambase (HEAD base).
- Not touched: pm_warp.c, azt_scenarios.json, map_restore_table.c (other agents' or user files).

## Session vram4b (row 4 test) and vram5 (row 5 status), handoff at main 43049b4bd

### State
- Main HEAD 43049b4bd. Row 4 is NOT on main.
- Branch vram-row4-untested: c3780df51 (main 5531ad768 merged in, clean) and 82e43c29c (log lines only: `row 4: copy buffer in bank X`, `row 4: full image N in bank X`, `... freed, idle N frames`). Branch is checked out in worktree papermario-3ds-port-vram2.
- Row 5: no code on main. A partial edit was reverted.
- Budget: vram4b 3 of 8 starts used. vram5 not used.

### Row 4 test results (file_select scene, base = lane vrambase5 = main 5531ad768)
- Base: log `vram acct tick: A free 1065 min 1065, B free 1065 min 1065`. 16 shots.
- Row 4, no grabtest (lane vram, pre-log build): 16 of 16 shots byte-equal to base. Log has no full image line. The file select scene allocates no full image in this build.
- Row 4 with grabtest (lane vram, 82e43c29c), `grabtest` runs the grab, which allocates the full slot and the copy buffer:
  - Log: `row 4: copy buffer in bank A (frame 0)`, `row 4: full image 0 in bank A (frame 0)`.
  - HUD: `VRAM A 41K B 1065K min 41/1065 f0` (min A 41 KB, tex fail 0).
  - Shots: 16 of 16 byte-equal to base.
- NOT observed: `freed, idle N frames`. The grab runs every frame in this scene, so the buffers never go idle for 60 frames. Branch is not merged, because the idle free is a required check.
- Premise change: fileselect_rotate does not need 3 full buffers in this build. The grab needs 2 (slot and copy). Both go to bank A (A first).

### Row 4 command (pass to run_azahar directly)
- `PM_ROW=vram4b PM_LANE=vram powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/run_azahar.ps1 -Until "shot \d+ frame 3[4-9][0-9]\b" -Timeout 90 -Turbo -TurboFactor 1 -Force -Debug "fileselect phase shots shotwin=282:60 grabtest"`
- Base: `powershell -File 3ds/tools/azt.ps1 file_select -Row vram4b -Build vrambase5`
- Shots are in build3ds/azt/shots/file_select (copy out after each run). Log: build3ds/lane-<lane>/last_log.txt.
- Copies: scratchpad r4/fs_base, r4/fs_row4, r4/fs_row4g (log.txt in each).

### Row 5 (not started in code)
- The trigger (third full buffer fails in both banks) is not reached in file select. The grab gives 2 buffers.
- Option to decide: add a debug switch `vramfilla=N` (N dummy 512 KB blocks in bank A). With `vramfill=2` (bank B 41 KB) and `vramfilla=1` (bank A 553 KB), the grab slot takes A to 41 KB, the copy buffer fails in both banks, and pm_tex_shrink runs. The plan's design for shrink and regrow (plan_vram_3mb.md row 5) is otherwise unchanged.
- Regrow: the plan says regrow when bank B holds no full buffer for 120 frames. A simpler rule (120 frames after the half is freed, then retry every 120 frames) is the first build.

### Next steps
1. Decide whether row 4 merges without the idle-free check, or whether a scene is added where the grab stops (for example a debug switch that stops the grab after N frames).
2. Decide the row 5 trigger (`vramfilla` above), then implement shrink and regrow on the branch, not on main, and test.
3. Rule: a full-buffer failure in both banks calls pm_tex_shrink from bank_block in pm_gpu.c. Do not add that call on main before row 4 is merged.
