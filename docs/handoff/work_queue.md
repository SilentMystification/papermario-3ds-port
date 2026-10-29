# Work queue (2026-10-29): everything not done

Rules: CLAUDE.md. One code agent at a time; one epic per agent (3 to 6 rows). The Azahar budget never blocks work: an agent that reaches the cap reports INCOMPLETE and the orchestrator resets it the same turn (rule 5). A row is dropped only when a valid gate ran and failed. Before any byte-equal gate, a HEAD-against-HEAD run must show the frames are reproducible; compare only frames HEAD reproduces (shot_cmp.py rule in test_catalog.md). No idle tests. Plans: plan_toad_33.md, plan_draw_cost.md, plan_entity_npc.md, plan_toad_model.md.

## Epic order (each starts when the previous code agent finishes)

| # | Epic | Rows | Gate | Expected console ms | State |
|---|---|---|---|---|---|
| E1 | LB1A | ARCHIVE (name pm_3ds_<build>_<hash>.3dsx, push_3ds.sh archives by itself), LB1B (lazy FrameBegin, retry on valid frames), FINAL9 (build, catalog, next push) | valid shots byte-equal; fsplit gwait down | LB1 3 to 4.5 | running |
| E2 | LOGTRIM | WARPLOG2 (per-frame spike lines to a count and max in the 300-frame summary, pace 1 per 5 s, delete list from diag_warpnoise.md; under 15 lines per warp), PRIMZFIX (prim_z - 1 at pm_gbi.c:1231 and :1886, plus Azahar rounding repro of zmix.y in pm4.v.pica on toad_pond; diag_primz.md), UNTILCHK (azt.ps1 rejects an until-line the scenario cannot print and stops a run with no new log line for 40 s), WMODEL (world model list count and render task count in the fsplit line) | log lines per warp under 15; toad_pond picture unchanged on Azahar and the repro shows the old bug and the fix; UNTILCHK blocks a bad until-line | log cost, pond fix | queued |
| E3 | PAUSEFIX | PAUSE4a (40 background strip tile loads per frame, background_gfx.c SHOW_PAUSED, to one bind), PAUSE4b (about 111 label texture changes, pause_gfx.c pause_draw_rect), PAUSE4c (time the open spike, max 112 ms; the one VRAM readback is 17.4 ms) | pause_menu gates (walk 11.1 ms, closes 135, ops 1471) fall; top shot byte-equal | pause +4.3 ms gbi | queued (diag_pause3.md) |
| E4 | MODEL | MR0 (census: plain static model tasks, us per model split ops/bind/close/dlc), MR1 (G_PM_MODEL op; runs only if MR0 shows 50 or more plain models and 120 us or more per model outside dlc), MS0 then MS1 (stable sort by texture inside one opaque static run; runs only if MS0 shows 20 or more removable closes) | 6 maps and mac01t byte-equal; prod model ops down 600 or more | MR1 3 to 7, MS1 1 to 2 | queued (high risk) |
| E5 | DRAW | DC6 (fix the cmd field, count citro3d dirty groups per close, drop repeated groups; draw stage 30.6 us), SPK1 (time pm_mem_hud; if 3 ms or more, mallinfo only when the memory rows show), LB1b (run the bg task gbi just before the main task, only if gwait above 1 ms after LB1) | dcsum draw us down 20 %; byte-equal | 1 to 3 | queued |
| E6 | SUNFIX | tile 1 values logged per ray draw on sun_sbk (diag_sunrays.md: S clamp at the window edge, pm_tex.c:120-124, pm_gbi.c:1146-1149), then the fix | sun_sbk cells and streaks gone on Azahar; console shot by the user | visual | queued |
| E7 | CONDITIONAL | CORE1 (producer on core 1 one frame ahead; only if above 36 ms after E4), FPS20 (stable 20 fps cap with full game speed; user decision) | full catalog | O3DS 2 to 4 | waits on E4 numbers |

## Waiting on console data or the user
- Post-battle slowdown (Forever Forest 51.7 ms after a battle; calm forest 34.5 ms): needs the WMODEL line from a console run after a battle (diag_batend.md, diag_forest.md). Fix row follows.
- Pond / fountain water (toad map, x -174 y 0 z 112): PRIMZFIX goes to the console; the user checks the water. Ground flicker in Toad Town: no cause yet; needs the user's photo or a texstat run (diag_toadvis.md).
- Yard before and after on the console (procedure in diag_abcmp.md).
- Picture checks the user owns: LOGCLEAN shots, cutscene box, volcano fog room, house_sign, super_text, the status bar after pause close, the 3 s steady-frame wait in the yard/sewer/ruins/volcano scenarios.
- Tree: azt_scenarios.json and test_catalog.md hold stale uncommitted changes from concurrent agents; restore from HEAD (git restore --staged --worktree on both) after the user agrees.
- Optional: git push origin --delete dlc-compiler-c1c2 (user runs it).
