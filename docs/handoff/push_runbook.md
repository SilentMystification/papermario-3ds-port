# Push runbook (one page, edit in place). Every agent reads this before a console push. No guessing.

## Facts (checked 2026-11-17)
- Consoles: O3DS 192.168.1.170 (`o`), N3DS 192.168.1.224 (`n`), O3DS XL 192.168.1.131 (give `-ip 192.168.1.131`, target `o`).
- The netloader broadcast probe does NOT reach the consoles from this PC. go.ps1 uses the address above, so no probe runs.
- The user opens the netloader. The user's word "ready" is a fact. A probe, ping or send failure is never a console state.
- `go.ps1 push` builds unless `-NoBuild`. A build runs only when build3ds/pm_3ds.3dsx is stale. Never start a push while a build is needed.

## Order (the user's rule, 4 violations)
1. Merge and build main (`sh 3ds/build.sh` as a background job, one command; wait for its notice). Check `build3ds/out/main_build*.done` or the 3dsx time.
2. Gate: `python 3ds/tools/binchk.py build3ds/pm_3ds.elf --file docs/handoff/push_strings.txt` prints `binchk OK`.
3. Only now ask the console selector (AskUserQuestion, multi-select O3DS XL .131, N3DS .224, O3DS .170). Ask only when the build is ready at that instant.
4. On the answer, push at once. Nothing else first. No merge, no build, no question.

## Commands (PowerShell, from the repo root, run in the background)
- Default push, game defaults, 75 s of live log: `powershell -NoProfile -File 3ds/go.ps1 push o -NoBuild -Seconds 75`
- Other consoles: `push n -NoBuild` (N3DS), `push o -ip 192.168.1.131 -NoBuild` (XL).
- Fault finding (debug log, bottom screen shows info): add `-Profile log`.
- Perf numbers (prof lines, bottom screen log OFF by design: conlevel=off): add `-Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200"`.
- Profiles are the words of 3ds/tools/bench_defs.json "profiles" (prof, log, none). No profile = game defaults (loglevel 1: about 6 lines after boot).
- A scripted bench recipe: `python 3ds/tools/bench.py show <bench>` prints the command. Read the real command in the code, not its echo line.
- Before the user spends console time on a new scenario: `python 3ds/tools/capture_check.py --row <id> --proof "<lines>" <push game words>` once in Azahar (CLAUDE.md rule 3).

## Reading the result (never claim before the log shows it)
- Files: `build3ds/out/push_o_<time>.txt` (+ `.err`), `build3ds/hw_live_log_<last octet>.txt` (170, 224, 131), `build3ds/out/push_<ip>_<time>_aN.txt` (3dslink output).
- The launcher prints `PUSH STARTED` and exits. The push runs detached. Read `PUSH RESULT` in `push_o_<time>.txt`. `ok rc 0` = sent and log read.
- `Sending pm_3ds.3dsx, N bytes` and `N sent (P%)` = the transfer runs. Only this line means the console answered.
- The game replays the old SD file crash.txt at boot (`#PMFILE crash.txt`). Those `[CRASH]` blocks carry old build numbers: compare with the `#3 build <N>` line of the run.
- fps and stutter: `prof f<n> w<n> at` lines (prof profile only). Ingest: `python 3ds/tools/bench.py ingest build3ds/hw_live_log_<octet>.txt --bench <name> --host o3ds --build <N>`.

## Failure table
- `no answer to the netloader broadcast from this PC`: the probe, not the console. Use go.ps1 `push o|n` (it passes -ip). Retry at once.
- `attempt 1: no response for 15s (console never answered)`: a 3dslink handshake timeout. The tool retries 3 times. Wait for attempt 2 and 3.
- `Connection to <ip> failed` three times, after an earlier push in the same session: the earlier game may still run on the console and no netloader listens. Tell the user that one fact. Push again when the user says so.
- `STALE 3DSX`: a source file changed after the build. Build first (step 1), then repeat steps 2 to 4.
- `NETLOADER REFUSED` / send below 100%: not enough free RAM in applet mode. See docs/handoff/test_catalog.md section C.
- `PUSH REFUSED: binchk`: the binary lacks merged code (push_strings.txt). Build main again.

## After the push
- Pull the SD log: `powershell -NoProfile -File 3ds/go.ps1 pull o -ip 192.168.1.170` (FTP).
- Put console findings into docs/handoff/verify_list.md. Never look at a screenshot (CLAUDE.md rule 2).
