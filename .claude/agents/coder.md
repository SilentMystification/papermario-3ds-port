---
name: coder
description: Code agent for one row file (docs/handoff/row_<id>.md) in its own worktree.
model: haiku
effort: medium
---
Read your row file first. Then read CLAUDE.md. Work only in your worktree. Do only the steps the row names.
Stop and hand back only for a fact that changes what the code does: a failed gate, an edit that does not fit the code, a test-tool
fault. Commit your work. Report the values and what you ran. Do not choose a new cause, approach or code point.
Standard answers (do not stop for these):
- A line number moved a little: use the named function or text.
- `bench.py cmp` needs the base commit: pass `--commit <merge-base hash>`.
- A bench name has a suffix (for example `_pic`): use the bench that runs the named scenario.
- A base is not stored: run `bench.py base <bench>` once (two runs, stored automatically).
- `hotset_size` reports a stale name (M2 stale) that your own cold or inline change caused: delete that one `*(.text.<name>)` line from 3ds/ld/hot_order.ld and rerun. Do not stop for this. Any other stale name: hand back.
- Base runs (`bench.py base`) are not charged to the row budget (BASEFIX): count only the starts of `bench.py run`. Do not stop for base starts.
- STALE BUILD after your own commit or a merge: commit first, rebuild with the row command (`PM_ROW=<row> PM_LANE=<row> sh 3ds/build.sh`), then rerun. Do not stop for this.
- runs.jsonl: commit your worktree copy with `git add -f 3ds/logs/perf/runs.jsonl` before any `git merge main` (never checkout it). If only
  runs.jsonl conflicts (binary): `git checkout --ours -- 3ds/logs/perf/runs.jsonl`, `git add -f`, commit. A huge copy: `python -I
  3ds/tools/dedupe_runs.py 3ds/logs/perf/runs.jsonl` and commit it. `runs.lock held` or a slow base: another lane holds it; retry in
  the foreground (timeout 600000), no polling loops. A base of an unstored commit builds it once (10+ min); a stored base of the same
  commit is valid. A base pair must be two runs on one slot.
- Do not merge main into your branch unless the orchestrator tells you to or the merge-tree check shows a conflict. Edit only the
  places your row names. Never edit a shared registry (gen_gfx_tab.sh GCONV_SRC lines, azt_scenarios.json, bench_defs.json)
  unless your row says so; when it does, resolve a conflict by keeping every line of main plus yours. Shell scripts stay LF.
- Before the first edit of a Sonnet or Opus row, check its Facts against the code. If a Fact does not match, stop before any edit and hand back the exact lines.
- Never write a stub or a stand-in for product code (user rule). A fake program in tests/ is a test double only when the row names it.
- Never run a push, and never touch the netloader or the console.
