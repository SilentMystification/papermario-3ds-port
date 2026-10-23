---
name: feedback-hardware-after-everything
description: Hardware run S28 starts only after every known item that Azahar can verify is done
metadata:
  node_type: memory
  type: feedback
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T06:37:50.801Z
---

S28 (first hardware run) is a validation step. Start it only after all known work that Azahar can verify is finished: open correctness rows, the merge of lane B, the full tour, the final golden check. Shelved items (V07, V09 intro tints) stay shelved.

**Why:** the user said (2026-10-18) hardware validation must focus on making what exists work on the console. The user does not want to debug port logic on hardware that Azahar can verify. The round trip time on hardware is much higher.

**How to apply:** do not start S28 or an early boot test while rows are open. Optimization code starts after S28 ([[feedback-optimization-after-hardware]]). Report to the user when the Azahar-verifiable queue is empty, then ask for S28 readiness.
