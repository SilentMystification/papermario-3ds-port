# Row PUSHFIX: -NoBuild never builds; a push runs detached with its whole output in a file; every 3dslink attempt is kept

(Model: Haiku. Two scripts and the push host test; every edit below. No console run: the gate is the host test and --dry.)

## User facts (2026-11-12)
`go.ps1 push ... -NoBuild` built anyway (build3ds/out/push_xl6.txt: STAGE sync-host, configure, compile-link, copy-out), then
"FAILED - push_3ds.sh exited 1" with no reason. Pushes block the orchestrator chat. N3DS: exit 1 with no reason. O3DS .170
stalls after `=== push o (O3DS) ===`. The raw 3dslink output is lost.

## Cause (ORACLE6 code read, main cd4783813)
- go.ps1:127-128 skips its own build under -NoBuild, but Push-One (:173-177) calls `push_3ds.sh -t ... @profArgs @extra`
  without `--no-build`, and push_3ds.sh:163-166 runs `sh 3ds/build.sh` whenever nobuild is 0.
- push_3ds.sh keeps each attempt's 3dslink output in build3ds/.netload_attempt_<pid>_<i>.log, then deletes it after note_partial
  (`rm -f "$outfile"`, about :427). Only the PUSH FAILED summary line stays.
- The broadcast probe (push_3ds.sh about :290-304, 8 tries) prints nothing until it ends. That is the silent part after the
  `=== push o ===` header when no -ip is given.

## Start
`sh 3ds/tools/wt_new.sh PUSHFIX`. PM_ROW=PUSHFIX.

## Touch only
3ds/go.ps1 (param block, Push-One, the push verb block), 3ds/push_3ds.sh (option parse, the probe, the attempt loop end, a stale
check before the send), 3ds/tests/push/run.sh (new cases).

## Exact edits
1. go.ps1 Push-One: `if ($NoBuild) { $profArgs += '--no-build' }` next to the `--mem-ok` line.
2. push_3ds.sh, after mem_gate and before the probe: when nobuild is 1 and PM_SRC3DSX is empty, compare the hash in
   3ds/include/pm_build.h (the line archive_build reads) with `git rev-parse --short=9 HEAD`. If they differ:
   `echo "STALE 3DSX: built <hash>, HEAD <head>. Build first (go.ps1 without -NoBuild). Nothing was pushed."; exit 5`.
   A test may set PM_BUILD_H to another file.
3. push_3ds.sh, the probe: print `probe: netloader broadcast (8 tries)...` before it, and `probe: no answer` on failure (the :305
   line stays).
4. push_3ds.sh, attempt loop: replace `rm -f "$outfile"` with a move to
   `build3ds/out/push_${ip}_$(date +%Y%m%d-%H%M%S)_a${i}.txt` and echo `3dslink output: <that file>`. In the wait loop, after the
   console answered, print once `attempt $i: no send progress for 30 s (last line: <tail -n 1 of outfile>)` when the outfile
   size has not grown for 30 s. Nothing is killed (the hard rule above the loop stays).
5. go.ps1 detached mode: new switch `-Fg`. For the verb push without -Fg and without -Interactive, start the same command
   again with `-Fg`: `Start-Process powershell -ArgumentList (@('-NoProfile','-File',$PSCommandPath) + <the bound args> + '-Fg')
   -WindowStyle Hidden -RedirectStandardOutput <log> -RedirectStandardError <log>.err -PassThru`, with
   log = `build3ds/out/push_<Target>_<yyyyMMdd-HHmmss>.txt`. Print one line `PUSH STARTED pid <pid> log <log>` and exit 0.
   With -Fg (the child), at the end write `PUSH END rc <rc> <time>` to the log and to build3ds/out/push_last.txt.
6. 3ds/tests/push/run.sh: four new cases.
   (a) `powershell -File 3ds/go.ps1 push o -NoBuild -Fg -ip 127.0.0.1` with PM_3DSLINK set to a fake script that prints
       `Sending ...` and exits 0: no `STAGE` line and no `build` line in the output.
   (b) PM_BUILD_H pointing to a copy with another hash: exit 5 and `STALE 3DSX`.
   (c) A fake 3dslink that prints `10 sent (5%)` and exits 1: a build3ds/out/push_127.0.0.1_*_a1.txt file exists and the
       NETLOADER REFUSED block is printed.
   (d) go.ps1 without -Fg prints `PUSH STARTED` within 5 s, and push_last.txt gets `PUSH END` within 60 s.

## Gate (expected values)
- `sh 3ds/tests/push/run.sh`: every old case and the four new ones PASS (hand back the summary line).
- `sh 3ds/push_3ds.sh --dry --no-build`: prints the 3dslink argv, no build line, exit 0.
- No Azahar start, no console. Budget 0.

## Stop
A failing case: stop, commit, hand back the output. Delete this row file in the completing commit.
