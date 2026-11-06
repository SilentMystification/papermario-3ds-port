# Row BENCH7: build-at for console benches, push log reconnect

## Goal
1. History by rebuild on the console: bench.py build-at <commit> <console bench> builds the commit in its scratch tree, pushes that 3dsx to the O3DS 192.168.1.170, and ingests the live log with the commit of the scratch build.
2. Netloader trouble (user): a push sent 23% and the next failed at send (3dslink exit 255 after the console answered, 3 attempts) but the game started. Twice the live log stopped after boot while the game ran. `python 3ds/tools/livelog.py 192.168.1.170 75 build3ds/hw_live_log_170.txt` alone worked.

## Start state
Branch nds1, main folder. A peer session works in this folder (nd_*, pm_gbi.c, pm_gpu.c, pm_tex.c, model.c). Commit with pathspec only. Never stash, reset, checkout or git add -A.

## Touch only
3ds/tools/bench.py (cmd_build_at near line 702, the 3dsx helper near line 497), 3ds/push_3ds.sh, 3ds/tools/livelog.py, docs/handoff/test_catalog.md (one line per new switch), this row file, docs/handoff/agent_bench1.md.

## Row BENCH7-1: build-at for console_prof benches
- Remove the refusal at bench.py:705 for kind console_prof. Azahar kinds keep the current path.
- The scratch build must not share the peer's build volumes or outputs: build it with PM_LANE=at_<short> (own docker volumes, build3ds/lane-at_<short>/). Check the Azahar path too: if it pops PM_LANE (line 739) and builds with the shared volumes, give it the same lane.
- Push: push_3ds.sh sends build3ds/pm_3ds.3dsx at line 140 and 145, and PM_SRC3DSX (line 35) is used only for the archive. Make the send use $src too, so PM_SRC3DSX=<scratch 3dsx> sh 3ds/push_3ds.sh --no-build --ip 192.168.1.170 <bench push switches> sends the scratch 3dsx.
- Then ingest the live log with --commit <commit> --build <scratch build number> and the bench device. dirty false.
- Add --dry-run to build-at: print the build, push and ingest commands, run nothing.
- Done: python 3ds/tools/bench.py build-at 43135332b spot_mac01_prof --row BENCH7 --dry-run prints the 3 commands with lane at_43135332b, PM_SRC3DSX, ip 192.168.1.170 and the bench switches. sh -n 3ds/push_3ds.sh exits 0. No real build, no push.

## Row BENCH7-2: log reconnect
- push_3ds.sh: when 3dslink fails at stage send after the console answered, try the live log anyway (the game may run). When the live log ends early (no data for N s while the cap is not reached), reconnect with livelog.py to the same file (append), up to 3 times, one line per reconnect.
- Add `sh 3ds/push_3ds.sh --attach --ip <ip> -t <s>`: no send, only livelog.py to build3ds/hw_live_log_<last octet>.txt, then the same ingest hint line.
- Done: sh -n 3ds/push_3ds.sh exits 0. `sh 3ds/push_3ds.sh --attach --ip 127.0.0.1 -t 5` prints the reconnect lines and ends within 20 s with a NO LOG line. Write the output to 3ds/logs/perf/status_BENCH7.txt.

## At the end
Overwrite the STATE block of docs/handoff/agent_bench1.md (10 lines or fewer). Delete this row file in the last commit. No real console push: the user runs the first one.
