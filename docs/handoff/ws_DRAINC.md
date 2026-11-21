# ws_DRAINC (Haiku): census of the logic thread's walker drains by caller (prof line `drainc`)
Designer: oracle NATIVE19 (plan_walk.md 5). Worktree build3ds/wt/DRAINC, branch DRAINC. PM_ROW=DRAINC PM_LANE=DRAINC. Start: main after STEPX merges.
Why Haiku: one table, one caller store, one print, given below. No draw change. Indentation: as in the file.
FACT: every drain goes through pm_walk_drain (3ds/src/nd_walk.c:469-476), which calls pm_wq_idle (:474). The why text is "rom" for every ROM read
(3ds/src/pm_os.c:494), so the ROM reads need the caller of nuPiReadRom. Objective: per window the top 4 drain sources with count and wait ms.
Counter: none (it picks the next narrow drain row after ROMDR).
## Edits (CRLF files: use the Edit tool; `/* DRAINC */` on each new line)
1. pm_os.c: above `void nuPiReadRom(` (or above `static void rom_read(` when ROMDR is merged) add `void* pm_rom_caller;`.
   In nuPiReadRom (the public function), first line: `    pm_rom_caller = __builtin_return_address(0);`
2. nd_walk.c, above `void pm_walk_drain(const char* why) {` add:
   `typedef struct { const void* k; unsigned n, t; } DrcEnt; DrcEnt pm_drc[8]; extern void* pm_rom_caller;`
   In pm_walk_drain replace `    pm_wq_idle();` with:
   `    const unsigned dt0_ = pm_wq_on_logic() ? pm_ticks() : 0u;`
   `    pm_wq_idle();`
   `    if (pm_wq_on_logic()) { const void* k_ = (why[0] == 'r' && why[1] == 'o' && why[2] == 'm' && !why[3]) ? pm_rom_caller : (const void*)why; unsigned i_ = 0;`
   `        while (i_ < 7u && pm_drc[i_].k && pm_drc[i_].k != k_) i_++; pm_drc[i_].k = k_; pm_drc[i_].n++; pm_drc[i_].t += pm_ticks() - dt0_; } /* slot 7 takes the rest */`
3. 3ds/src/pm_phase.c: after the `core2 wait bw` print statement (:728-729) add one block: copy pm_drc (extern `typedef` as in nd_walk.c: declare
   `typedef struct { const void* k; unsigned n, t; } DrcEnt; extern DrcEnt pm_drc[8];`), pick the 4 entries with the largest t, print
   `prof f%u w%u drainc %08x %u %u %08x %u %u %08x %u %u %08x %u %u` (key address, count, ms x 100 = t * 100 / PF_TPM), then zero pm_drc.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=DRAINC sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row DRAINC`: byte-equal.
3. `python 3ds/tools/capture_check.py --row DRAINC --proof "drainc" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
4. For each key address of the last drainc line: the function in build3ds/pm_3ds.map of the DRAINC build whose range holds it (a why text address names a
   string: give the string). Put the names into the hand-back.
Fix rights: compile errors, declaration placement. A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `DRAINC 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line> <the 4 names>` deletes this page. Hand-back: one SubagentHandback: commit hash, results.
