# ws_C1MIC (Haiku: closed edits, byte-equal): core 1 snap build and list walk do less copy and less repeat work
Designer: oracle NATIVE15 (plan_perf.md section 6, ARM11 quality). Branch from main. Worktree build3ds/wt/C1MIC, branch C1MIC. PM_ROW=C1MIC PM_LANE=C1MIC.
Facts (main 0b1ba0f322; base logs build3ds/basecache/0b1ba0f322.../*/run.log):
- snap_build0 (nd_snap.c:49-85) runs about 100 times per step (snb 20190 to f201, spot). Each call clears a local NdCombOut (:53, about 370 B, GUESS size).
  Every decision path writes all of o: the cache copy (:71), nd_pso_find_i on a hit (nd_psolk.c memset), nd_comb_core1 (nd_decide.c:235 memset). So :53 is dead work.
  A new snap then copies o into r.o (:79): one more 370 B copy.
- The decision cache has 32 entries (:28). spot dec 2580 dhit 7546, entcape dec 3812 dhit 6206 (FACT snapi f201): 25 and 38 percent of the new snaps still decide.
- ds_pairs (nd_ds.c:432-434) calls the pure nd_ds_ep_rule twice for the same unit when a copy and a list table are open.
Objective: fewer bytes and calls per core 1 unit and snap. Exact by construction. c0gate stays 0 / 0 / 1.
Proxy (Azahar, base against tip at f201): snapi `dec` falls (bigger cache). Bytes per snap_build fall by 370 (code fact). Core 1 ms needs the console.
Edit only (line numbers main 0b1ba0f322): nd_snap.c:28, :50-53, :70-82; nd_ds.c:432-434. Sources are CRLF.
## Edits
1. nd_snap.c:28 `#define SD_N 32u` becomes `#define SD_N 128u /* C1MIC: about 66 KB on the heap at first use (MDLBLK freed 256 KB) */`.
2. nd_snap.c: delete `NdCombOut o;` (:51) and `memset(&o, 0, sizeof o);` (:53). In :70-82 write the decision straight into r.o:
   - :71 `... nd_sd_hit++; r.key = de->key; o = de->o; }` becomes `... nd_sd_hit++; r.key = de->key; r.o = de->o; }`.
   - :73-77: every `&o` becomes `&r.o`, every `o.plan_on` and `o.plan` becomes `r.o.plan_on` and `r.o.plan`, and `de->o = o;` becomes `de->o = r.o;`.
   - Delete :79 `r.o = o;`. :80-81 `sn_leg = o.leg; sn_mode5 = o.mode5; sn_pat = o.pat;` and `r.plan_on = o.plan_on;` read r.o instead of o.
   r is cleared at :52, so r.o is zero before nd_pso_find_i, as o was.
3. nd_ds.c:432-434 before: the 3 lines with 2 calls of nd_ds_ep_rule. after:
   `const int lost = s->sc_lost, dirty = s->sc_dirty; const unsigned ch = nd_ds_apply(s, w0, w1);`
   `if (nd_alt_raw_on | nd_lsc_on) { const unsigned rc = nd_ds_ep_rule(w0, ch, lost, dirty); /* C1MIC: once */`
   `    if (nd_alt_raw_on) nd_alt_mark(rc); /* NAT6B */ if (nd_lsc_on) { if (rc) nd_lsc_ep(); nd_lsc_unit(w0); } } /* NCONV2C */`
## Checklist (hw_guidelines.md)
- No new loop, no division, no double. The cache stays direct-mapped (one probe). One heap block for the life of the game (no per-frame allocation).
- memcheck must stay OK (the cache is heap, not static).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `sh 3ds/tools/nd_host_test.sh > build3ds/out/c1mic_host.txt` (timeout 600000): PASS (nst1, nset1, nat5a link nd_ds.c). Commit `C1MIC 1: snap build and ds_pairs trims`.
2. `PM_LANE=C1MIC sh 3ds/build.sh` (memcheck OK); `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
3. Base, run and cmp (`bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec,entcape_kmr03_drec --row C1MIC`, `bench.py cmp`): every shot byte-equal.
   Pass also: last snapi line has new + hit = snn and dec + dhit = new; tip dec below base dec on both benches.
Fix rights: the edited lines and compile errors. A DIFF or a broken sum: stop, send the output path to oracle NATIVE15.
Stop: last commit `C1MIC 2: gate` deletes this page. One SubagentHandback: commit hash, host, memcheck, c0gate, cmp, the snapi lines of base and tip.
