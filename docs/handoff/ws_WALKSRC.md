# ws_WALKSRC (Haiku): log the start of the walker run that walks non-list memory (debug aid; counter: none, names the producer for a fix page)
Designer: oracle NSTO. Branch from main. Worktree build3ds/wt/WALKSRC, branch WALKSRC. PM_ROW=WALKSRC PM_LANE=WALKSRC.
Edit only: 3ds/src/nd_walk.c.
## Facts (code read, main 307f917e07; base ELF build3ds/basebin/07b0785e17cf38b8/pm_3ds.elf, build 6471)
- wscmb_pause_drec logs `bad dl target ... list 013ee748` (nd_walk.c:33). 0x013ee748 is pm_f127 + 4 (pm_gpu.c:1725, a float table).
- Frame 6 logs `gbi: G_MTX ... list 0x13e2aa8` (nd_walk.c:62, from :315). 0x013e2aa8 is t_blk0 (pm_dspaud.c:52), a system tick.
- nd_walk_state (nd_walk.c:391) runs words as typed units until NDO_END or n (ND_LIST_MAX 200000, :202). The log names only the bad unit.
## Edits (nd_walk.c only)
1. Above gbi_cold_bad_next (:32): `static const uint32_t* ws_q0; static unsigned ws_n0; static const void* ws_ra; /* WALKSRC: the current run */`
2. nd_walk_state (:391), first lines:
   `const uint32_t* const sq = ws_q0; const unsigned sn = ws_n0; const void* const sr = ws_ra;`
   `ws_q0 = q; ws_n0 = n; ws_ra = __builtin_return_address(0); /* WALKSRC */`
   Last line, after the `if (... WD_PH ...)` line: `ws_q0 = sq; ws_n0 = sn; ws_ra = sr;`
3. gbi_cold_bad_next (:33) before: `pm_log_e("bad dl target %08x w0 %08x list %08x", w1, w0, list);`
   after: `pm_log_e("bad dl target %08x w0 %08x list %08x run %p n %u ra %p", w1, w0, list, (const void*)ws_q0, ws_n0, ws_ra);`
4. gbi_cold_bad_mtx (:62): append ` run %p n %u ra %p` to the end of the format string and `, (const void*)ws_q0, ws_n0, ws_ra` after the
   last argument (`pc > list + 1 ? pc[-2].words.w1 : 0`).
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 1 Azahar start)
1. Edits 1-4. `sh 3ds/tools/nd_host_test.sh > build3ds/out/walksrc_host.txt`: PASS as on main. Commit `WALKSRC 1: walker run start in the bad target logs`.
2. `sh 3ds/build.sh`, then `python 3ds/tools/bench.py run wscmb_pause_drec --row WALKSRC --why "WALKSRC: run start of the non-list walk"`.
   `grep -n "bad dl target\|G_MTX w0\|load_mtx: source" build3ds/azt/logs/wscmb_pause_drec.txt > build3ds/out/walksrc_lines.txt`.
   Pass: each `bad dl target` line holds `run 0x... n ... ra 0x...`.
3. Copy build3ds/pm_3ds.elf to build3ds/out/walksrc_pm_3ds.elf (the ELF of the run).
## Fix rights: nd_walk.c; compile errors. No run line, a crash, a missing log: stop, send the output path to oracle NSTO.
Stop: last commit `WALKSRC 2: run start logged` deletes this page. One SubagentHandback with the absolute paths of walksrc_lines.txt and
walksrc_pm_3ds.elf. The oracle names the producer from the run address and ra, then writes the fix page. This branch is not merged.
