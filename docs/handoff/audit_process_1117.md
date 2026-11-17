# Audit: where the row time goes (oracle, 2026-11-17, main 13ca8b0872)
Question (user): is the time real work or poor process? Answer: process. Real work is 18% of the sampled row time.
Sources: `git log --first-parent main` (merge and branch commit times), 3ds/logs/perf/runs.jsonl (run times),
build3ds/out/post_merge_*.txt (STAGE lines), harness tasks folder (file birth and mtime, sizes only), plan_buildspeed.md.
Agent start = birth time of its tasks/a*.output file. Agent end = mtime. Splits inside a gap are GUESS. Anchors are FACT.

## Wall time of the merged rows (FACT: merge commits and agent files)
- 41 rows merged from 11-16 01:41 to 11-17 01:36 (FACT git log). 47 agents, 359 background jobs (FACT tasks folder).
- Before the pipeline rows (to 11-16 18:16): agent start to merge 52-116 min a row (FACT: CGFX 62, DLSITE1 76, AXNS 53,
  FXHOLD 67, FXREN 53, MODELRM 116, GMERLEE 71, WSCMB 63).
- After BASEBIN and VOLGEN (11-16 18:18 on): 16-43 min a row (FACT: EVAR 33, PBUF 33, GFXINC2 24, VINE 18, ROOTREST 18,
  GENSPEED 16, VOLHOST 19, GPOSREST 24, GCONVSPLIT 24, NIMGM 39, HOSTD2 9). Pipeline rows: MAINBIN 126, BASEBIN 60, VOLGEN 62.

## Minutes per class for the sampled rows (anchors FACT, splits GUESS)
Classes: a real work (code, design, required proof runs), b tool compute a better pipeline removes, c queue wait,
d agent overhead, e rework, f orchestrator overhead.
| row | anchors (FACT) | a | b | c | d | e | f | total |
| DLSITE2 | start 15:23, commit 15:43, base runs 16:05 and 16:32 (same commit cc671d2df), gate 16:36, merge 16:38 | 5 | 58 | 0 | 2 | 0 | 10 | 75 |
| MODELRM | start 14:46, commit 15:35, base ae6d66d3e run 4 times (15:58, 16:13, 16:24, 16:40), merge 16:42 | 29 | 70 | 0 | 7 | 0 | 10 | 116 |
| EVAR | start 18:18, commit 18:29, base 18:48, gate 18:50, merge 18:51 | 5 | 21 | 4 | 2 | 0 | 1 | 33 |
| PBUF | start 18:18, commit 18:32, base 18:50, gate 18:51, merge 18:51 | 5 | 20 | 4 | 3 | 0 | 1 | 33 |
| MAINBIN | start 18:18, commits 18:30, 19:08, 19:27, 19:41, 20:14 (fix 1b-1e), gate 20:24 | 12 | 23 | 0 | 9 | 80 | 2 | 126 |
| GPOSREST | start 00:57, commit 00:58, cold gate run 01:17 not comparable, rerun 01:20, merge 01:21 | 3 | 10 | 5 | 3 | 3 | 0 | 24 |
| NIMGM | start 00:57, code 01:16, bench fixes 2b 01:26 and 2c 01:30, 4 base runs fail:1 01:29-01:30, merge 01:36 | 14 | 8 | 3 | 2 | 11 | 1 | 39 |
| VOLHOST | start 00:57, host runs 229 s, 425 s cold, 8 s warm (FACT commit), gate 01:15, merge 01:17 | 13 | 4 | 1 | 1 | 0 | 0 | 19 |
| sum | | 86 | 214 | 17 | 29 | 94 | 25 | 465 |
| share (GUESS split) | | 18% | 46% | 4% | 6% | 20% | 5% | 100% |
- Not in the table: GPOSREST, VOLHOST, GCONVSPLIT, NIMGM had pages on main at 20:42 and 21:11. No coder started until 00:57
  (FACT: no agent file from 21:00 to 00:57). That is 226-255 min a row, 4 rows, about 935 row-minutes of class f. Cause OPEN
  (GUESS: session pause). Pages ws_CGFX and ws_DLSITE1 (03:04) started 13:44: 10.7 h (GUESS: night).

## The 5 largest avoidable costs
1. Base build per bench (class b, fixed by BASEBIN 18:16). FACT: MODELRM built base ae6d66d3e 4 times, 10-15 min each,
   38 min. DLSITE2 built cc671d2df 2 times (16:05, 16:32), 27 min. Cause: bench.py base_build_run per bench (plan_buildspeed
   root cause 1). About 40% of all pre-pipeline row time (GUESS).
