---
name: feedback-inline-small-tasks
description: Do small tasks (handoff, summaries, git bundle, file checks) inline; spawn agents only for code work or long investigations
metadata:
  type: feedback
---

On 2026-10-22 the user called a handoff agent a waste of about 150k tokens: writing the handoff, copying memory, and making a git bundle was quicker inline. The orchestrator-only rule in CLAUDE.md (no doc edits by the orchestrator) does not override the user's wish to save tokens.

**How to apply:** do these inline: handoff notes, summaries, short doc edits, git bundle, file copies, quick status checks, and anything under about 10 tool calls. Spawn an agent only for code changes, long reads across many files, or work that needs Azahar. Ask first if unsure. See [[feedback-visible-status]], [[feedback-push-failures]] and [[feedback-luma-and-baselines]].
