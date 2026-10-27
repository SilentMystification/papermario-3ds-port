# diag_tex2: texture quadrant bug (A) and file select flip bug (B)

Branch worktree-agent-a4e68c678c783b5dc. Row tex2, lane tex2. Azahar starts used: 4 of 8.

## Fixes (commits stay)
- 87b05658d (A): `3ds/src/pm_tex_tx1.h` tx1_build_native read IA4 as one byte per texel (LA4 target), so the draw used 2 source rows per texel row. Result: 2x2 copies and half garbage. It now reads one nibble per texel (3 bit I, 1 bit A). Picture: the Mario House sign (kmr_20, intro, shot frame 210) reads "MARIO". Before the fix: repeated small text and an orange block.
- a03daae9c (B): `3ds/src/pm_gpu.c` pm_gpu_off_begin has 4 full size off-screen slots (OFF_N = 4). The file select flip begins a 5th box in frame 150 (log "V05: more than 4 off-screen images"). That box drew on the screen, so its contents showed at the top left. The slots are now used again in turn. Picture: flip frames 152 to 159 show the title, Delete File, Copy File, Cancel, Stereo and Mono contents turning with their boxes. No text shows at the top left.

## Checks
| check | result |
|---|---|
| SUPER text (multibounce) | NOT RUN. The intro has no battle, so it does not show this text. fx_attack_result_text loads an IA4 64x32 image (D_09001658_391A28), which is the same code path as fix A. |
| statehash base (d6a5899c5 files, build 1183-tex2), `fast preset=toad battle fixedseed shots shotwin=300:3 statehash=100` | toad f100 94083528, battle f200 dda688fa, f300 6b6fe648 (battle starts at scene frame 100) |
| statehash fixed (HEAD), same switches | NOT RUN. The golden.py battle call printed RUN FAILED and did not start Azahar. The log stayed the base log, and the budget shows no new start. |
| golden fileselect | NOT RUN |
| golden intro (Mario House) | NOT RUN. Expect a difference: the reference frame most likely has the garbled IA4 sign (the intended fix A). |
| golden battle | NOT RUN (RUN FAILED, no start) |

## Next
- Run `PM_ROW=tex2 PM_LANE=tex2 python 3ds/tools/golden.py --extra "statehash=100" battle`, then `fileselect intro`. Compare the battle statehash lines with the base values above. Find out why golden.py returned RUN FAILED (it printed the last 3 lines of the run script output; the lock or a duplicate-run refusal are possible).
- Fix B leaves an earlier image unfound when it is read after its slot is used again in the same frame. No such read is seen in the file select.
