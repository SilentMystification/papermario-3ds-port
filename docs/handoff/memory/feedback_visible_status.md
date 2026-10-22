---
name: feedback-visible-status
description: Orchestrator must be able to report agent status on request; do not overtest; detect failures instead of waiting
metadata:
  node_type: memory
  type: feedback
  originSessionId: ac571f0b-1513-4085-9778-3f58937ad7cd
  modified: 2026-10-20T06:24:38.583Z
---

Agents must write status lines to a file (3ds/logs/perf/status_<row>.txt) so the orchestrator can answer "status" from facts. Silence is not acceptable.

**Why:** the user could not see what a silent agent or a hung push was doing, and asked "is it running?" twice.

**How to apply:**
- Every agent prompt: append a status line before and after each long step, at least every 2 minutes.
- Run long commands in the background with output to a file and check the result line (PUSH OK / PUSH FAILED), never assume success from exit code 0.
- Do not overtest: one or two runs per performance change, no 3-run hash series unless the first run differs unexplained.
- Never use `pkill -f` with a pattern that matches the own command line.

2026-10-21: the user asked why the same tests run over and over. Code agents ran golden yard/toad/pause/battle after EVERY commit (about 10 runs in one batch). New rule: run the golden set ONCE at the end of a batch of related fixes (or when a change touches a shared render path in a way that can change other scenes), not after every small commit. Per-fix checks are one targeted shot run. Put this rule in every agent prompt.

User rule (2026-10-21, final): golden runs come ONLY after a major milestone or when a batch of tests is done. Any individual feature gets ONE targeted single test. Never run golden per commit or per fix. Stopping runaway test loops: a runner script (python sweep.py) can outlive its agent; kill it by PID.
