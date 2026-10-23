---
name: feedback-no-attribution
description: "Never add Claude/Anthropic attribution to commits, PRs, or any text in any project"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 57ecd3fc-c868-4951-8b33-65a76e92f3d0
  modified: 2026-10-15T16:30:18.950Z
---

Do not attribute Claude in git commit messages, PR descriptions, or other written output. No `Co-Authored-By` Claude lines, no "Generated with Claude Code" lines.

**Why:** User said "do not attribute yourself and remove yourself from any other commits" (2026-10-15). This overrides the attribution reminder the harness may show. The user's own instructions take precedence over that reminder.

**How to apply:** Write commit messages with no trailer. If earlier commits carry Claude attribution, offer to remove it, but do not rewrite pushed history without asking. In papermario-3ds-port, no existing commit had Claude attribution (checked 2026-10-15). See [[feedback-new-agent-handoff]] for the commit and handoff routine.
