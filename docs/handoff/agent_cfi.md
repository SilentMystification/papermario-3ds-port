# agent_cfi: epic CFI state (CACHE-FIT, instruction side)

Branch: main. Commits in this epic:
- f0001e7a4 CF0 (pmu switch: PMU window line, cache counters).
- 90868fa2f CF1 (gbi_run_body rare cases and dlc replay out of line).
- 055800175 CF2 (draw_tri CPU clip path out of line).
- 3cb770ee7 CF3a (tex_op debug out of line, dead settdbg probe removed).
- 12001e468 CF3b (texture miss path out of line, load_miss).
- 595cb8efc CF4 (hot text sections first in 3ds/ld/3dsx.ld.in).

## State
All five rows are done and committed. Each row passed its gate (table below). Epic CFI is complete. The Azahar check passed. The push command is for the orchestrator (rule 6).

| Row | Map quantity (before -> after) | Azahar mac01t_fsplit gbi f900 | Shots |
|---|---|---|---|
| CF0 | pmu line present, rc 00000000 (yard_pmu, 1 run) | n/a | yard picture read |
| CF1 | gbi_run_body 17692 -> 9204 B (target 10 KB) | base 6.8 / 6.7, new 6.7 / 6.7 | mac01t top 2 of 2 x 2 equal; pause_menu top equal |
| CF2 | draw_tri 11256 -> 740 B; hot draw_tri + gv_tri 3256 B (target 3500) | 6.7 / 6.7 | 2 of 2 x 2 equal |
| CF3a | tex_op 8812 -> 7728 B; dlc_try 1908 B | 6.7 / 6.7 | 2 of 2 x 2 equal |
| CF3b | load_impl 16036 -> 4084 B, load_miss 12024 B; load_impl + flush_draw 17796 B (target 21000) | 6.7 / 6.7 | 2 of 2 x 2 equal; pause_menu equal |
| CF4 | hot span 2.9 MB -> 57704 B (= byte sum) | 6.7 / 6.7 | 2 of 2 x 2 equal |

## Notes per row
- CF0: the PMU counts are read at the window end (one line per 300 drawn frames), not per frame. This is 0 svc calls per frame, not 2. Azahar returns garbage counter values (per-frame counts near 2^32/300); the console values are the real result.
- CF1: the dlc compile and replay helpers were outlined too (not only the rows named in the plan). Without the dlc replay outline, gbi_run_body stayed at 14.4 KB. The pm_gbi.c cast `(Gfx*)dlc_end` removes a new warning (no code change).
- CF2: GCC placed gv_tri and draw_tri_cpu in .text.unlikely. Hot set is still draw_tri + gv_tri (3256 B). Azahar shows no change.
- CF3a: the settdbg block computed a flag nothing read. It was deleted.
- CF3b: flush_draw is not changed, because the sum was already under the target. The plan's flush_draw moves (CF3b second half) are open if the console needs them.
- CF4: the hot list uses section names (-ffunction-sections), not PM_HOT source tags. The list is in 3ds/ld/3dsx.ld.in lines near `.text`.

## Open items
- DLF6 (TXR1) is still not started. The plan orders DLF before CFI. CFI ran without it.
- docs/handoff/test_catalog.md: HEAD has 13 BEL bytes (0x07) from an earlier write. The working copy has them as the text backslash-a now. This change is not committed. The orchestrator decides whether to commit it.
- 3ds/tools/azt_scenarios.json: the working copy still has another uncommitted hunk (the shot 252 to 253 change). Not mine.
- Console: the pmu line gives the miss counts. Read rc at the epic push. A nonzero rc means delete the CF0 code in the next epic (plan rule).
- CFD (CF5 to CF7) is not started.

## Azahar budget (row starts used)
CF0 1, CF1 6 (incl. pause_menu base and new), CF2 2, CF3 5 (CF3a 2, CF3b 2, pause 1), CF4 2. All under 8.

## Commands
- Build: `python` is not on PATH. Use `/c/Python314/python 3ds/tools/gen_map_restore.py`. Save 3ds/src/map_restore_table.c first and restore it after. Then `PM_ROW=<row> sh 3ds/build.sh`.
- Test: `PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 mac01t_fsplit -Row <row> -Try [-Build <lane>]`. The baseline lane is build3ds/lane-cf0base (CF0 binary).
- Map sizes: `/c/Python314/python` with the mapsz helper (scratchpad mapsz.py). Cold functions sit in `.text.unlikely.<name>`, so a `.text.<name>` search misses them.
- Instruction bytes per source line: docker `arm-none-eabi-objdump -d -l --disassemble=<fn>` at /opt/devkitpro/devkitARM/bin. Each ARM instruction is 4 bytes.
