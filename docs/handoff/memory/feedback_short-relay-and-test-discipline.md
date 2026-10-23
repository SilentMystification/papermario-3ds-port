---
name: short-relay-and-test-discipline
description: "Orchestrator relays in 3 lines or fewer, runs one code agent per build tree, and runs golden only on touched scenes"
metadata:
  node_type: memory
  type: feedback
  originSessionId: acbc5c65-f70f-49ef-a836-89b9d8e63196
  modified: 2026-10-18T23:33:55.821Z
---

User called two habits "really bad" (2026-10-18): long relayed agent reports, and inefficient testing (many agents in one tree, golden on many scenes per slice).

**Why:** The chat filled with text. Concurrent agents in one tree gave mixed builds, a false 59.7 percent battle golden diff, stale objects and long mutex queues.

**How to apply:**
- Relay each agent report in 3 lines or fewer: row, result, what is not checked or needs the user. No tables, no bullet lists, no repeated detail.
- One code agent per build tree. A second code agent goes to lane B (docs/LANES.md). Read-only agents may run in parallel.
- Run golden only for the scenes a slice touches. Full golden and full tour only at a milestone.
- Use fxtest or one tour= run, not temp hooks.
- Unprompted subagents are fine, but not for tiny tasks. Do small edits inline. See [[feedback_new-agent-handoff]] and [[feedback_token-efficiency]].
