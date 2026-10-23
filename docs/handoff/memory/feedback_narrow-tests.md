---
name: feedback-narrow-tests
description: "Prefer short, narrow, focused tests per slice to save time and tokens, without skipping real faults"
metadata:
  node_type: memory
  type: feedback
  originSessionId: 35381212-6047-491f-a48c-66d4d564fd4b
  modified: 2026-10-18T05:48:06.504Z
---

Use short, narrow, focused tests. Test only what the change touches: one scene or map, a fixed short length, one text check. Do not build large or complex test setups. Do not let faults pass: fix what the narrow test finds and report what was not tested.

**Why:** the user said (2026-10-18) the work gets heavy and wastes effort and tokens with intensive tests. Narrow tests are faster and cheaper.

**How to apply:** put the rule in every build slice prompt. Run the full tour only once at the end. Agents state which areas they did not test. Related: [[feedback-new-agent-handoff]].
