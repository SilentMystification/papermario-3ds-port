## STATE
- Branch nds1. BENCH6 done (commits 0f2c74732 build source, 5d151eb12 ndmcheck benches). Row file deleted in the last commit.
- Azahar build: boot line "#<seq> build 2175 git 19348de70 loglevel 3" (pm_main.c:660). prof=4 runs have no boot line (file starts at #13), so build comes from 3ds/include/pm_build.h of the 3dsx that ran (mtime check). pushed_builds.txt is for console hosts only.
- Lane builds print "build 2175-UC1 git ...": BUILD_LINE takes the suffix.
- New benches (pending until S3N-3 prints "ndmcheck f<n> recs"): spot_mac01_ndmcheck, spot_mac00_ndmcheck, spot_sbk56_ndmcheck (cap 25; gates ndm_diff == 0, ndm_eq > 0).
- Tools: 3ds/tools/bench.py, bench_defs.json, azt_scenarios.json, azt.ps1. Process: docs/handoff/test_catalog.md. Data: runs.jsonl (append only).
- Open, low: boot_mem runs 21 and 22 have no linear_kb metric. Run 7 is labelled build 2020 but holds the 2099 log (left, history is append only).
- Open, peer area: game source still names texdiff and mdldiff (pm_gbi.c, model.c).
- Next command: python 3ds/tools/bench.py list
