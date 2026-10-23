---
name: feedback-new-agent-handoff
description: "Cross-project rule for when to start a new agent/chat - at finished slice, via short handoff file, user starts it"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 57ecd3fc-c868-4951-8b33-65a76e92f3d0
  modified: 2026-10-18T06:48:44.192Z
---

**Addendum (2026-10-18):** the orchestrator must itself call out when its own chat is bloated from long use (many agent relays, many turns) and plan a handoff to a new orchestrator. Say it plainly, refresh docs/ORCH_STATE.md, then ask the user to start a new chat.

"New agent" means the user starts a fresh chat from a handoff file. It does not mean Claude spawns subagents. Claude spawns subagents only when the user asks.

Rules (apply in every project, not only papermario-3ds-port):
- When a feature or slice is finished, stop and tell the user. Do not keep working in the same chat.
- A commit is not automatically a handoff. After each commit, judge whether the slice is done enough for a new agent. If yes, or if a handoff looks right for any other reason, raise it and wait for the user.
- Handoff lives in one file (`HANDOFF.md` in repo root). Replace it each time. Never append history. Keep it short: current state, next step, do-nots.

Exception (user approved, 2026-10, papermario-3ds-port): the user wants an ORCHESTRATOR chat that spawns one background subagent per slice automatically, in large chunks, and only relays reports. The rules live in `3ds/ORCHESTRATOR.md` (read it first). Model rule: never `fable`; `opus` only for planning agents; `sonnet` is the top tier for coding; `haiku` for mechanical slices. Keep `HANDOFF.md` small (history goes to `docs/HISTORY.md`). Screenshot analysis only when really needed.

**Why:** User runs long multi-slice work (e.g. 3DS Paper Mario port). Long chats drift. Fresh agent from a short file keeps context clean. Source: papermario-3ds-port `HANDOFF.md` and `.cursor/rules/handoff.mdc`.

**How to apply:** After each commit or finished slice, ask whether to hand off. If the repo has no `HANDOFF.md`, offer to write one. Do not spawn subagents unprompted. See [[feedback-ste100-style]] if written.
