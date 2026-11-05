# agent_rk2: RK2 (texture versions in the record key, arena reuse, churn guard)

Plan: docs/handoff/plan_record_reuse.md row RK2. Base: HEAD a057a7ef1 (RK1). Map_restore_table.c not committed.
Status: gate passed. Commit hash in git log.

## Changed files
- 3ds/src/pm_gbi.c: RecKey 32 B (ver, gen, hgen, tgen0, tgen1, off, len, state, nb, s0, s1). rec_try compares the five fields
  and counts the miss per field (mv, mh, mt). rec_done stores hgen, s0, s1, tgen0, tgen1. A new build reuses the old place when
  the old state is 1 and n fits. nb counts builds since the arena reset; at 4, state 3 (no capture).
- src/model.c: pm_rec_tex(slot) = texture handle of the slot (same expression as pm_gbi_ms0_mark).
- 3ds/src/pm_tex.c, 3ds/include/pm_tex.h: pm_tex_cur_slots(&a, &b) returns cache slots of unit 0 and 1 (-1 none).

## Deviations from the plan
- s0, s1 are u16, not u8: SLOTS is 512 (pm_tex.c 21). REC_NO_SLOT = 0xffff.
- rec line: mv mh mt and ak appended after RK1 fields. ak = arena words used / 256 (KB). kb is still build bytes per window.

## Build
- Lane rk2h built from HEAD (before edits) for the baseline. Default build after the edits. No new warning in changed lines.

## Gate (window 2 = f614 line of mac01t_rec)
| Run | Build | Scenario | Result |
|---|---|---|---|
| RK2H 1 | HEAD (lane rk2h) | mac01t_rec -Try | f307 cap 303 build 79 emp 224 rb 0 ak n/a. f614 cap 0 build 0 wbad 0 rb 0. Top shots 0000_1_2, 0001_1_2 saved |
| RK2H 2 | HEAD (lane rk2h) | yard | f349 cap 48 build 9 emp 39 wbad 0. Top shot 0000_0_11 saved |
| RK2 3 | RK2 | mac01t_rec -Try | f307 cap 311 build 79 emp 232 rb 8 ls 4 mv 0 mh 0 mt 8 ak 16 wbad 0. f614 cap 4 build 0 emp 4 wbad 0 rb 4 ls 4 mv 0 mh 0 mt 4 ak 16. Shots 0000_1_2, 0001_1_2 byte-equal to RK2H 1 |
| RK2 4 | RK2 | yard | f349 cap 48 build 9 emp 39 wbad 0 rb 0 mv 0 mh 0 mt 0 ak 6. Top shot 0000_0_11 byte-equal to RK2H 2 |

- Window 2 cap 4 (gate: under 5). wbad 0. mt 4 (under 5). ak 16 KB (under 256 KB).
- Window 1 cap 311 (RK1 was 303). Not a gate item. The mt misses (8 in window 1) are texture cache slot changes. Cause not read.
- Picture of mac01t_rec and yard not read by eye in this row. Equal bytes to the HEAD shots.

## Open items
- Window 2 cap 4 is near the limit of 5. The four captures are the moved and empty models. A later row can count them per slot.
- mt misses in window 1 name the cache slot churn. A fresh investigation names the slot if the count grows.

## Commands
- Start: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scene> -Row <row> [-Build rk2h] [-Try]`.
- Rec line: `grep "rec f" build3ds/last_log.txt`.
- Never commit 3ds/src/map_restore_table.c.
