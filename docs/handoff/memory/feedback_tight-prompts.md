---
name: feedback-tight-prompts
description: Keep subagent prompts short and limited to needed information
metadata:
  node_type: memory
  type: feedback
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T05:50:51.203Z
---

Write subagent prompts tight. Give only the information the agent needs: task, done check, files or rows to read, rules by reference. Do not repeat rules the agent reads in ORCHESTRATOR.md. Do not add background or option lists the agent does not need.

**Why:** the user said (2026-10-18) prompts should not overshare. Long prompts waste tokens.

**How to apply:** use the slim template in 3ds/ORCHESTRATOR.md section 5. Point to files instead of pasting facts. Related: [[feedback-narrow-tests]].
