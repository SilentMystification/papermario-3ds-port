# diag_toadt: mac_01 producer ranking in the tour view (Azahar)

Epic TOADT, rows T0 and T1. Diagnostic only. Azahar numbers show direction only.
Build: lane toadt (HEAD 41662a62a plus the T1 census window reset). Scenario mac01t, window mdlsum f899 (frames 600 to 899, all on mac_01).

## Census state
- T0 (commit 41662a62a): render task ops go to pseudo IDs. 0xF001 transform group, 0xF002 model group, 0xF0FF other task, 0xF0FE tail.
- T0 check, scenario mac01 (warp view, f299): total ops 1165 (same as HEAD). 00b7 is out of the top 12 (ops 7 or less). 0xF0FF 439 ops. PPM byte-equal to HEAD.
- T1 fix: the census table (128 slots) is cleared at each report. Before the fix, 646 ops per frame were unattributed in the tour view. After it, 62 ops per frame.

## Tour view, mdlsum f899, total 2315 ops per frame
| rank | producer | ops per frame | share of total | tris per frame | draws per frame | binds per frame |
|---|---|---|---|---|---|---|
| 1 | task other (0xF0FF) | 1308 | 56.5 % | 202 | 58 | 0 |
| 2 | task tail after last task (0xF0FE) | 332 | 14.3 % | 0 | 0 | 0 |
| 3 | model 0006 | 37 | 1.6 % | 0 | 0 | 6 |
| 4 | model 0049 | 9 | 0.4 % | 0 | 0 | 1 |
| 5 | model 0043 | 9 | 0.4 % | 0 | 0 | 1 |
| 6 | model 0048 | 9 | 0.4 % | 0 | 0 | 1 |
| 7 | model 0064 | 9 | 0.4 % | 0 | 0 | 1 |
| 8 | model 0098 | 9 | 0.4 % | 0 | 0 | 1 |
| 9 | model 0044 | 8 | 0.3 % | 0 | 0 | 1 |
| 10 | model 0066 | 8 | 0.3 % | 0 | 0 | 1 |

- Unattributed ops: 62 per frame (2.7 %). Models in the window: 84, static 77.
- All 58 draws and all 202 tris are in the task other bucket. Map models replay through dlc, so their draws and tris do not show as model counts.
- Sum of model rows (84 models): about 613 ops per frame (26 %). Task other plus tail: 1640 ops per frame (71 %).

## Gate result (T1)
- mdlsum f899 total 2315 ops per frame. Gate 1900 to 2200 FAILED (above limit).
- mdlsum f599 total 2205 ops per frame (same run, window 300 to 599, includes the arrival frame).
- mdlsum f299 total 1684 ops per frame (window includes mac_00 frames). Not a mac_01 window.
- Picture of shot 500 (build3ds/azt/shots/mac01t/0000_1_2.ppm): Mario and two Toads on a Toad Town street, field HUD on. Subject present.
- The earlier mac01 warp view gives 1165 ops. The tour arrival gives about 2200 to 2300. The camera and state differ.

## Reading
- OPS2 named appendGfx_model as the largest producer (79 to 94 % on console). Its producer census was not re-run here. The model census (mdc) gave task ops to the last model mark before T0.
- Before T0, mdlsum put 755 ops on models in the warp view. After T0 it puts about 571 (1165 minus 439, 95 and 60).
- In the tour view the model share is about 26 %. Task other (sprites, entities, custom gfx) is 57 %. The OPS2 share needs a check with the T0 build on console.
- Next: T2 splits task other into sprite and entity draws. The tail (332 ops) has no owner yet. Console re-measure needed with T0 build (phase on) before the share is used.
