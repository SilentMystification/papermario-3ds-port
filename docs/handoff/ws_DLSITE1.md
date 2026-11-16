# ws_DLSITE1 (Haiku): codemod --ntab rewrites 21 gSPDisplayList sites of v2 tables to nd_draw_list(X, ND_SELF) (C1 C2)
Designer to ask: oracle DL2 (SendMessage; the orchestrator gives the id). Worktree build3ds/wt/DLSITE1, branch DLSITE1. PM_ROW=DLSITE1 PM_LANE=DLSITE1.
Start after Merge GSETR. Parallel with ws_CGFX (no shared file). Do not edit src/model.c (ws_MODELRM owns it).
Objective (FACT, oracle dry runs: main e91cff3feb 11 sites; GSETR worktree a6b5c569f3 10 sites): 21 sites, C1 build -21, C2 gfxpos -21 (GUESS: each site is one of each).
Files (13): src/i_spy.c src/pulse_stone.c src/status_star_shimmer.c src/screen_overlays.c src/msg_draw.c, and under src/effects/: explosion.c
landing_dust.c shattering_stones.c shimmer_burst.c shimmer_wave.c something_rotating.c star_spirits_energy.c stars_shimmer.c. Do not edit any other file.
## Edit shape (the tool writes it; do not edit by hand)
Before (src/i_spy.c:51): `gSPDisplayList(gMainGfxPos++, NDN(ispy_icon_gfx));`. After: `nd_draw_list(NDN(ispy_icon_gfx), ND_SELF);`.
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dls_host0.txt 2>&1` (writes build3ds/out/ntab_tables.txt). Pass: `ND HOST TESTS PASS`.
2. `python 3ds/tools/codemod.py --dry --ntab build3ds/out/ntab_tables.txt <the 13 files> > build3ds/out/dls_dry.txt`. Count per file: dl minus
   the `gSPDisplayList`/`nd_rec_dl` keep lines. Pass: 21 in total (screen_overlays.c 8, msg_draw.c 2, each other file 1). Another number: stop, send the path.
3. Same command without `--dry`. `git diff --stat`: 13 files. Host test again (> dls_host1.txt). Pass: `ND HOST TESTS PASS`, `lkey census ok`.
   Commit `DLSITE1 1: codemod --ntab, 21 sites`.
4. One build: rc 0, 0 compile errors, memcheck margin above 0. `python 3ds/tools/listcen.py > build3ds/out/dls_lc.txt`. Pass: build and gfxpos totals go down by 21 each (vs main).
5. Gate. For spot_mac01_drec, sbk14_drec, fxr_drec: `bench.py base <b>`, `bench.py run <b> --row DLSITE1 --why "DLSITE1 gate"`,
   `bench.py cmp <b> --row DLSITE1 --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each. A cold first fxr_drec: one warm rerun.
   NOT proven by bench (GUESS): screen stencils, i_spy, pulse_stone. Their proof: the host compare of their tables (conv2c bad 0).
## Fix scope
A compile error or a byte diff names a file: `git checkout HEAD~1 -- <file>`, run step 3 on the rest, write the new count in the commit.
## Stop and hand-back
Stop: 3 gates pass. Final commit `DLSITE1 2: gate` with the values deletes this page. Hand-back: build3ds/out/handback_DLSITE1.txt
(line 1 commit hash, then gate values, sites, listcen build and gfxpos, memcheck). Any other failure after 2 tries: send the output path to oracle DL2. Then stop.
