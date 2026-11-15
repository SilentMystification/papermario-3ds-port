# ws_XFT: three map change timers in the xfer2 prof line (Haiku). No behaviour change. Runs in parallel with all pages.
Objective (user item 4, map change stutter): the console prof log gives the time of the shape seam file load, the .dlc open and the entity
key compiles of one transition. Counters C1..C12: no change.
FACT console O3DS XL, build3ds/hw_xl_log.txt (2026-11-11, before S7MAP): #806 seamfix mac_00 us 103902 (src file); #809 xfer2 dlc 127/121.2,
ent 3; #810 xfer3 maxstep dlc 112.6. seam_get runs inside the timed dlc_build (nd_shape.c:375, timer :757-759). OPEN: the values of build 5672+.
Files: 3ds/include/pm_port.h, 3ds/src/nd_shape.c, 3ds/src/nd_ent.c, 3ds/src/pm_xfer.c. One commit.
## Edit 1, pm_port.h:385. Before: `    unsigned ent_n;               /* ent_compile calls */`  After (add one line below it):
    unsigned seam_t, dlcp_t, ent_t; /* XFT: ticks of seam_get (nd_shape.c), dlcp_open (nd_shape.c), first compiles of entity keys (nd_ent.c) */
## Edit 2, nd_shape.c:189 (seam_get). Before: `    unsigned us = (pm_ticks() - t0) / 268u;`
After: `    const unsigned dt = pm_ticks() - t0; unsigned us = dt / 268u; if (pm_xf.on) pm_xf.seam_t += dt; /* XFT */`
## Edit 3, nd_shape.c:261 (dlcp_load). Before: `    if (!dlcp_open()) return 0;`
After: `    { const unsigned t0 = pm_ticks(); const int ok = dlcp_open(); if (pm_xf.on) pm_xf.dlcp_t += pm_ticks() - t0; if (!ok) return 0; } /* XFT */`
## Edit 4, nd_ent.c. Add after the closing `}` of ent_compile (line 350), before `void ent_reset(void) {`:
    static void ent_compile_t(EntKey* k, const Gfx* l) { const unsigned t0 = pm_ticks(); ent_compile(k, l); if (pm_xf.on) pm_xf.ent_t += pm_ticks() - t0; } /* XFT: a first compile, timed */
Then change the two first-compile calls only. Line 367 before: `            ent_compile(k, l);` after: `            ent_compile_t(k, l);`
Line 443 before: `        ent_compile(k, l);` after: `        ent_compile_t(k, l);`. Do not change lines 488 and 502.
## Edit 5, pm_xfer.c:67. Before: `#define XF_L2 " | dlc %u/%s mdl %u/%u ent %u tex %u/%u/%s warm %u/%u"`
After: `#define XF_L2 " | dlc %u/%s mdl %u/%u ent %u tex %u/%u/%s warm %u/%u seam %s dlcp %s entt %s"`
pm_xfer.c:73 before: `                     pm_xf.ent_n, pm_xf.tex_n, pm_xf.tex_b >> 10, msf(m[7], pm_xf.tex_t), pm_xf.warm_f, pm_xf.warm_skip);`
After: `                     pm_xf.ent_n, pm_xf.tex_n, pm_xf.tex_b >> 10, msf(m[7], pm_xf.tex_t), pm_xf.warm_f, pm_xf.warm_skip, msf(m[0], pm_xf.seam_t), msf(m[1], pm_xf.dlcp_t), msf(m[2], pm_xf.ent_t));`
FACT readers of xfer2: bench.py:769 XFER2 (re.search at :784, a prefix: the new tail keeps it), azt_scenarios.json until-lines `xfer2 ... n<k>\b`
(prefix). Slot: 15 argument words (limit 16, pm_xfer.c:55-56). m[0..2] are free after the xfer line (pm_xfer.c:78 reuses m[0] the same way).
## Gates (1 build, 1 Azahar run)
1. `sh 3ds/tools/nd_host_test.sh`: PASS. 2. One build: rc 0, memcheck OK.
3. Field check, seeded fault first: write the old line `#809 xfer2 map n1 | dlc 127/121.2 mdl 231/0 ent 3 tex 53/230/25.2 warm 1/85` to
   build3ds/out/xft_seed.txt. `grep -c "seam [0-9.]* dlcp [0-9.]* entt [0-9.]*" build3ds/out/xft_seed.txt` prints 0.
   Then `PM_ROW=XFT PM_LANE=XFT python 3ds/tools/bench.py run xfer_door_kmr20 --row XFT --why "XFT: xfer2 seam dlcp entt fields" > build3ds/out/xft_run.txt 2>&1`.
   Take log_path of the last line of 3ds/logs/perf/runs.jsonl. The same grep on that log prints 5 or more (5 doors, until-line `xfer3 door n5`).
Fix a failing test: edit only the lines of Edits 1-5. Stop: gates 1-3 pass, one commit, delete this page and its INDEX.md entry in the commit.
Hand-back: build3ds/out/handback_XFT.txt (commit, host, build rc, grep counts 0 and N, one xfer2 line of the run).
## After the merge (orchestrator, not the coder)
Push a prof build to the O3DS XL .131. The user walks one map change (mac_01 to mac_00). Ingest build3ds/hw_live_log_131.txt. The oracle reads
seamfix us, xfer2 dlc/seam/dlcp/entt, xfer3 maxstep. Rule: seam + dlcp >= half of maxstep dlc: the oracle writes the load-at-map-entry fix page.
