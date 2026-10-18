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
