# Row UI1: choice menu text and glove drawn twice

## Goal
The yes/no choice menu (super block prompt, Toad House prompt, save prompt) draws the rainbow text and the glove selector twice (a doubled, offset copy). User report 2026-11-06. Draw each once, as the N64 does.

## Start state
Branch nds1, main folder. Old note (archive/agent_playfix2.md): "menu text and glove drawn twice. Toad House menu, 3D slider 0, fps60 off. Candidate palette path msg_draw.c:1347" (line numbers moved: grep). Not fixed since.

## Split rows (after one stall; do ONE row per launch, then hand back)
UI1a HARNESS GUARD: 3ds/run_azahar.ps1, under the mutex: copy %APPDATA%/Azahar/sdmc/3ds/PaperMario/options.ini byte for byte before the run, put it back in finally. If the scenario debug words have no `prof=` word, set the line `profmode=off` in options.ini before the start. Gate: one run of ui1_choice; the log has a `warp:` line. Commit run_azahar.ps1 only.
UI1b CHOICE HOOK: pm_warp.c (grep `MSG_Choice_000A`): get the MSG_Menus_SavePrompt printer first, then the choice printer, as src/entity/SaveBlock.c (grep `MSG_Menus_SavePrompt`) does. Gate: one ui1_choice shot shows the choice menu. Commit.
UI1c CAUSE AND FIX: steps 2 to 4 below. Candidate: the +2 offset grey shadow pass (msg_draw.c grep `charPosX + 2`, msg.c msg_draw_choice_pointer). Compare its colour and alpha in the shot with 40,40,40 alpha 72.

## Steps
1. Run ui1_choice (preset=yard uishow=choice, shots 200 to 205). Confirm the double in the shot. If uishow=choice fails, use toad_house_menu.
2. Name the cause from the shot and the code: one of (a) the message draw runs twice per frame (count calls of the choice draw function per frame with a debug log), (b) a drop shadow or outline pass of msg_draw.c that draws the same glyphs at an offset with the wrong colour or alpha, (c) the 3D stereo second eye draw at slider 0, (d) the glove HUD element draws in two passes. Write the value the draw used in the status line.
3. Fix that cause only (max 2 tries). Game source only inside `#ifdef TARGET_3DS`.
4. Gate: one shot shows one copy of the text and one glove. Commit. Set ui1_choice status to verified in the same commit.

## Touch only
src/msg_draw.c, src/msg.c, src/hud_element.c, 3ds/src/pm_stereo* if cause (c), own hunk of 3ds/tools/azt_scenarios.json. Do not edit pm_gbi.c or pm_gpu.c (S6 and FX1 own them): if the cause is there, write it in agent_ui.md with the line and hand back.

## Rules and commands
Other agents in this folder: S6 (entity files, pm_gbi.c, nd_*.c), FX1 (effects, grab code, npc_merlee.c), MENU5 (menu files). Pathspec commits, own hunks only, JSON stays valid. If git says index.lock exists, wait 10 s and retry. Build: `python 3ds/tools/gen_map_restore.py; PM_ROW=UI1 PM_LANE=ui sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row UI1 -Build ui -Try`. Status line in 3ds/logs/perf/status_UI1.txt before each start. Diagnostic budget 3 starts, then reset and continue. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`. No toggles, no stubs, no git push, no console push, no attribution. Hand back after the gate.
