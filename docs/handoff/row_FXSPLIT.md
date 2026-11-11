# Row FXSPLIT: per-effect GPU drawing time and core 0 walk time in the fxtest census (measurement)

(Model: Haiku. A measurement row: the effect cost table (build3ds/out/fxcen_xl3.txt) gives only the frame period per effect ("us",
pm_fxtest.c:200, tick delta of the logic step). It cannot tell GPU-bound from core 0-bound, and two console readings disagreed before
(fxcen_xl.txt outlier). Cut rows for the effects wait for two console runs of this census.)

## Design (oracle code read, main 6e171b39a)
- GPU: C3D_GetDrawingTime() (ms of the last frame's GPU drawing) is read at the frame begin by pm_ph_gpu_note (pm_phase.c:76, called
  at pm_gpu.c:2517) only in phase mode. Sum it per frame while the census runs (pm_cnt_on, pm_port.h:162).
- Core 0: gbi_walk (pm_nusys.c:99-103) times the stream walk only in phase mode. Sum the walk time in us while pm_cnt_on.
- Both sums are u32 us totals written on core 0; fxc_take (pm_fxtest.c, core 1) snapshots them (a u32 load is atomic) and fxc_print
  appends their window deltas to the fxcenb line, so fxcen_report.py gives gpu and walk us per frame above the base window.

## Next step (after try 1, code e3e7ea2b5; continue in build3ds/wt/FXSPLIT)
Fact: pm_log_evt formats into LF_STR_SIZE 160 (pm_logfmt.h:17, pm_log.c:583-585); fxcenb with gpu and walk is 164 characters and is
cut, so gpu and walk never reach the log (run 527442). Fix: (a) pm_fxtest.c fxc_print: put the fxcenb format and arguments back to
the main form (ends `mmiss %u mclr %u"` and `c.mclr - s->mclr);`); after that call add
`pm_log_evt("fxcenc %d gpu %u walk %u", idx, c.gpu - s->gpu, c.walk - s->walk);` (6 words, under 40 characters). (b) fxcen_report.py:
add `RE_C = re.compile(r"fxcenc (-?\d+) gpu (\d+) walk (\d+)")`; in parse, an RE_C match does
`rec.setdefault(idx, {"c": {}})["c"].update(gpu=int(m.group(2)), walk=int(m.group(3)))` then `continue`; the RE_B branch changes
`r["c"] = {...}` to `r["c"].update({...})` (so line order does not matter). Keep "gpu", "walk" in COUNTS. Then rerun the Gate with
the same commands (1 Azahar start; the gate now checks `fxcenc 87 gpu <n> walk <n>` and `fxcenc -1 ...` in the LOG).

## Start
`sh 3ds/tools/wt_new.sh FXSPLIT`. PM_ROW=FXSPLIT PM_LANE=FXSPLIT on every build and bench call.

## Touch only
3ds/include/pm_port.h (one line after :162), 3ds/src/pm_fxtest.c (globals, FxCen, fxc_take, fxc_print), 3ds/src/pm_gpu.c (one line
after `pm_ph_gpu_note();` in the frame begin, :2517), 3ds/src/pm_nusys.c (gbi_walk), 3ds/tools/fxcen_report.py (COUNTS, docstring).

## Exact edits
1. pm_port.h after line 162: `extern unsigned pm_fx_gpu_us, pm_fx_walk_us; /* FXSPLIT: GPU drawing and core 0 walk us, summed while pm_cnt_on */`
2. pm_fxtest.c: next to `int pm_cnt_on; unsigned pm_cnt_all[C_N];` add `unsigned pm_fx_gpu_us, pm_fx_walk_us;`. FxCen gets
   `unsigned gpu, walk;`. fxc_take: `s->gpu = pm_fx_gpu_us; s->walk = pm_fx_walk_us;`. fxc_print: the fxcenb format ends
   `... mmiss %u mclr %u gpu %u walk %u"` and the arguments end `c.mmiss - s->mmiss, c.mclr - s->mclr, c.gpu - s->gpu, c.walk - s->walk);`
3. pm_gpu.c, after `pm_ph_gpu_note();`: `if (__builtin_expect(pm_cnt_on, 0)) pm_fx_gpu_us += (unsigned)(C3D_GetDrawingTime() * 1000.f); /* FXSPLIT */`
4. pm_nusys.c gbi_walk: keep its three lines; add `const uint64_t f0 = pm_cnt_on ? svcGetSystemTick() : 0; /* FXSPLIT */` after the
   t0 line and `if (pm_cnt_on) pm_fx_walk_us += (unsigned)((svcGetSystemTick() - f0) / 268u);` after the pm_pf_nd_add line.
5. fxcen_report.py: COUNTS gets `"gpu", "walk"` at its end; the docstring says: gpu = GPU drawing us per frame (C3D_GetDrawingTime),
   walk = core 0 stream walk us per frame; both above the base window; Azahar values are not the console.

## Gate (CLAUDE.md 2-3)
- `PM_ROW=FXSPLIT PM_LANE=FXSPLIT sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run tidal_census --row FXSPLIT --why "FXSPLIT: the fxcenb line carries gpu and walk us"` (1 Azahar
  start): its LOG has `fxcenc 87 gpu <n> walk <n>` and `fxcenc -1 gpu <n> walk <n>`; then
  `python 3ds/tools/fxcen_report.py build3ds/azt/logs/tidal_census.txt --label fxsplit`: build3ds/out/fxcen_fxsplit.txt has the gpu
  and walk columns. Hand back the underwater row of that table.
- After the merge (orchestrator, user push, O3DS XL, prof profile): `warp=mac_01:0 pos=0:0:200 fxtest=all` twice; fxcen_report.py on
  both logs (labels xl4a, xl4b). The two runs must agree within 15 % on us, gpu and walk for the top 15 effects before any cut row.

## Stop
A failed gate, an edit that does not fit the code, or a test-tool fault: commit, then hand back the exact output (CLAUDE.md 4-5).

## At the end
Delete this row file in the commit that completes the row. Hand back: the ready hash, the gate results, the memcheck line.
