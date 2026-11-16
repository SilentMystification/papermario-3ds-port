# ws_MAPCONV (Sonnet, user approved 2026-11-16): every map and battle shape list is served by its .dlc section; the device compiles no shape list
Designer: the oracle (SendMessage). Worktree build3ds/wt/MAPCONV, branch MAPCONV. PM_ROW=MAPCONV PM_LANE=MAPCONV. Start now.
No shared file with MODELRM, DLSITE2, GMERLEE, FXWRAP, WSCMB (FACT: git diff --stat of each branch and patch).
Why not Haiku: the converter must reach every list the device can ask for, and the cause of each run-time compile is found by measurement.
## Objective (blocker of S9DEL1C and stage 5; C1-C10 do not move: no listcen term matches nd_dlc_compile)
- FACT (code read, main 3771a3edbb): 3ds/tools/nd_shape_conv.c write_dlc (:485) writes sections only for lists in the seam table (`t->list`), and skips a list
  the compiler refuses (:518). The device loads a section only when the list is in the seam table: `pre = sl ? dlcp_load(..) : 0` (3ds/src/nd_shape.c:401).
- FACT: the device still runs nd_dlc_compile in 3 places: dlcp_check (:376, switch dlcchk), the run-time compile (:412), the pool-full compile (:418, NDR_FULL).
- Pass: `grep -nE "nd_dlc_compile *\(" 3ds/src/nd_shape.c` prints nothing (3 calls today). nd_dlc.c stays in the device build (nd_ent.c:311 uses it until S9DEL1B).
## Design rules (the coder decides the rest)
- Same draws: the loaded section equals the run-time compile result for every (list, variant). Host proof over every shape of the ROM, not a sample.
- Coverage: the converter writes a section for every list address that dlc_try (nd_shape.c:888) can receive: model node display lists and every nested CALL target
  inside the shape data. Both variants where the run-time rule (dlc_build use_tp, :398) can ask for both.
- A list the compiler refuses: the converter writes a section that says so (refusal code). The device sets e->refused from it and returns 0 from dlc_try, as today.
- Pool full: the device keeps the section groups and drops the pool copies (today: :413-420 compiles with env2 and frees vtx and idx). The host test proves the
  env2 compile and the section give the same groups, gop, gat and exit state. If they differ for any list, stop and hand back the list count and one example.
- The .dlc version goes to 3 (header check nd_shape.c:233). dlcchk moves to the host test. No toggle, no stub. Follow hw_guidelines.md (cold paths stay cold).
## Memory (risk)
- FACT: dlcp_open reads the whole .dlc into the heap (dlcp_buf, nd_shape.c:204, DLCRD). More sections make this buffer larger on each map.
- The converter prints the largest .dlc and the sum, before and after. If any map's .dlc grows by more than 64 KB, stop and hand back the per-map list.
## Edit shape (real code)
Before (3ds/src/nd_shape.c:401): `const int pre = sl ? dlcp_load(&e->o, (u32)(uintptr_t)l - (u32)(uintptr_t)gMapShapeData, use_tp ? 1u : 0u) : 0;`
After: `const int pre = dlcp_load(&e->o, (u32)(uintptr_t)l - (u32)(uintptr_t)gMapShapeData, use_tp ? 1u : 0u);` (every list has a section; a missing section is a fault line).
## Tests that decide
1. Converter census (build.sh gen_shapes step, the shapeconv line :710): add `dlc lists <L> sections <S> refused <R> missing 0 fullbad 0 maxkb <k>`. missing 0 and fullbad 0 pass.
2. New host test 3ds/tests/host.d/mapconv.sh: for every shape, load each section with the device loader code and compare with nd_dlc_compile (the dlcp_check fields, :377-383).
   Print `mapconv ok <n>`. Seeded fault: one changed index byte in one section prints FAIL.
3. `sh 3ds/tools/nd_host_test.sh`: `ND HOST TESTS PASS`. One build: rc 0, memcheck margin above 0.
4. Gate (at most 6 Azahar starts, base runs not charged): spot_mac01_drec, sbk14_drec, jan09_mesh_drec, fxr_drec. `bench.py base <b>`,
   `bench.py run <b> --row MAPCONV --why "MAPCONV gate"`, `bench.py cmp <b> --row MAPCONV --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal each.
## Loop and stop
Group 1: converter coverage + census + host test (commit `MAPCONV 1`). Group 2: nd_shape.c loader, the 3 compile calls out (commit `MAPCONV 2`). Build, gate, commit
`MAPCONV 3: gate` (deletes this page). Fix scope: 3ds/tools/nd_shape_conv.c, 3ds/src/nd_shape.c, 3ds/tests/host.d/mapconv.sh and its fixture dir, 3ds/build.sh (census line only).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle. Hand-back: commit, census line before and after, gate values, memcheck.
