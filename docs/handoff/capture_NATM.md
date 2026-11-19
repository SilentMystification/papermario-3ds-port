# capture_NATM: one console prof capture of main for the per-core load (plan_native.md section 3, UD-N1. Only after the user's yes to UD-N3). Designer: oracle NATIVE.
No code change. Build main as push_runbook.md Order 1-2 says.
Capture check first (CLAUDE.md rule 3): `python 3ds/tools/capture_check.py --row NATM --proof "core2 lgms,cls rec, wk " --profile prof warp=mac_01:0 pos=0:0:200 profdisk=1`. Push only after CAPTURE CHECK OK.
Push (push_runbook.md Commands, perf line): `powershell -NoProfile -File 3ds/go.ps1 push o -ip <address of the console the user names> -NoBuild -Seconds 90 -Profile prof -GameDebug "warp=mac_01:0 pos=0:0:200 profdisk=1"`.
Link 1 switch: the profile word `prof=30` (bench_defs.json:6) turns prof mode on. `profdisk=1` (pm_log.c:362) also sends the prof lines to log.txt.
Link 2 sink: the live socket, and log.txt on the SD card (pm_log.c:1008: a prof line goes to log.txt only with prof_disk on).
Link 3 tool: push_3ds.sh writes build3ds/hw_live_log_<last octet>.txt. Backup with the game still open: `python 3ds/tools/pm_logrecv.py --ip <address> --label natm --out build3ds` (capture_flip.md Link 3).
Link 4 proof lines, one set per 300 drawn frames (FACT pm_phase.c):
- `prof f<N> w<N> fps <x> | ft <ms> max <ms> o34 <n> wk <ms>` (:590): core 0 frame time and step work (pm_main.c:757-760).
- `prof f<N> w<N> core2 lgms <ms> n <n> wwait <ms> ser <n> core <c> ...` (:669): core 1 logic ms per step, and the core 0 wait for core 1 per drawn frame.
- `prof f<N> w<N> cls rec .. ent .. tex .. comb .. geo .. mtx .. rect .. list .. rest .. task ..` (:618): the core 0 split per sampled frame.
- `prof f<N> w<N> nd .. lg .. gb ..` (:640).
User (about 2 min): let the game boot into mac_01. Stand still 40 s with no input. Then walk through Toad Town for 30 s.
Read: core 1 load = lgms. Core 0 load = wk minus wwait. The state machine share of core 0 = cls tex + comb + list (GUESS: the unit runs fall in these classes, nd_walk.c:97-111).
UD-N1 rule (plan_native.md section 3): wwait 2 ms or more in the still windows supports P2 (core 1 is the long pole). Else P1.
