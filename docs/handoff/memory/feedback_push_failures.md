---
name: feedback-push-failures
description: When a console push fails, report the failure in one line and stop; no diagnosis, no retry, no suggestions
metadata:
  type: feedback
---

If a push to a console fails, report "push failed" and stop. Do not diagnose, do not ping or probe ports, do not retry on my own, do not list causes or options.

**Why:** the user (2026-10-21) said diagnosing wastes tokens. They fix the console side themselves and say "push" again.

**How to apply:** one short line with the result (PUSH FAILED or NO LOG, the stage). Wait for the user's next "push". See [[feedback-visible-status]] and [[feedback-luma-and-baselines]].
