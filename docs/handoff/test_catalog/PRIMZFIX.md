## PRIMZFIX (2026-10-29): toad pond depth, one unit nearer

Fix: `prim_ndc_z()` in 3ds/src/pm_gbi.c (the zmix and CPU z uses), prim_z - 1, clamped at 0. Repro switch `zround` (3ds/src/pm_gpu.c): zmix.y rounded to 2^-16 steps on the upload. Row PRIMZFIX. Builds: lane pzhead (HEAD d99eea457), pzold (HEAD plus zround only), pzfix (fix plus zround switch, same code otherwise). Shot compare: `shot_cmp.py` is not byte-equal here; see the result column.

| Scenario | What it proves | Command | Result | Status |
|---|---|---|---|---|
| toad_pond (PRIMZFIX) | Pond, mac_00 entry 4, route to x -174 z 112. HEAD run 1 and run 2 (lane pzhead): top shots not byte-equal, all 10 diffs in the NPC strip x 92 to 138. Fixed code, switch off (lane pzfix): same as HEAD outside x 92 to 138 in all 10 shots. The water at x 20 to 60 is identical in every run. | `toad_pond -Try -Row PRIMZFIX -Build pzfix` | 3 of 3 log checks, 4 runs | unverified |
| toad_pond_zround (PRIMZFIX repro) | Old code with zround (lane pzold): pond water unchanged from HEAD (teal water at x 20 to 60). Does not reproduce the console bug at the 2^-16 rounding. Fixed code with zround (lane pzfix): same picture. | `toad_pond_zround -Try -Row PRIMZFIX -Build pzold` | 3 of 3 log checks, 2 runs | unverified (does not reproduce) |

