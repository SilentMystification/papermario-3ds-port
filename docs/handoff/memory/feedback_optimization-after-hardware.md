---
name: feedback-optimization-after-hardware
description: "All optimization work (audio, GPU vertex, math, PO2) starts only after hardware validation; planning is allowed before"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T05:57:48.032Z
---

Optimization work means PO2, PO3, AU1, MA1 and similar. Plan it now if useful. Start the real code work only after the hardware validation run (S28).

**Why:** the user said (2026-10-18) audio is optimization work, and all optimization waits for real hardware numbers. Planning is fine because it is needed to get the game running well.

**How to apply:** do not launch PO2, PO3a to PO3g, AU1a to AU1g or MA1 code slices before S28 is done. Plan and listing slices are allowed. A correctness fault in audio code (for example the nok_01 crash, row F01) is a bug fix, not optimization. Related: [[feedback-narrow-tests]].
