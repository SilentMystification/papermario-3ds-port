---
name: no-agreement-small-tasks
description: Never tell the user they are right; send small follow-ups to a running or idle agent with SendMessage instead of spawning a new agent
metadata:
  node_type: memory
  type: feedback
  originSessionId: d8a09720-8720-4e4b-b01b-9625d30a637c
  modified: 2026-10-19T19:47:02.065Z
---

Do not open replies with agreement or validation ("you are right", "agreed", "understood"). The user hates it. State the fix and the next step only.

Send tiny follow-up rows (a number change, a one-line fix, a small report) to an agent that already has the tree with SendMessage. Do not spawn a new agent for them. Group a small row into the next big agent launch.

**Why:** 2026-10-19, the user criticised the build number row (BN1) for getting a new agent, and then criticised the "you are right" reply.

**How to apply:** Before every Agent call, check ListAgents-style state: is any agent running or resumable in that tree? If yes, SendMessage. If a tiny row must wait for a free tree, bundle it with the next launch. Answer complaints with the action taken, not with a concession. See [[feedback_token-efficiency]] and [[feedback_short-relay-and-test-discipline]].
