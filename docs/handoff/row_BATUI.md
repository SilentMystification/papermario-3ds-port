# Row BATUI: battle UI mostly missing (user, 2026-11-09)

## User facts
User (O3DS, build de415917f pushed 2026-11-09): "The battle UI is mostly missing right now. I see the icons for jump, item and
strategies along with their text but the rest of the UI is not visible." So the action wheel icons and labels draw; the other battle UI
(likely the menu boxes/backgrounds, the HP/FP status bar, target arrows, the action command UI) does not.
Draw-code merges in de415917f since the earlier push 92e013100: KMR10M a0022da02 (tile_bound refuses a stale walker bind: a refused
bind can drop a draw), KMR11W 1378f6261 (edge-AA cutout blend, nd_edge_aa in nd_tmpl.c), HUDTRAIL ef146e73d (F07-7 edge stretch only
for rects >= 160 px, pm_gpu_rect.c:218), FFOREST 9d2f39292 (far-plane z clamp in pm4/pm5). Main since then adds EDGEORD (cutout sort
base 9000000), INTERIOR (nd_tex flush), HC9A (pm_tex slot index), HC2 (link order).

## Start state
`sh 3ds/tools/wt_new.sh BATUI`, branch BATUI. PM_ROW=BATUI, PM_LANE=BATUI. One Haiku agent.

## Steps
1. REPRO (at most 2 Azahar starts, main build): a battle scenario that reaches Mario's first turn with the action wheel up (reuse a
   scenario whose until is "battle: start formation", add wait to the menu; new scenario + bench in the same commit). Shot of the
   whole battle screen at the menu. Fault = boxes/status bar missing where the wheel icons show.
2. If shown: find the cause by code, not by more runs. For each suspect merge above, read its diff and name whether battle UI draws
   (hud_element, draw_box, the status bar rects: nd_rect.c, pm_gpu_rect.c, nd_tex_set.c tile_bound) take the changed path. Add one
   log line counting refused or dropped draws per frame in the battle menu (e.g. tile_bound refusals with fmt/siz/w/h). One run with it.
   Fix the named cause; one run shows the full UI. Byte gate: battle_start_f182_cold and msgbox_talk_nowide vs stored base.
3. If not shown in 2 starts: finish the code trace of the 4 suspects, commit the log line, hand back NEEDS CONSOLE with the push
   recipe (prof push, start a battle from the Debug menu, pull the log).
No-repeat: a try that does not change the picture ends that cause; hand back for the oracle.

## Touch only
3ds/src/nd_rect.c, pm_gpu_rect.c, nd_tex_set.c, nd_tmpl.c, pm_gpu.c (only the named cause), azt_scenarios.json, bench_defs.json.

## Done criteria
nd_host PASS, memcheck OK, the battle menu shot with the full UI, byte gates equal except the fixed UI. Delete this row file in the
completing commit. Budget: 6 Azahar starts.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
