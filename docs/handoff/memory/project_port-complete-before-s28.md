---
name: project-port-complete-before-s28
description: "All functionality must be ported before the first hardware run S28; after S28 only hardware compatibility, missed fixes and optimization"
metadata:
  node_type: memory
  type: project
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T07:14:27.324Z
---

All functionality (game logic, maths, lighting, effects, battles, everything) must be ported before S28. S28 and later work is hardware compatibility, missed fixes and optimization. No new functionality is ported after S28.

**Why:** the user said (2026-10-18) the hardware phase must not turn into porting. The pre-S28 audit (row H05, with Part C for functionality completeness) must find anything missing or approximated, for example the lava texture fault, before the hardware run.

**How to apply:** treat H05 Part C as required. Every finding becomes a fix row done before S28. Do not accept a missing feature as "later". Related: [[feedback-hardware-after-everything]], [[feedback-optimization-after-hardware]].
