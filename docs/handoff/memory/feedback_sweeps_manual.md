---
name: feedback-sweeps-manual
description: Full-sweep tours (cutscenes) run only on request; boss fights are never automated (user drives and reviews)
metadata:
  type: feedback
---

The full sweep (tour of all cutscenes, credits) runs ONLY when the user asks for a "full sweep". It is never part of a per-feature or per-milestone gate. Boss fights are never run by agents: only when the user explicitly asks, and the user drives and reviews them by hand (an automated boss run has no value without the user).

**Why:** the user said the credits test took too long and that automated boss runs are not valuable without them driving. Credits runs must reach the thank-you screen and stop about 3 s after it, not sit on it.

**How to apply:**
- Do not schedule the sweep or boss fights by habit; no agent runs them.
- Provide boss fights only as manual warp entries (Select menu list) for the user to play.
- Related: [[feedback-luma-and-baselines]] (batch optimizations before hardware tests) and [[project-perf-azahar-only]].
