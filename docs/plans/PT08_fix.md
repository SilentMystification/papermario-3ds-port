# PT08: Star Spirit text boxes, text effects (findings)

Status: partial. One root cause proven (NOISE input). The other effects are CPU side and were not shown broken. Evidence level per effect below.

## What the Star Spirit boxes use
- assets/us/msg/0B_Prologue.msg and the chapter files: every Star Spirit line starts with `[Style right]`, `[Voice star]` and `[DitherFade 217]` (44 lines in all msg files). No Wave, Shake or Rainbow in these lines.
- `[Static 50]` and `[Static 30]` appear in 04_ToadTownBridge.msg (10 lines), 0D_Chapter2.msg (6), 19_MapTattles.msg (3), 10_Chapter5.msg (1). `[Wave]` (212) and `[Shake]` (213) are in many files.

## Effects by evidence level
| Effect | Code | Result |
|---|---|---|
| MSG_FX_STATIC | src/msg_draw.c:1137 to :1145, combiner PM_CC_MSG_STATIC = (NOISE - TEXEL0) * ENV + TEXEL0 (include/macros.h:495) | BROKEN, proven by code. NOISE reads 0 (pm_comb.c:64, same cause as PT07). The text is multiplied by (1 - env). No static grain. |
| MSG_FX_NOISE_OUTLINE | src/msg_draw.c:1132 to :1135, PM_CC_MSG_NOISE_OUTLINE = NOISE * TEXEL0 (macros.h:494) | BROKEN, proven by code. With NOISE = 0 the glyph is black. No message uses it (no `[NoiseOutline]` in assets/us/msg). |
| MSG_FX_DITHER_FADE (the Star Spirit boxes) | src/msg_draw.c:1353 to :1354, G_AD_NOISE and G_AC_DITHER with alpha 217 | APPROXIMATED. pm_gbi.c:530 handles only G_AC_THRESHOLD. G_AC_DITHER becomes a plain alpha blend of 217/255 through PM_CC_02. No grain, no fault. Likely not what the user saw. |
| MSG_FX_SHAKE, WAVE, GLOBAL_WAVE, SIZE_JITTER, SIZE_WAVE, RISE_PRINT, GROW_PRINT, DROP_SHADOW, BLUR | src/msg_draw.c:1322 to :1560. Only CPU position, scale and rand_int. No combiner input outside the port set. | NOT FOUND BROKEN by code reading. Not run (the 3 Azahar starts of this investigation went to PT09, the bisect and PT12). Evidence: guess. |
| MSG_FX_RAINBOW, GLOBAL_RAINBOW | palette index per glyph (msg_draw.c:1347 to :1349, palette reload with MSG_PRINT_FLAG_10) | Not checked. Related to PT04 (rainbow text double image), see the PT04 investigation. |

## Fix
- STATIC and NOISE_OUTLINE: the NOISE input of PT07 (docs/plans/PT07_fix.md). No change in src/msg_draw.c.
- DITHER_FADE: no fix needed for the look. If the user wants grain, the same noise texture can drive an alpha test, but the PICA alpha test compares against a constant only. Option: draw the glyph with alpha = 217/255 (now) and accept.
- Test: one start on the Toad Town bridge scene text with `[Static 50][Shake][Wave][PrintRising]` (msg MAC_Bridge_0026) after the PT07 fix, and one Star Spirit box shot. Compare the Shake and Wave lines in the same shot.