2. Generator keys that cover all of src (class b, open now). build.sh:142-154 keys gfx_tab on all of 3ds/tools, 3ds/include,
   3ds/stub_include, include, src. build.sh:168-170 keys pso_tab on the same plus 3ds/src. Every DL row edits src, so each row
   misses. FACT post_merge_341349fe3b.txt: STAGE gen 95 s. FACT post_merge_ffdbb47067.txt: gen 162 s and 164 s. The host test
   pays it again: VOLHOST first worktree run 229 s, gfx-table 153 s of it (FACT commit 39f52de80). About 4-5 min a row (FACT parts).
3. Rework from page and tool faults (class e). MAINBIN 4 fix rounds 1b-1e, 80 min (GUESS split, FACT commits). NIMGM 2b and 2c:
   bench defs of the page were wrong (FACT commits 7794f403d6, 65904ddf1e, runs.jsonl 4 fail:1). The oracle released pages
   with untested bench lines. Page faults fixed by coders: CGFX, PBUF, WSCMB, NMDL0, MAINBIN, POSTASYNC (FACT commit text).
4. Cold first Azahar run (class e). bench.py:2229 refuses a cmp when the row run is cold (warm 0). The first run in a new lane
   slot is cold. FACT: GPOSREST spot run 01:17 warm 0 boot 3.7 s, rerun 01:20. DLSITE1 sbk14 warm 0 boot 13.7 s, rerun.
   The tool knows the run is cold but does not rerun it. Each case costs a turn and 1-3 min.
5. Agent wait churn (class d, mostly tokens). 178 of 359 background jobs are 22-byte files (FACT sizes). GUESS: each is a wait
   loop that the harness moved to the background. They come in chains, one new job every 3 min, all ended at the agent end:
   MODELRM 18 jobs ended 16:41, MAPCONV 16 ended 18:23, NMDL0 14 ended 18:47 (FACT mtimes). Each new job is one agent turn that
   re-reads the whole context. One job ran 96 min (16:41-18:17) and wrote 5.37 GB (FACT bvu31lxtx.output size). Cause OPEN.

## Rows redone or blocked by a tool fault (FACT commit text)
- 11 of 41 rows: AXNS (slot.pid lock, try 2), DLSITE1 (cold run rerun), CGFX 2b (scratch_build leftover folder), GSETR (no
  worktree), STALEOBJ (whole row: stale objects on the shared volume), BASEBIN 1b and 1c (unstable image id, no docker in the
  host container, first host run stopped), VOLGEN 1b (image key), GFXINC 3 (check mode miscount), MAINBIN 1b-1e,
  GPOSREST (cold run), NIMGM 2b and 2c (bench defs). WAVEWIDE fxr_drec DIFF stays open (FACT commit 01b664502d).
- Blocked: 4 rows waited 226-255 min for a start with pages ready (see above).

## Share of agent turns that wait
- GUESS 50%: 178 of 359 background jobs are wait jobs (FACT counts, GUESS content). Transcripts were not opened (brief).

## Verdict
Mostly process. In the 8 sampled rows, real work is 86 of 465 min (18%, GUESS split). Tool compute that a better pipeline
removes is 46%, rework 20%, queue, agent and orchestrator overhead 15%. The pipeline rows cut the row wall time from 52-116 min
to 16-43 min (FACT), but the remaining time is still mostly generator misses, cold runs, page faults and wait turns. The largest
single loss is not in any row: 4 ready rows waited 3.8 h for a start, and 2 rows waited 10.7 h.

## Top 5 cuts, by minutes saved a row (GUESS each, measure with STAGE lines)
1. One-call row gate `row_gate.sh`: host test, build, basebin, warm head runs (rerun when warm 0), cmp, one summary line, all in
   one foreground call under 590 s. Removes the cold rerun, the wait chains and most turns. Saves 4-8 min a row.
2. Narrow the generator keys: gfx_tab keys on the gconv object hashes plus the table tool files, not on all of src
   (build.sh:142-154). pso_tab keys on the files gen_pso_tab.py reads (build.sh:168-170). Host test uses the same entry.
   Saves 3-5 min a row (FACT: gen 95-164 s, host gfx-table 153 s).
3. Oracle dry run of each page before release: apply the patch, run the host test and the new bench defs on base once.
   Saves the e class of page faults: 5-10 min on 1 row of 4 (FACT 6 page faults in 41 rows).
4. Prebuilt base at merge and kept per commit (post_merge prebuild, POSTASYNC): the base of a row is ready before the row
   starts. Saves 1-3 min a row (FACT: EVAR and PBUF waited from 18:30 to 18:48 for build and base).
5. Start rows when pages land: the orchestrator starts a coder at the page commit, and the session hands over pages to a
   live session. Saves the 226-255 min start wait of the last batch (FACT), not row work time.
