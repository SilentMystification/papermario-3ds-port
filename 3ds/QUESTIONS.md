# Open questions for the user

Answered questions and the old text are in docs/HISTORY.md (section "Original QUESTIONS.md"). Answer by editing the Answer line or by telling the orchestrator.

## Q7. Memory: how to link the last 154 maps? (row S30b)
- Fact (Azahar numbers, O3DS values estimated, not hardware): image about 16.98 MB. Estimated O3DS spare about 518 KB against a limit of 500 KB. All maps plus the battle tree do not fit. Linking the other 154 maps (+2.0 MB) gives about -1.1 MB.
- Options: (a) stream the map data of rare areas. (b) stream the ROM from SD (Q2, saves up to 26 MB). (c) stay at 271 maps. (d) wait for the hardware run (S28).
- Recommendation: (d), then (b) if S28 shows a limit below 64 MB.
- Answer: (open)

## Q2. Keep the ROM resident or stream it from SD? (from M02)
- Fact: resident ROM is about 27 MB. Not measured on hardware. Stays resident until S28.
- Answer: (open, default: resident)

## PO2. Transform each vertex once at G_VTX? (row PO2)
- Fact: draw_tri in 3ds/src/pm_gbi.c transforms per triangle corner. A transform at G_VTX is CPU only and a small change.
- Recommendation: yes, after S28 gives real timing.
- Answer: (open)

## V06. Which problem do you mean? (row V06)
- In the Toad Town houses (Russ T, shop, dojo, waterfront) the game hides the outside ground on purpose when you enter (shroud fade). Do you mean this, or a ground that stays missing after you leave or in another building? Give the map, the building and a screenshot.
- Answer: (open)

## S28. Is the O3DS ready?
- Is the O3DS at 192.168.1.168 on, with dspfirm.cdc on the SD card?
- Answer: (open)

## Q5. Crash screen and obfuscation shims (row S31)
- Decision so far: the obfuscation shims stay empty. The crash screen works only if the debug output can be used. S31 checks. No user action.

## V07. Where do the items flicker? (row V07)
- I could not reproduce the flicker in Azahar. Item draws and pixels were stable in 8 maps. Which map, which item, and which camera move show it? Did you see it only on the O3DS or N3DS?
- Answer: (open)
