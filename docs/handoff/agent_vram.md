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
