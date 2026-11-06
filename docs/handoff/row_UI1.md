# Row UI1: choice menu text and glove drawn twice

## Goal
The yes/no choice menu (super block prompt, Toad House prompt, save prompt) draws the rainbow text and the glove selector twice (a doubled, offset copy). User report 2026-11-06. Draw each once, as the N64 does.

## Start state
Branch nds1, main folder. Old note (archive/agent_playfix2.md): "menu text and glove drawn twice. Toad House menu, 3D slider 0, fps60 off. Candidate palette path msg_draw.c:1347" (line numbers moved: grep). Not fixed since.

## Split rows (after one stall; do ONE row per launch, then hand back)
UI1a HARNESS GUARD: 3ds/run_azahar.ps1, under the mutex: copy %APPDATA%/Azahar/sdmc/3ds/PaperMario/options.ini byte for byte before the run, put it back in finally. If the scenario debug words have no `prof=` word, set the line `profmode=off` in options.ini before the start. Gate: one run of ui1_choice; the log has a `warp:` line. Commit run_azahar.ps1 only.
UI1b CHOICE HOOK: pm_warp.c (grep `MSG_Choice_000A`): get the MSG_Menus_SavePrompt printer first, then the choice printer, as src/entity/SaveBlock.c (grep `MSG_Menus_SavePrompt`) does. Gate: one ui1_choice shot shows the choice menu. Commit.
UI1b DROPPED (2026-11-06): printer lookup without the script ShowChoice step opens no window. Use a real prompt instead: scenario sbk56_superblock (the super block yes/no prompt, log line "superblock: hit" follows the prompt; shots before it) or toad_house_menu.
UI1c CAUSE AND FIX (Sonnet, after the second stall): steps 2 to 4 below, on sbk56_superblock. Candidate: the +2 offset grey shadow pass (msg_draw.c grep `charPosX + 2`, msg.c msg_draw_choice_pointer). Compare its colour and alpha in the shot with 40,40,40 alpha 72.

## Steps
1. Run ui1_choice (preset=yard uishow=choice, shots 200 to 205). Confirm the double in the shot. If uishow=choice fails, use toad_house_menu.
2. Name the cause from the shot and the code: one of (a) the message draw runs twice per frame (count calls of the choice draw function per frame with a debug log), (b) a drop shadow or outline pass of msg_draw.c that draws the same glyphs at an offset with the wrong colour or alpha, (c) the 3D stereo second eye draw at slider 0, (d) the glove HUD element draws in two passes. Write the value the draw used in the status line.
3. Fix that cause only (max 2 tries). Game source only inside `#ifdef TARGET_3DS`.
4. Gate: one shot shows one copy of the text and one glove. Commit. Set ui1_choice status to verified in the same commit.

## Touch only
src/msg_draw.c, src/msg.c, src/hud_element.c, 3ds/src/pm_stereo* if cause (c), own hunk of 3ds/tools/azt_scenarios.json. Do not edit pm_gbi.c or pm_gpu.c (S6 and FX1 own them): if the cause is there, write it in agent_ui.md with the line and hand back.

## Rules and commands
Other agents in this folder: S6 (entity files, pm_gbi.c, nd_*.c), FX1 (effects, grab code, npc_merlee.c), MENU5 (menu files). Pathspec commits, own hunks only, JSON stays valid. If git says index.lock exists, wait 10 s and retry. Build: `python 3ds/tools/gen_map_restore.py; PM_ROW=UI1 PM_LANE=ui sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c (md5 437b90efe5f32b82394ce2ece094210e, never commit). Azahar: `powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row UI1 -Build ui -Try`. Status line in 3ds/logs/perf/status_UI1.txt before each start. Diagnostic budget 3 starts, then reset and continue. Never `git add -N`, `git add -A`, `git commit -a`, `git reset`, `git stash`, `git checkout`, `git restore`. No toggles, no stubs, no git push, no console push, no attribution. Hand back after the gate.

## Oracle plan (2026-11-06, Opus; the user sees the double copy on O3DS AND Azahar)
Draw: src/msg_draw.c ~1514-1541 drop shadow pass: every glyph again at charPosX+2, charPosY+2 (G_RM_XLU_SURF, PM_CC_07: colour PRIM, alpha TEXEL0*PRIM, prim 40,40,40,72), then the real glyph (TEX_EDGE, G_CC_DECALRGBA). src/msg.c msg_draw_choice_pointer (~2245): the same for the glove (shadow +2,+2, PM_CC_07, shadowAlpha 72, then draw_ci_image_with_clipping). Both reach pm_gbi.c gbi_texrect then pm_gpu.c pm_gpu_tex_rect (DR1 batch). Text and glove share exactly one thing: a PM_CC_07 shadow at +2,+2.
Causes: 1 the shadow draws as a full second copy: the PM_CC_07 rect takes its colour from the texel or palette (rainbow) instead of PRIM, or its alpha is 255 not 72 (walker plan for PM_CC_07 in pm_comb.c, combine 8; or rect_begin merges rects whose prim or render mode differ: its key has no prim and no render mode). 2 the shadow is right but too dark or opaque (XLU blend factor, CI4 glyph alpha 1 on transparent texels). 3 the message draws twice per frame (pm_interp key frame, fps60). 4 stereo second eye (unlikely: Azahar shows it).
Measurement (one run, sbk56_superblock, frames 512 to 524): first check the shots of the last UI1c start (shadow pass off with `&& 0`; never written in status_UI1.txt). Else debug switch `ui1log`: one line per frame with the count of msg_draw_choice_pointer calls and of texrects with prim 0x28282848. Crop "Yes" and the glove, measure the offset (dx, dy) and the RGB of the second copy against bg*0.72 + 40*0.28. Offset (2,2) with wrong colour or alpha: cause 1 or 2. Two pointer calls per frame or another offset: cause 3.
Fix: cause 1 colour: PM_CC_07 mapping (pm_comb.c), or add prim and render mode to the rect merge key in pm_gpu.c rect_begin (and rect_raw_open). Cause 2: blend or alpha of the XLU rect (pm_gpu.c flush_draw, nd_tmpl.c build_words). Cause 3: draw the message only in the step draw (pm_interp key path). Game code under TARGET_3DS only. A RECT agent edits pm_gpu.c: coordinate with it (SendMessage) before you edit rect_begin.
Gate: sbk56_superblock and toad_house_menu shots show one copy of text and glove with only a faint grey shadow at +2,+2 (N64 look); if pm_gpu.c changed, byte-equal on a scene without text.
