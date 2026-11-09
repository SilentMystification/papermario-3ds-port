# Row EFFCEN: per-effect cost census (user approved, 2026-11-09)

## User facts
Slow effects on the O3DS XL: super block orbs (fixed), Tidal Wave, Sleepy Sheep dust, Up and Away pulse, star spirit sparkle.
User approved: (1) a census that ranks every slow effect in one console push, then (2) one general batching fix (separate row).

## Goal (this row: the census only)
Debug word effcen (cold): in execute_render_tasks (src/model.c :5131+) take per render task the pm_gbi_task_id (CENSUS_TASK,
model.c ~5050) or the effect id, and the deltas of batch closes (native submits, C_DRAW pm_gpu.c:1097), draws, gbi ops and us
(pm_ticks). Every 60 frames print `effcen f<n> top: <effect name or fn> tasks closes draws ops us | ...` (top 5 by us). No per-frame
cost when the word is off. Note: phase counter C_FLUSH is never incremented (pm_port.h): do not use it.

## Start state
`sh 3ds/tools/wt_new.sh EFFCEN`, branch EFFCEN. PM_ROW=EFFCEN, PM_LANE=EFFCEN. One coder agent.

## Touch only
src/model.c (execute_render_tasks hook, TARGET_3DS), a small 3ds/src/pm_effcen.c (or pm_cen.c), azt_scenarios.json, bench_defs.json.
Not pm_gpu.c beyond reading existing counters (row HC4 edits it).

## Done criteria
nd_host PASS, memcheck OK. One Azahar run with fxtest of fx_underwater (index 87) on mac_01 and effcen on: the effcen line lists
underwater in the top 5 with ops near +2100. spot_mac01_det byte-equal with the word off. Commit; delete this row file. Budget: 3 starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
