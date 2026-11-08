# Row TATTLE: overworld tattle speech box stretched, one side blank

## User facts (bug, 2026-11-08, console, wide mode)
User: "Sometimes goombario's text box when using his tattle ability in the overworld (NOT BATTLE) is stretched weirdly. The part that renders off of the original aspect ratio is not rendered at all, its just a white text box with no text and it is weirdly stretched making the oval text box look asymmetrical."
Trigger: field map, Goombario out, press the partner ability (C-down / tattle) near an NPC or entity (src/world/partner/goombario.c, N(EntityTattles) and the world tattle path). "Sometimes": find when (speaker position near a screen edge? long text? which message style).
Expected: the oval speech bubble at its 4:3 shape, symmetric, all text drawn. Reproducing scenario: add one (a field map in wide mode, Goombario tattle on a known NPC or block; until-line a log line at the message open; shot after the box is open).

## Goal
Trace by code first (CLAUDE.md rule 5): the overworld tattle message uses a speech bubble style (msg_draw / msg_draw_speech_bubble or the MSG_STYLE of the tattle message, src/msg.c and src/msg_draw.c or equivalent). Name the box x, width and the scissor/clip of the text in wide mode. Likely: the wide-mode rule moves or widens the box (edge vertices extended to the screen edge, or the box x placed by pm_wide_* while the text scissor stays at the 4:3 range), so the box part outside 320 px is drawn but the text is clipped. Name the file, line and value.
Do NOT edit src/msg.c: the user has uncommitted changes there. If the fix belongs in src/msg.c, hand back with the exact lines.

## Start state
Worktree build3ds/wt/tattle, branch tattle from nds1 (current). Copy assets/us and ver/us/build into it.

## Touch only
The wide-mode draw path the trace names (pm_wide*.c, nd_rect.c, the window/box code), or the message draw file that holds the speech bubble other than src/msg.c.

## Done criteria
1. Code trace in the handback: box x/width/uv and the text scissor in wide mode, and the failing value.
2. Build; nd_host PASS.
3. Picture gate: the new scenario shot shows the tattle bubble symmetric with all text drawn, in wide mode, in the case that failed (near a screen edge if that is the trigger).
4. Wide off: the same scenario byte-equal to the base. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they show a speech bubble in wide (then name the pixels).
Affected scenarios (no others): the new tattle scenario (wide and non-wide), the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=tattle PM_ROW=tattle on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=tattle PM_LANE=tattle sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=tattle PM_LANE=tattle python 3ds/tools/bench.py run <bench> --row tattle --lane tattle --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "tattle ready <hash>".
