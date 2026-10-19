# Questions for you (plain version)

Source: docs/plans/F11.md, section "Decisions for the user".

## Part 1: Options menu decisions

### D4: Which debug switches appear in the Options panel

**The question.** The Options panel can show switches that help testing. Which ones should the player see?

**What the player sees.** On the File Select screen, the player picks "Options". A panel opens with rows. Each row has a name and a value, On or Off. Up and Down move the cursor. A flips the value. B closes the panel.

**Choices and what they do:**

- **A. Four debug rows (plan default).** The panel shows "FPS counter", "Stats", "No fog", "No light". Each shows On or Off.
  - Consequence: the player sees a Debug section with four switches. "No fog" and "No light" change how the world looks.
- **B. Two debug rows.** The panel shows "FPS counter" and "Stats" only.
  - Consequence: the panel is shorter. Fog and light stay as the game draws them.
- **C. No debug rows.** The panel shows only "60 FPS", "Widescreen" and "Back".
  - Consequence: the debug switches stay in the file debug3ds.txt. The player cannot reach them from the menu.

**Plan default:** Choice A.

**Note.** "60 FPS" and "Widescreen" appear in every choice. They do nothing yet. The player can turn them On or Off, but the game does not change until later work is done.

### D5: How much of the controls the player can change

**The question.** The plan adds a "Controls" page. The player picks a button on the 3DS and then presses the N64 button to assign to it. How many 3DS buttons should the player be able to change?

**What the player sees.** The Options panel has a "Controls" row. A list of 3DS buttons opens. Each line shows a 3DS button and the N64 button it sends. A "Reset" row sets all of them back to the default.

**Choices and what they do:**

- **A. All 18 inputs.** The list has A, B, X, Y, L, R, ZL, ZR, START, SELECT, the D-pad (4 directions) and the C-stick (4 directions).
  - Consequence: the player can change almost every button. The plan keeps START and the circle pad fixed. ZL, ZR and the C-stick exist only on the N3DS. On the O3DS, those rows do nothing.
  - Risk: a bad setting can remove the A button. The "Reset" row fixes this.
- **B. Face and shoulder buttons only.** The list has A, B, X, Y, L and R (6 inputs). The D-pad and the stick keep their default.
  - Consequence: a shorter list. Less risk of a bad setting. The player cannot change the D-pad or the stick.

**Plan default:** The plan does not name a default for D5. The design table in the plan covers all 18 inputs. Choice A matches that table.

## Part 2: Golden intro picture

**Saved golden picture (the reference):**
- C:\Users\Andrew\Documents\GitHub\papermario-3ds-port\3ds\golden\intro.png (saved Oct 17 11:56)

**Newest test shot from the last golden run:**
- Main folder: C:\Users\Andrew\Documents\GitHub\papermario-3ds-port\build3ds\shots\0000.ppm (saved Oct 19 12:16). This file is a raw image (.ppm). The name does not say "intro". The last golden run wrote its shot to this same file name, but a later run also wrote here. Confirm it is the intro frame before you judge it.
- Lane B folder: no intro shot exists. Lane B's last golden run (Oct 19 12:12) passed only "battle". Its saved golden picture is C:\Users\Andrew\Documents\GitHub\papermario-3ds-port-laneB\3ds\golden\intro.png (Oct 18 01:46). Its size matches the main golden picture.

**What to compare:**
1. Open the golden intro.png and the test shot side by side. Convert the .ppm to PNG first if your viewer cannot open it.
2. Check that the title text and the picture layout match.
3. A scene fails when too many pixels differ. The limit is set in 3ds/tools/golden.py.
