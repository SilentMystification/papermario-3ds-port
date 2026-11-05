# Investigation: doubled text and gloves in menus with a selection

Worktree: papermario-3ds-port-main, branch native. Read-only investigation. No Azahar start.

## Symptom (console, user)
Any menu with a selection (Toad House "Take a nap" choice) draws the text twice and two gloves as selector. Predates the second-eye pass. Stereo off.

## Ruled out (code reading)
- Double list build of the message window. `render_messages()` runs once per frame. The three call sites in src/main_loop.c:241, 257, 269 are gated by mutually exclusive flags (!CURTAINS && !FRONTUI, !CURTAINS && FRONTUI, CURTAINS). `draw_message_window()` and `msg_draw_choice_pointer()` run once per printer with flag MSG_STATE_FLAG_2 (src/msg.c:609, 624).
- `draw_msg()` (src/msg.c:2032, appendGfx_message at src/msg.c:2104) is the popup menu path only. The choice printer does not use it.
- Rainbow palette (src/msg_draw.c:1347) changes the glyph colour, not the count of glyphs.
- Stereo second pass (commit 6de4e162b): excluded by the user.

## Leading candidate (INFERRED, not proven)
The second text and the second glove are the drop shadow copies. They are drawn on purpose, at +2,+2 px, dark grey (40,40,40), with alpha 72:
- Text: src/msg_draw.c:1514-1526. Flag MSG_FX_FLAG_DROP_SHADOW is set on the selected option (src/msg_draw.c:1303). Render mode G_RM_XLU_SURF, combine PM_CC_07 (include/macros.h:327), prim colour alpha 72.
- Glove: src/msg.c:2213-2240 (msg_draw_choice_pointer). shadowAlpha = 72, draw at posX+2, posY+2 (src/msg.c:2232-2236). The glove itself is drawn at posX, posY with alpha 255.

Expected output on a correct build: a faint shadow. Bug output: the shadow copy is opaque. Then the user sees a second dark text and a second dark glove.

Why the port may draw the shadow opaque: PM_CC_07 alpha is TEXEL0 * PRIMITIVE alpha. The port maps combine modes in src/pm_gbi.c (set_leg, ~lines 560-600). For PM_CC_07 the colour sides have no TEXEL0, so wants_tex is 0 and wants_prim is 0. The mode is probably 0 (general combiner, src/pm_comb.c). The legacy alpha math `out[3] = t[3] * sa` (src/pm_gbi.c:985) ignores PRIMITIVE alpha, but that only runs for legacy modes 1 to 4. This is not verified. Which path PM_CC_07 takes is not confirmed.

Commits that touch the relevant code:
- af71c887b (S25): legacy combine mode 2 alpha, general combiner as PICA stages (pm_comb.c).
- 8384c4599: "Start a multi-stage alpha plan from zero" (pm_comb.c).
- The shadow code itself is upstream decomp (bfe9a42b0, 7a9df943a). It is not a port regression.

## Not done
- No Azahar shot. The prior ui1_choice shots (test_catalog.md line 63) show no menu at all, so a glove shot is not possible on the current scenario. A draw census (drawcensus=200) needs a working menu first.

## Proposed next step
1. Add a single debug log in the combiner path for PM_CC_07 (mode, and the output alpha for PRIMITIVE alpha 72). Run ui1_choice once with drawcensus=200 after the menu opens. Check whether the glove shadow draw has alpha 255.
2. If the alpha is 255, fix the combiner so PRIMITIVE alpha multiplies TEXEL0 alpha for this pattern. Fix it in the shared path, not per caller.
3. If the alpha is 72, the copy is not the cause. Then run the census to count draws per glove.
