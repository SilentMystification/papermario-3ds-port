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

## ANSWERED 2026-10-18 (user)
- V12d memory: YES, approved (about 18 KB).
- F09: US only. Languages may come from a PAL ROM later. No 25/50 Hz mode.
- F06 and F07: toggles for N3DS and O3DS, wired to the UI later. Not before optimization. Build 4:3 for now. The earlier N3DS-only limit and the 80 to 144 KB heap question are replaced by this: re-check cost on the O3DS in the plan phase after optimization.

F06-6 (2026-10-19): BUDGET 4/4 used. fps60+wide tour faults in load_mtx at area 6 map 5. Reset needed to continue. Details in the row.

V16 (2026-10-19): No flicker frame found. Name the scene, enemy and attack where the box shows (or a save), then one run with drawlog on that frame closes it. 1 of 4 starts left in row V16.
V19 (2026-10-19): BUDGET 4/4 used. Azahar shows no Sushie shadow and no jump in jan_04 (frames 170 to 320). Question: is the flicker seen on Azahar or only on the O3DS and N3DS? Which shadow flickers, Sushie or another one?

FX2a: BUDGET 4/4 used again on 2026-10-19 (second budget). No user decision needed. The row needs a new budget for one test start: the noise scroll fix (t1_fold) and the darkness stencil check are coded or open and not tested. State in the FX2a row of 3ds/SLICES.md.

F07-7 (2026-10-19): BUDGET 4/4 used. Code is done (frame grab 400 columns). Open: golden toad with `wide` off (expect 0 diff) and one fxtest=54 shot pair with the glow centre within 40 px of a screen edge. Needs a reset of 1 start. No user decision needed.
F07-7 (2026-10-19, coordinator add): the overlay edge fix (pm_gpu_vert_clip, pm_gpu_tex_rect) is coded and not tested, BUDGET 4/4. Needs 2 starts in total (this test and the golden toad / edge glow test).

V26 (2026-10-19): BUDGET 4/4 used. Code: osGetCount fix and switch simlog. Needs 1 start with a per-step Mario and Bullet Bill log for demoscene=4 (see row V26). V25: 3 of 4 used, 1 start left for the one line per strip log. V24: 0 starts used, not started (V26 and V25 first).
