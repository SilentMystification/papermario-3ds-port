# agent_g2t: row G2T (g2_same in list runners) - result: REVERTED

## State
- Row G2T: skip G_TEXTURE and G_SETCOMBINE words with `g2_same` before `tex_op` in tb_run, tb_inline, rm_apply (pm_gbi.c).
- Result: quantity did not drop. Edit reverted with Edit. pm_gbi.c is at HEAD (git diff empty). No commit.
- Azahar starts used on row G2T: 5 of 8 (HEAD mac01t_cap1 x2, HEAD yard x1, modified mac01t_cap1 x2). Modified yard not run (gate stopped at the quantity).

## Trace of g2_same (not changed)
- G_TEXTURE: handler writes tex_on, tex_ss, tex_ts (equal in the skip case), then sync_combine(). sync_combine returns at once when sc_ok and sc_sig_now() == sc_sig, with only GP_SET(5) (debug counter, gbiprof only). The skip test is the same check.
- G_SETCOMBINE: handler writes comb_w0/w1 and derived fields (comb_a..comb2_ad, use_t1, box_folded, box_prod, comb_alpha_shade, cmb_gen_seen) then sync_combine(). Skip needs comb_set, box_folded == 0, cmb_gen_seen == comb_gen, sc_ok, sc_sig_now() == sc_sig. box_fold_sync (pm_gbi.c 585) bumps comb_gen when it changes box state, so the skip stops until the handler runs again. No other writer of the derived fields outside box_fold_sync.
- Conclusion: g2_same returns 1 only when the handler writes equal values and the side calls do nothing (except the gbiprof counters).

## Measurements (mac01t_cap1, phase fsplit, sp0c line at f483)
| run | lane | sp0c tex (us per frame) | shots vs HEAD |
|---|---|---|---|
| HEAD 1 | g2head | 1908 | baseline |
| HEAD 2 | g2head | 1911 | 2 of 2 byte-equal to HEAD 1 |
| mod 1 | g2t | 1934 | 0000 equal; 0001 truncated (run stopped early) |
| mod 2 | g2t | 1936 | 2 of 2 byte-equal to HEAD 1 |

- HEAD spread: 3 us. Modified: +26 to +28 us (rise, not drop). Gate "tex drops" failed.
- Hypothesis, not measured: the g2_same call (a switch per word, not inlined) runs inside the tex kind, and the skipped words are cheap handlers. The added cost may exceed the saved cost. The `dup` census (DUP0) was not run.
- yard: HEAD baseline saved (1 top shot, 0000_0_11.ppm, log 3 of 3). Modified yard not run.

## Files
- HEAD shots: C:\Users\Andrew\AppData\Local\Temp\claude\c--Users-Andrew-Documents-GitHub-papermario-3ds-port\d6d00d1c-7fdf-4fe7-b586-9f6c1655ea55\scratchpad\head_m1 (and head_m2, head_yard)
- Modified shots: ...\scratchpad\mod_m2
- Lanes: build3ds\lane-g2head (HEAD), build3ds\lane-g2t (modified, reverted in source)
- Status: 3ds\logs\perf\status_G2T.txt

## Commands
- Build: `python 3ds/tools/gen_map_restore.py`; `PM_LANE=<lane> PM_ROW=G2T sh 3ds/build.sh`; restore 3ds/src/map_restore_table.c from a copy (hash 92645afb, unchanged by gen).
- Test: `powershell -File 3ds/tools/azt.ps1 mac01t_cap1 -Row G2T -Build <lane> -Try`; `yard -Row G2T -Build <lane>`.
- Quantity: `grep sp0c` in %APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt.

## Open items
- The next step belongs to the orchestrator. Options: drop G2T, or try the check inside the handler path (a cheaper test) on a new row with its own gate.
- mac01t_cap1 second shot is written at frame 504; a run that stops at the until-line can cut it short. Check file size (288015 bytes) before a compare.
