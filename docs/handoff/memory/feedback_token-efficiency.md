---
name: feedback-token-efficiency
description: "Optimal token use with minimum waste, without losing context agents need; batch small doc edits"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T06:47:46.231Z
---

Use tokens optimally. Do not launch a subagent for a tiny doc edit: each agent costs about 45,000 to 70,000 tokens at start. Batch doc edits, reuse a running agent by SendMessage, or put the doc edit in the next slice agent's task. Search docs/HISTORY.md and docs/SLICES_DONE.md before launching an audit. Keep agent prompts tight. Keep relays to the user short. Do not drop context an agent needs to work well.

**Why:** the user said (2026-10-18) the many small docs agents were a big waste. The user wants the lowest waste that still keeps agents effective.

**User decision (2026-10-18):** the orchestrator may make small doc-only edits itself. A fresh docs agent per milestone is also fine. The user trusts the orchestrator to pick what is cheapest. Keep each agent's context small.

**How to apply:** one docs agent per milestone at most. Use haiku for mechanical work. Agents read 3ds/AGENT_RULES.md and their row only. Related: [[feedback-tight-prompts]], [[feedback-narrow-tests]].
