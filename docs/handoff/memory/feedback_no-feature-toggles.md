---
name: no-feature-toggles
description: finished implementations replace the main path; no leftover A/B switches, default-off flags or old paths
metadata:
  type: feedback
---

When an implementation is right, it replaces the main path. Do not leave feature toggles, abtog states, default-off flags or the old code path. Git history keeps the incremental steps.

**Why:** the user said so on 2026-10-22 (DSP audio: "no flag, no switch"; GPU path: "do not leave in feature toggles"). Toggles clutter the code and keep dead paths alive.

**How to apply:** a temporary switch for one console A/B is fine only until the result is in. Then delete the switch and the losing path in the same row. Applies to existing ones too: dlc, dlc1/dlc2, nopf, nowg, gv1/raw1/fl1 off-switches, dspfx/dspgain/dspaux/dspflush. Debug tools (logs, shots, padscripts) may stay. See [[feedback_no-stubs]].
