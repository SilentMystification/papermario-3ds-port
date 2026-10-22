---
name: feedback-luma-and-baselines
description: Do not pull or decode Luma crash dumps unless the user asks; no repeated hands-off baseline runs just for comparison numbers
metadata:
  node_type: memory
  type: feedback
  originSessionId: ac571f0b-1513-4085-9778-3f58937ad7cd
  modified: 2026-10-21T04:16:08.360Z
---

Do not check or decode Luma crash dumps/logs unless the user asks (2026-10-21). FTP pulls still fetch log.txt; skip the dump analysis.

The user does not need comparative numbers when they say performance is still bad. Do not schedule a hands-off repeat run (P3R) only to get before/after numbers. Hardware tests happen after a batch of more optimizations.

**Why:** the user plays and judges the console themselves; extra pushes cost them effort on a flaky network.

**How to apply:**
- After an FTP pull, read the game log only. Do not run decode_dump.py or spawn dump analysis.
- Batch optimizations first, then one hardware test (see [[project-perf-azahar-only]] for the push constraints and [[feedback-visible-status]]).
