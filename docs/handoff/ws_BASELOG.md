# ws_BASELOG (Haiku): bench.py base keeps both run logs when two base runs differ (tool; counter: none, diagnose aid for NST2D)
Designer: oracle NSTO. Branch from main. Worktree build3ds/wt/BASELOG, branch BASELOG. PM_ROW=BASELOG PM_LANE=BASELOG.
Edit only: 3ds/tools/bench.py, new 3ds/tools/test_basekeep.py.
## Facts (code read, main 6502e280b4)
- base_run (bench.py:1703) runs 2 starts. cmd_ingest_last (:2361) copies each run's shot folder with snap_shots (:1590, call :2377).
- Each start overwrites the run log (the lane slot last_log.txt, azt/logs/<scenario>.txt). On a differing pair, :1757-1759 keeps only
  the 2 shot folders. Run 1's log is lost.
## Edits
1. snap_shots (:1590): new signature `def snap_shots(shots, scenario_name, log=None):`. After `shutil.copytree(shots, dst)` add
   `if log and Path(log).is_file(): shutil.copy2(log, str(dst) + ".log")  # BASELOG: the run log beside its shot copy`.
   The .log file sits beside the folder, not in it, so store_base (:1555) and shot_names (:1608) do not see it.
2. cmd_ingest_last (:2377): before `shot = snap_shots(shot, s) or shot` after `shot = snap_shots(shot, s, p) or shot`.
3. New function above base_run:
   ```
   def keep_logs(keep, first, last):
       """BASELOG: copies the run log of each run of a differing pair (snap_shots wrote <shot copy>.log) to keep/a.log and keep/b.log."""
       got = []
       for k, r in (("a", first), ("b", last)):
           lg = Path((r.get("shot_path") or "") + ".log")
           if lg.is_file():
               shutil.copy2(lg, Path(keep) / (k + ".log")); got.append(k + ".log")
       return got
   ```
   In base_run after `shutil.copytree(last["shot_path"], keep / "b")` add `logs = keep_logs(keep, first, last)`. In the print
   that follows, append `", logs %s" % (" ".join(logs) or "none")` inside the message (after `kept at %s`).
4. test_basekeep.py (stdlib only, `python 3ds/tools/test_basekeep.py`): import bench from 3ds/tools (sys.path). In a tempfile dir:
   make shots/drec.txt and run.txt. Set os.environ["PM_BASE_SNAP"] = <tmp>/snap. Case 1: snap_shots(shots, "sc", run.txt) gives d;
   check Path(d + ".log") exists with the run.txt bytes. Case 2: a second call gives d2; keep_logs(<tmp>/keep, {"shot_path": d},
   {"shot_path": d2}) returns ["a.log", "b.log"] and both files exist. Case 3: shot_names(d) has no .log name.
   Print `basekeep cases 3 bad <b>`, exit 1 when b > 0. Fault: env BASEKEEP_FAULT=1 makes the test call snap_shots without log: case 1 fails.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 0 builds, 2 Azahar starts)
1. Edits 1-4. `python 3ds/tools/test_basekeep.py`: `basekeep cases 3 bad 0`. With BASEKEEP_FAULT=1: bad 1. `python -m py_compile 3ds/tools/bench.py`.
   Commit `BASELOG 1: bench.py base keeps both run logs of a differing pair; test_basekeep`.
2. In build3ds/wt/BASELOG run:
   `python 3ds/tools/bench.py base wscmb_pause_drec --commit 86b4a069092ffc021bcf85b98ff15757b93aecee --row BASELOG > build3ds/out/baselog_base.txt`.
   It uses the basebin store (no build). Two outcomes, both a pass of this page:
   (a) "two runs differ ... logs a.log b.log": list the kept folder path. (b) "two runs equal, stored": note it.
## Fix rights: the 2 files; syntax errors. A tool fault (rc other than 0 or 1, a hang, logs none on a differing pair): stop, send the output
path to oracle NSTO. Stop: last commit `BASELOG 2: base rerun <a or b>; kept <path or none>` deletes this page. One SubagentHandback with the
kept folder path (outcome a), so the oracle diffs the psoseen sets of a.log and b.log.
