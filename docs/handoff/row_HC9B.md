# Row HC9B: software prefetch in the stream replay loop (plan_hotcode.md HC9, the part with no HC6 file)

(Model: Haiku. The agent reads only this file, CLAUDE.md and what this file names.)
## Facts (oracle code read)
- plan_hotcode.md HC9: "PLD 64 B ahead in stream_run". The ARM11 of the O3DS has no hardware prefetcher (nd_geom.c:253 PF1 note); the
  stream is written by core 1 and read once, in order, by core 0.
- stream_run (3ds/src/nd_rec.c:471-488) reads the header p[0], dispatches, and steps p by 2 + 2 * n words; no prefetch today.
- A PLD has no effect on results (no fault on an unmapped address, no write): byte-equal by construction. Azahar has no cache model:
  the gain shows only on the console (pmu_d in the prof line).
## Start
`sh 3ds/tools/wt_new.sh HC9B`; PM_ROW=HC9B PM_LANE=HC9B on every call.
Other rows: HC6B edits nd_rec.c in the recorder shadow and nd_rec_model only, not stream_run. `git merge main` before the completing commit.
## Touch only
3ds/src/nd_rec.c: stream_run (:471-488). No other line.
## Exact edits
1. In stream_run, first line inside `for (;;) {` before `if (p == stop) return;` add:
   `__builtin_prefetch(p + 16); /* HC9B: the stream 64 B ahead (no hardware prefetcher on the ARM11) */`
## Gate (CLAUDE.md 2)
- `PM_ROW=HC9B PM_LANE=HC9B sh 3ds/build.sh > build3ds/out/build.txt 2>&1`: memcheck OK.
- `python 3ds/tools/bench.py run nd_host --row HC9B --why "HC9B"`: PASS.
- Byte compare (one list): spot_mac01_det and mdl5_iwa10_det: `bench.py base` each, one `run spot_mac01_det,mdl5_iwa10_det`, `cmp` each: 6/6.
- Console (user): prof push mac_01 0 0 200 (pmu_d, cycles) against the last push; the orchestrator asks for it.
## Stop and end
A failed gate, an edit that does not fit the code, or a test-tool fault: commit and hand back the exact output (CLAUDE.md 4-5).
At the end delete this row file in the completing commit. Hand back: the ready hash, the gate results, the memcheck line.
