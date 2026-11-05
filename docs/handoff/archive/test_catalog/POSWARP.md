## POSWARP (2026-11-03): pos=<x>:<y>:<z>[:<angle>] places Mario after a warp

| Scenario | What it proves | Command | First marker | Runtime | Shot paths | Known limits |
|---|---|---|---|---|---|---|
| kmr10_pos (POSWARP, verified by log and picture) | pos=-142:20:221 places Mario on the kmr_10 floor. Start 8 (budget reset): readback `placed Mario at -142.000 20.000 221.000 ... settled -142.000 20.000 221.000`, 2 of 2 log checks, frame 56 shot shows Mario on the path by the fence (read by the code agent). | `azt.ps1 kmr10_pos -Row POSWARP -Build POSWARP -Try` | `warp: kmr_10 entry 0` | cap 45 s, deadline 35 s | build3dszt\shots\kmr10_pos | Gate: x and z within 0.5, y within 3 of 20. Starts 6 and 7 ended with Azahar exits (no result). Padscript token not run by this scenario. |
| kmr10_pos_hi (POSWARP, unverified) | pos=-142:136:221 (texture garble band view). Start 9: readback at frame 40 settled y 97.265 (still falling), x -142 z 221 exact, 2 of 2 log checks, frame 56 shot shows Mario on the path. Gate x and z only. | `azt.ps1 kmr10_pos_hi -Row POSWARP -Build POSWARP -Try` | `warp: kmr_10 entry 0` | cap 45 s, deadline 35 s | build3dszt\shots\kmr10_pos_hi | Y falls to the floor at 20 after the readback. Picture read by the code agent. |


