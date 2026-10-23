---
name: feedback-no-stubs
description: Papermario 3DS port - do not stub or hand-write stand-ins for decomp code; ask the user before any stub
metadata:
  node_type: memory
  type: feedback
  originSessionId: 942495d4-279f-4cfd-8b42-5433bc2273f2
  modified: 2026-10-16T03:31:53.636Z
---

Do not add stubs or hand-written stand-ins in place of real decomp code. Build the real decomp file. Ask the user for permission before any stub, and name the unported system that forces it.

**Why:** On 2026-10-15 the user found that map transitions failed because `get_map_IDs_by_name` and `load_map_by_IDs` were hand-written stand-ins with a hard-coded map list. The real versions exist in `src/world/world.c`. The user dislikes stubs and says they cause bugs. The user is not recreating the game. Cleaning up stubs is the top priority of the next pass.

**How to apply:** Before writing a stub, check whether the real file builds and links. Use `3ds/tools/dedup_stubs.py` to drop stubs that a real file now defines. If a stub is still the only option, ask the user first. Do not follow an earlier commit's habit of stubbing. See [[feedback-new-agent-handoff]] for the handoff rule.
