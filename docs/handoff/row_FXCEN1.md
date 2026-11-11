# Row FXCEN1: per-effect cost census in fxtest (counts per slot, frame time per slot) and the ranking tool

(Model: Haiku. Base: main 337a5d4fe. Disjoint from MAPROMFS, BTLDBG, BATUI2. Reads only this file and CLAUDE.md.)

## Design (oracle code read)
- fxtest (3ds/src/pm_fxtest.c, switch fxtest=list|all|n,n,..) spawns 128 effects (FXLIST :31-159, FXT_COUNT 128) one per 44-frame
  slot from frame 400 (:179-180) on the player, on the normal draw path (pm_main.c:492, logic step tail; effects record through the
  nd_gbi.h redirect like the game). Today it only logs names and takes shots (pm_gpu_shot_burst :188, every slot, no switch test);
  3ds/tools/fxtest_report.py judges pictures (void under CLAUDE.md 2). No per-slot count and no per-slot time exist.
- Counts that do not depend on CPU speed: the phase counters pm_ph_cnt[C_*] (pm_port.h:88, PH_ADD :161: tris, vertex loads, draws,
  texture calls and misses and bytes, commands, gv triangles, rejects, fallbacks, flush KB) count only while pm_ph_on (prof sampling).
  pm_gpu_cnt_take (pm_gpu.c:3895) keeps monotonic totals (program switches, shadow resets, UpdateContext, splits, sprite memo misses,
  full combine syncs in ent) but resets on take; pm_gpu_mdl_stat (pm_gpu.c:3344) gives model cache misses and clears since boot.
- Census: a second, never-reset counter array pm_cnt_all, counted by PH_ADD while pm_cnt_on (fxtest on); two new counters for the
  native submits (draws, indices); a non-resetting reader of the pm_gpu totals; per slot the deltas from spawn (k 2) to clear (k 40)
  and the logic-thread time (pm_ticks: on the console the step is paced by the frame, so us per step is the frame time; on Azahar a
  proxy only), plus one base window without an effect (frames 360-400).

## Start
`sh 3ds/tools/wt_new.sh FXCEN1`. PM_ROW=FXCEN1 PM_LANE=FXCEN1 on every build and bench call.

## Touch only
3ds/include/pm_port.h (the C_ enum, PH_ADD, 2 extern lines); 3ds/src/pm_gpu.c (nd_native_submit, one new function after
pm_gpu_cnt_take); 3ds/src/pm_fxtest.c; new 3ds/tools/fxcen_report.py; append-only entries in azt_scenarios.json and bench_defs.json.

## Exact edits
1. pm_port.h: in the enum at :88 add `C_NDRAW, C_NIDX,` before `C_N`. Replace PH_ADD (:161) by
   `#define PH_ADD(c, n) do { if (__builtin_expect(pm_ph_on | pm_cnt_on, 0)) { if (pm_ph_on) pm_ph_cnt[c] += (unsigned)(n); pm_cnt_all[c] += (unsigned)(n); } } while (0) /* FXCEN1: pm_cnt_all never resets (fxtest census) */`
   and above it `extern int pm_cnt_on; extern unsigned pm_cnt_all[C_N];`.
2. pm_gpu.c nd_native_submit (:1115), after `NDT_B(tsub);` add `PH_ADD(C_NDRAW, 1); PH_ADD(C_NIDX, dp->nidx); /* FXCEN1 */`.
   After pm_gpu_cnt_take add `void pm_gpu_cnt_now(unsigned o[8]) { const unsigned now[8] = { <the same 8 values as pm_gpu_cnt_take> }; memcpy(o, now, sizeof now); } /* FXCEN1: totals, no reset */`.
3. pm_fxtest.c: define `int pm_cnt_on; unsigned pm_cnt_all[C_N];`. When nsel > 0 after selection: `pm_cnt_on = 1;`. shot(): call
   pm_gpu_shot_burst(1) only `if (pm_debug_has("shots"))`. Make the `fxtest: done` line pm_log_evt. A snapshot struct: pm_cnt_all
   copy, pm_gpu_cnt_now 8 values, pm_gpu_mdl_stat miss and clears, pm_ticks(), f. Take a snapshot at f == START - 40 and at k == 2;
   print at f == START (name base, index -1) and at k == 40 before clear_all (the slot's effect) two lines with pm_log_evt:
   `fxcena %d %s frames %u us %u live %d` (index, name, f - snapshot f, (ticks delta) / 268u, live effects as in shot()) and
   `fxcenb %d tri %u vtx %u draw %u tex %u texmiss %u texkb %u cmd %u gvtri %u gvrej %u gvfb %u flkb %u ndraw %u nidx %u prg %u shr %u upd %u spl %u sprmiss %u cfull %u mmiss %u mclr %u`
   (C_TRI, C_VTXLOAD, C_DRAW, C_TEXCALL, C_TEXMISS, C_TEXBYTE >> 10, C_CMD, C_GVTRI, C_GVREJ, C_GVFB, C_FLKB, C_NDRAW, C_NIDX, then the
   8 values 0-3, 5, 6 of pm_gpu_cnt_now, then the model misses and clears; all deltas).
4. 3ds/tools/fxcen_report.py <log>...: joins fxcena/fxcenb by index; per effect: per-frame value minus the base per-frame value
   for us and every count; writes build3ds/out/fxcen_<label>.txt (label = --label, default azahar): one table sorted by us, and the top
   15 by draw, ndraw, tri+gvtri, texmiss, cmd, upd, cfull, mmiss. Header line: `Azahar us is a proxy; the console decides`.
5. Scenarios fxcen_c0 .. fxcen_c7 (form of kzn04_shadestat; status unverified; speed turbo; needs_picture false; cap 45, deadline 40;
   until `fxtest: done`; first_marker `warp: mac_01 entry 0`): debug `warp=mac_01:0 pos=0:0:200 fixedseed fxtest=<16 indices> loglevel=debug`,
   chunk k = indices 16k to 16k+15. Benches of the same names (azahar_stream, turbo, default false, tags azahar fxcen). One console bench
   fxcen_all_prof: kind console_prof, host o3ds, recipe `warp=mac_01:0 pos=0:0:200 fxtest=all`, until `fxtest: done`, cap 240.
   Then `bench.py rebuild`.

## Gate (CLAUDE.md 2; Azahar budget 3 of 8)
- Build with memcheck OK; `bench.py run nd_host --row FXCEN1 --why "PH_ADD census"`: PASS; `bench.py run hotset_size --row FXCEN1 --why "PH_ADD sites"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row FXCEN1 --why "census off: no draw change"`, `bench.py cmp spot_mac01_det --row FXCEN1`: 6/6 byte-equal.
- `bench.py run fxcen_c0 --row FXCEN1 --why "census lines of 16 effects"`: rc 0; the LOG: copy has 17 fxcena and 17 fxcenb lines;
  `python 3ds/tools/fxcen_report.py <that log>` writes the table.

## Stop and end
A failed gate (fxcen_c0 without its until line in 45 s: stop, hand back the last fxcena index), an edit that does not fit, or a tool
fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file in the completing commit. Hand back the hash, gates,
memcheck line and the table file path.
