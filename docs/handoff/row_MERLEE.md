# Row MERLEE: Merlee spell cutscene in dro_02 (cards, fade, lag)

## User facts (bug, 2026-11-08, console, wide mode)
Trigger: dro_02, Merlee near -110 0 -421; talk, pay, she casts the spell (src/world/area_dro/dro_02/npc_merlee.c). The whole cutscene runs, with three faults:
1. CARDS: the cards do not move correctly when shuffled; their movement in general is buggy (RitualCards, imgfx IMGFX_ANIM_SHUFFLE_CARDS and FLIP_CARD_1..3, npc_merlee.c:172-280, appendGfx_ritual_card).
2. FADE: the screen does not fade to black; instead the transparency of all on-screen objects is off for the time of the cutscene.
3. LAG: very slow at the start (after she spins and the light around Merlee starts) and at the end (the star effect on Mario).
Reproducing scenario: merlee_spell (azt_scenarios.json:7147, status unverified; padscript pad_merlee.txt, until "merlee: ritual state D (spell done)").

## Goal
Each fault traced by code to a file, line and value first (CLAUDE.md rule 5), then fixed.
1. Cards: the imgfx shuffle anim drives vertex positions per frame (imgfx anim keyframes); name what the port does with the anim vertices / matrix (interp, fp precision, the cached imgfx vertex buffer, the native draw of imgfx components) and why the motion is wrong.
2. Fade: find the call that fades the screen (set_screen_overlay / the ritual's fade, or a fill with alpha) and the render state the port uses; name why it switches off blending of other draws (a stuck render mode, combine or alpha compare left set, a fullscreen fill drawn without blend).
3. Lag: with prof=30 or a cls line in Azahar (direction only) find which producer class or effect (the light effect, the star effect: fx_* in src/effects) costs the frames; the console decides. Name the per-frame work (e.g. an effect rebuilt each frame, a texture re-upload, a fallback path).

## Start state
Worktree build3ds/wt/merlee, branch merlee from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
The paths the traces name. src/ game code only under #ifdef TARGET_3DS and only if the port path cannot express it. Do not edit src/msg.c.

## Done criteria
1. Traces for all three in the handback.
2. Build; nd_host PASS.
3. merlee_spell: shots of the shuffle show the cards in the N64 motion (3 cards moving apart and swapping), the fade shot shows the screen going dark with objects keeping their transparency; the scenario becomes verified.
4. Lag: the cls/prof numbers of the two slow parts before and after (Azahar direction); a console prof run is the final judge (the user pushes).
5. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they use the changed path (then name the pixels).
Affected scenarios (no others): merlee_spell, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=merlee PM_ROW=merlee on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause; a fault that fails twice on two causes goes to the handback, the others continue.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=merlee PM_LANE=merlee sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=merlee PM_LANE=merlee python 3ds/tools/bench.py run <bench> --row merlee --lane merlee --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "merlee ready <hash>".
