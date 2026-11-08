# Row MSGBOX: message box faults (wide star, selector double text, rainbow text lag, tattle bubble)

## User facts (bugs, 2026-11-08, console, wide mode)
1. STAR: the star that appears on the text box (the "next" / end-of-page star or the box decoration) is not aligned with the box in widescreen mode.
2. SELECTOR DOUBLE TEXT: the choice/selector text box still draws the text twice; the copy behind is not turned into the grey shadow (on N64 the second pass is the drop shadow in dark grey, offset).
3. RAINBOW LAG: the multi-item selector is sometimes slow when there is a lot of rainbow text. User: "Is this implemented efficiently?"
4. TATTLE (was row TATTLE): Goombario's overworld tattle bubble (not battle) is sometimes stretched: the part outside the 4:3 range is a blank white box with no text, so the oval looks asymmetric.
Reproducing scenarios: find or add one per fault (a field NPC talk with a multi-page message for the star; a choice prompt for the selector; a shop or item list with rainbow text for the lag; Goombario tattle near an NPC or block for the bubble). Wide mode on, as the user plays; check wide off too.

## Goal
Each fault traced by code to a file, line and value first (CLAUDE.md rule 5), then fixed:
1. Star: name the star's x in wide mode against the box x (the box may move by pm_wide_* while the star keeps a 4:3 offset, or the reverse).
2. Double text: the message draw makes two passes (shadow then text); name why the shadow pass keeps the text colour on the port (the shadow colour set via prim/env colour or a palette swap that the native draw ignores, a combine mode not decoded, or a state that the text batch reuses).
3. Rainbow: rainbow text sets a colour per character per frame; name the per-character cost on the port (one draw/flush per glyph, a texture or palette upload per colour change, a template rebuild) and make it batch (per-vertex colour, one draw per run of glyphs). Measure the draws per frame before and after (count, valid in Azahar).
4. Tattle: name the bubble's x/width and the text scissor in wide mode; the part beyond 320 px is drawn but the text is clipped or not placed.
src/msg.c may be edited here (the user's working copy differs only in line endings). Keep src/ changes under #ifdef TARGET_3DS where they differ from N64.

## Start state
Worktree build3ds/wt/msgbox, branch msgbox from nds1 (current). Copy assets/us and ver/us/build into it.

## Done criteria
1. Traces for all four in the handback.
2. Build; nd_host PASS.
3. Picture gates (wide on): star on the box; selector text with a grey shadow, no double text; tattle bubble symmetric with all text. Wide off: the same scenes byte-equal to the base unless the trace shows the N64 path itself was wrong on the port (then name the pixels).
4. Rainbow: draws per frame in the selector scene before and after; frame cost direction in Azahar; the console decides.
5. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal unless the trace shows they draw text (then name the pixels).
Affected scenarios (no others): the 4 fault scenarios, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=msgbox PM_ROW=msgbox on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause; a fault that fails on two causes goes to the handback, the others continue. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=msgbox PM_LANE=msgbox sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=msgbox PM_LANE=msgbox python 3ds/tools/bench.py run <bench> --row msgbox --lane msgbox --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "msgbox ready <hash>".
