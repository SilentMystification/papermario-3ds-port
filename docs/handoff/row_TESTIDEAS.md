# Test gate ideas for the endgame conversion rows (ORACLE7, 2026-11-12; ideas only, no rows yet)

User bars: exact equality, a single differing bit fails; no screenshot comparisons. Plan numbers (plan_endgame.md section 1):
3820 active builder call sites, 3254 gMainGfxPos uses, 2107 Gfx arrays, readers in about 22 3ds/src modules.
"Reach" = how much of the work the gate sees. Counts are from the code; numbers marked est. need the coverage map (G7) to measure.

## Candidate gates
| # | gate | proves | cannot prove | cost per row | exact | reach | how the gate itself is tested |
|---|---|---|---|---|---|---|---|
| G1 | Host differential over every static list: old walker model vs new native form, per draw (pso output, bind desc, transform ops, absolute triangle vertex triples, uniform values) | every converted list draws the same state and geometry, for every entry state it is called with | lists called with an entry state the scan did not find; call-site (dynamic) data | 1-3 min in nd_host, no Azahar | yes (field equality) | today 451 lists in the S7 set; target all 2107 arrays (GSET rows grow it) | change one unit of one list in the host copy (drop a SETCOMBINE, shift one VTX v0): the compare must fail on that list |
| G2 | psochk: at every native draw, the stated pso and bind vs what the old state model would give at that moment (debug switch, while the model exists) | the call site states its full state (nothing inherited), on the frames run | sites not executed by the scene; it dies with the model (S9DEL2) | 0 extra starts (a switch in the hash scenes) | yes | only the sites the scene executes (G7) | mutation: delete one gDPSetRenderMode before a converted draw in a test build: psochk bad must be above 0 |
| G3 | FRAMEHASH: per-frame 64-bit hash of the submitted PICA words with address words replaced by the bytes they point to (vertex, index, texels, target role) | the GPU input of the frame is bit-identical between base and head | sites not executed; GPU behaviour of a valid stream | 2 starts per scene, base 2 runs per new commit | yes (collision 2^-64) | frames run by the scene | flip one vertex coordinate or one TEV word in a test build: the frame hash must differ, fhashdraw must name that draw |
| G4 | Golden native-record streams: per frame a text dump of every native draw (kind, pso desc fields, bind content hash, matrix rows, vertex and index hashes), stored per scene, diffed line by line | as G3, and it names the differing draw and field without a rerun | as G3 | as G3 plus disk (about 20-60 KB per frame) | yes | as G3 | the same seeded faults as G3: the diff must show the field |
| G5 | Record/replay: the real core 1 stream of a det scene (NDC records of N frames) saved once from Azahar, then replayed on the host through the old walker and through the new path, comparing draw records as G1 | call-site data (dynamic producers) on the host, without the emulator, for both paths; reruns cost no Azahar | the recorder itself; sites not in the recorded frames | one capture per scene (1 start); then host only (seconds) | yes | the sites in the recorded frames (more if the capture uses a tour of scenes) | replay a stream with one record dropped: the compare must fail |
| G6 | Host PICA validator over captured command buffers (the FRAMEHASH capture): every draw has every register it reads written in the frame, lists are 16-byte aligned and closed, sizes match, no read past a list, GX entries valid | the class Azahar forgives and the console hangs on: malformed lists, missing state, bad addresses (the 60 fps hang class) | GPU behaviour of valid streams; timing | host only, seconds per capture | yes | frames captured | feed a list with one header word count +1 or a missing finalize: the validator must fail |
| G7 | Coverage map: which of the 3820 sites and 2107 arrays each det scene executes (a per-site counter table built in by a debug switch, or fncov extended to src/) | the reach of every other runtime gate; a row's gate scenes must execute its sites | correctness | one run per scene per phase | counts | all sites | a site in a file no scene loads must show 0; a scene that runs it must show above 0 |
| G8 | Compile-time ban: a converted file includes nd_nolist.h, which `#pragma GCC poison`s gDP*/gSP*/gs*/Gfx/gMainGfxPos; the native draw API takes `const NdPsoDesc*` built only by ND_PSO_DESC (static const) | no list call is left in a converted file, ever again; no draw without a full static description | runtime behaviour | zero (the build) | yes | every converted file | add one gDPPipeSync to a converted file: the build must fail |
| G9 | End proofs: LISTCEN2 counters and elfgate.py on pm_3ds.elf (plan_endgame.md section 4) | the old system is gone from source and binary | behaviour | seconds | yes | everything | add one list function back: elfgate func must be 1 |
| G10 | Hardware fault capture: GX queue waits with a timeout and the hang dump (pm_gpu_hang_dump), watchdog dumps in crash.txt, pulled after every console batch | a console hang names its GX entry and phase instead of a frozen screen | it finds faults only on the console | none per row; one push per batch | n/a | console runs | the escape test seam (wdhang, crashtest) must write the dump |
| G11 | Console soak batch per phase: a 30 min tour of N maps with fps60 on and off, crash.txt empty, fps and memory lines stored | long-run memory, GPU hangs, the 60 fps path on real hardware | exact picture | one push per phase (user action) | n/a | the toured maps | a build with a known leak must show rising heap use |
| G12 | Determinism guard for every gate scene: two base runs equal (G3/G4) and a warm vs cold run equal, before the scene is used | the gate compares the build, not the run | | 3 starts once per scene per base | yes | | a scene with a load frame in its window must fail it (the frame 33 case of S1TSTEN) |

## Recommended use
- Every conversion row: G8 (build), G1 for its lists, G2 + G3 in 1-2 scenes that G7 shows reach its sites (a row's scenes must
  execute at least 90 % of the sites it converts, else it adds a scene or a G5 capture), G6 on the captured frames. Each row also
  seeds one fault once in its own worktree and shows the gate fail (then reverts it): proof the gate sees its sites.
- Pilots (design rows NPSO, CONV2, NXF and the Sonnet pilots): additionally G4 (named draw diffs) and G5 (host replay).
- Phase ends: G11 console soak and G10 dump pull. Final proof: G9 + G3/G4 over the full scene set + G11.
- What it replaces in today's flow: `bench.py base` + `bench.py cmp` on ppm shots become `bench.py base/cmp <scene>_hash` (G3),
  with psochk (G2) in the same run; byte-equal shots go away (no image stored or compared).

## Reach today (honest)
The present 2-scene gates (spot_mac01_det plus one fxpic) execute only the mac_01 world, its HUD and one effect file: est. under
10 % of the 3820 sites (effects alone are 1388 sites in 127 files; one fxpic scene runs one file). G7 gives the real number; until
then a row's gate cannot claim to cover its sites.

## Failure classes no gate above catches
- Speed regressions (console timing): only the console prof push measures them.
- Differences in how the real PICA renders a valid stream (precision, filtering) that Azahar does not show: a valid, equal stream
  is assumed to look equal on the console; the user's look check stays.
- Sites that no scene executes and no host gate models (dynamic call sites outside every capture): code read only; G7 lists them.
- Non-deterministic scenes (map load frames, threaded timing): excluded by G12, so their frames stay unchecked.
- Latent state differences that do not change any frame of the tested windows.

## Rule text to propose (the user confirms)
- CLAUDE.md rule 4, delete the last sentence ("A byte difference of at most 1 step per channel ... rounding rule ...") and add:
  "Every compare is exact: one differing bit fails the gate. There is no rounding exception."
- orchestrator_rules.md, Rows: delete "A byte diff of at most 1 step per channel inside the effect area passes when the oracle
  names the code cause (rounding rule)."
