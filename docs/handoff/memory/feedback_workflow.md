---
name: workflow
description: how many agents to run and when to rotate them; the rules live in CLAUDE.md
metadata:
  type: feedback
---

One code agent at a time, one per epic (3 to 6 related rows). Rotate it at 350k tokens or at the end of the epic. Diagnoses go to fresh read-only agents. The orchestrator does small tasks, handoffs and rule edits itself.

**Why:** on 2026-10-23 the user said the old flow spawned too many small agents, then reused one agent until its context bloated (about 920k tokens). The user asked for a middle path.

**How to apply:** follow CLAUDE.md rules 2 and 3 in the repo. The full rule text is there, not here. See [[feedback_no-feature-toggles]].
