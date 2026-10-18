# Open questions for the user

Answered questions and the old text are in docs/HISTORY.md (section "Original QUESTIONS.md"). Answer by editing the Answer line or by telling the orchestrator.

## PO2. Transform each vertex once at G_VTX? (row PO2)
- Fact: draw_tri in 3ds/src/pm_gbi.c transforms per triangle corner. A transform at G_VTX is CPU only and a small change.
- Recommendation: yes, after S28 gives real timing.
- Answer: (open)

## S28. Is the O3DS ready?
- Is the O3DS at 192.168.1.168 on, with dspfirm.cdc on the SD card?
- Answer: (open)

## Q5. Crash screen and obfuscation shims (row S31)
- Decision so far: the obfuscation shims stay empty. The crash screen works only if the debug output can be used. S31 checks. No user action.

## 2026-10-18 plans F06, F07, F09 (docs/plans/F06.md, F07.md, F09.md)
- F09: the US build has no German, French or Spanish text. Option A: US only. Option B: a second PAL build from a PAL ROM you give. Recommendation: decide after S28. Slices F09-1 to F09-4 and F09-6 wait for your answer.
- F06: 60 fps cannot run on the O3DS (extra frame about 21 ms, budget 16.7 ms). The plan makes the toggle N3DS only. The matrix record needs 80 to 144 KB of heap (over the 10 KB rule). Do you approve both?
- F07: widescreen costs about 1 to 4 ms per frame on the O3DS. The plan keeps it off by default there. Do you approve?

## 2026-10-18 V12c memory (over the 10 KB slice rule)
- V12c added 22.8 KB (3dsx 13.02 MB) for the fright jar and snowfall data. This is real game data. Without it the effects draw nothing. The same data-layout bug probably hits 12 more effect files (row V12d, about 18 KB). Recommendation: yes, approve V12d. Check docs/MEMORY_BUDGET.md first (the estimated O3DS spare was 518 KB). V12d waits for your answer.
- The texture cache peaks at 4092 of 4096 KB at kzn_22 (row V13c). It has almost no spare. S28 on the O3DS shows the real limit.
