# Row WBO-1: list world and battle custom gfx sites (no code)

## Goal
Site list for stream WBO of docs/handoff/plan_s10_path.md (read Stream WBO and Decisions D2, D3, D8).

## Do
Grep src/world and src/battle for custom gfx builders: gMainGfxPos, local Gfx* side buffers (Gfx* arrays and pointers written at run time), gSPEndDisplayList, gDPSetColorImage, gSPDisplayList to heap lists. Write docs/handoff/agent_S7.md: STATE block (10 lines or fewer) on top, then one line per site: file, function, grep string, kind (main-list words / side buffer / colour image / static data only), the scenario that shows it if one exists in 3ds/tools/azt_scenarios.json. Then group the files into rows of about 10 files for WBO-2 (world, by area) and WBO-3 (battle). Add the INDEX.md line for agent_S7.md. Commit both files (pathspec commit, docs only).

## Rules
No code edits. No Azahar runs. No git add -A, commit -a, reset, stash, checkout, restore. No attribution lines. Delete this file in the same commit.
