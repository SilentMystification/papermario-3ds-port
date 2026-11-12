# Row PUSHBATCH: one script for the batch console run (push, wait, pull the log, fxcen report; tool, no game code)

Model: Haiku (calls existing tools in a fixed order; no design choice).

## Start
`sh 3ds/tools/wt_new.sh PUSHBATCH`; PM_ROW=PUSHBATCH. No Azahar run, no game build. No real push in this row (the console is the
user's): the gate uses the dry and replay modes.

## Files
3ds/tools/push_batch.ps1 (new). No other file.

## Script `powershell -File 3ds/tools/push_batch.ps1 -Ip <ip> -Label <label> -Seconds <s> -Args "<game args>" [-Stored <file>] [-Dry] [-FromLog <file>]`
1. Push: Git Bash `sh 3ds/push_3ds.sh --ip <ip> -t <s> <game args>` (push_3ds.sh does memcheck, archive and pushed_builds.txt).
   Exit code not 0: print the full output and stop with the same code.
2. Wait: until `<s>` seconds after the push returned (Start-Sleep in 1 s steps, printed every 10 s).
3. Pull: `python 3ds/tools/pm_logrecv.py --ip <ip> --label <label> --timeout 60 --out build3ds`. Exit 2 (timeout): print and stop.
4. Report: `python 3ds/tools/fxcen_report.py build3ds/hw_<label>_log.txt --label <label>` (it takes logs with their own base line,
   fxcen_report.py:73-90). With -Stored <a stored report text, e.g. build3ds/out/fxcen_xl4a.txt>: parse the rows of "table sorted by
   us" in both texts (idx, name, us, and the last column walk) and print per idx: us new, us stored, difference, walk new, walk
   stored, difference; an idx in only one text is listed as missing.
5. Save: the full output of every step to build3ds/out/pushbatch_<label>_<yyyyMMdd_HHmm>.txt (Tee-Object, no Select-Object -First,
   no truncation), and print its path last.
- -Dry: print the 4 commands and stop. -FromLog <file>: skip steps 1-3 and run 4-5 on that log (replay of a stored run).

## Gate
1. `-Dry` prints the 4 commands with the given values.
2. `-FromLog build3ds/out/fxcen_pmu_xl4a.txt -Stored build3ds/out/fxcen_xl4a.txt -Label t1` (the xl4a log and its stored report, main
   folder build3ds/out): the report runs, every difference is 0.0 (same run), the saved file exists and its line count equals the
   console output line count (no cut).
Commit the script; delete this row file in the same commit.

## Stop
A step that fails twice: stop, commit, hand back the output.
