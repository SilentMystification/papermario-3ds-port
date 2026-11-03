# agent_uc3: handoff (row UC3, branch uc2c). Read this first.

## 1. State
- UC3a (plan_uc3.md section 4, pal-only reccheck mask) FAILED its gate. The edit is reverted (pm_gpu.c back to HEAD).
- UC3b, UC3c, UC3d, UC3e: NOT STARTED. The plan says UC3a must pass before the rest (plan_uc3.md section 2b, risk r1).
- Azahar starts used for UC3a: 3 of 8 (budget row UC3a).

## 2. UC3a result (window rec lines, same binary)
| Scene | Window | hit | bad | miss | wbad | top shot |
|---|---|---|---|---|---|---|
| yard_rec | f348 | 2003 | 0 | 0 | 0 | 0000_0_11 byte-equal to base_yard_rec |
| mac01t_rec_chk | f307 | 2878 | 184 | 0 | 0 | 0000_1_2, 0001_1_2 byte-equal to base_mac01t_rec |
| mac01t_rec_chk | f614 | 0 | 0 | 0 | 0 | not used |

Mask in the UC3a build: pal slots (idx in [u4_pal, u4_pal + 4 * GPAL)) and the 0x227 param0 masked. Texture address words 0x85, 0x95, 0x9D compared (UC2 masked them).
First bad word (UC2D log, mac01t f307): j 103, reg 0x081, header 0x804F0081, cmd 98, n 196, role data, a 040E8660, b 03025780.
Open question for an investigation agent: which uniform or texture word is per frame at reg 0x081 and at the 184 words. Do not guess a fix from this line (CLAUDE.md rule 3).

## 3. Commits on uc2c for UC3
- none for the code. The row files reverted. Commit of this file and the scenario cap change only.

## 4. Scenario cap change (coordinator rule, 2026-11-03)
- 3ds/tools/azt_scenarios.json: cap 90 to 45 (yard_rec), cap 120 to 45 (mac01t_rec_chk). Runs ended at 17 s to 21 s.

## 5. Open items
- yard_rec START 1 ended at frame 203 with Azahar exited and no rec line. The shared sdmc log.txt (%APPDATA%\Azahar\sdmc\3ds\PaperMario) was missing during that run. START 3 (same binary) passed the rec line. azt printed a false marker-missing check (exit 2) on START 3 because it read the shared log file. Use build3ds/lane-UC3/last_log.txt for the log.
- The Azahar data folder is shared by all agents. Runs can overwrite log.txt. The lane copy (last_log.txt) is the record.
- Console numbers: none.

## 6. Next steps
1. Orchestrator: start an investigation agent for the 184 bad words (window f307, first bad word j 103, reg 0x081).
2. Orchestrator: decide whether UC3 resumes (plan_uc3.md r1 says the plan stops).

## 7. Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `PM_LANE=UC3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from C:\Users\Andrew\AppData\Local\Temp\claude\uc3\map_restore_table.c.saved.
- Start: `PM_ROW=UC3a PM_LANE=UC3 powershell -File 3ds/tools/azt.ps1 <yard_rec|mac01t_rec_chk> -Row UC3a -Build UC3 -Try`.
- Process check: `Get-CimInstance Win32_Process -Filter "Name='azahar.exe'"`. Poll 10 s, wait up to 5 min. Never kill another run's Azahar.
- Status: 3ds/logs/perf/status_UC3.txt (justification line before each start).

## 8. Acronyms
UC3 = replay row. UC2 = reccheck row. bad = compared words that differ. miss = patch tables that differ. wbad = records that fail the walk. hit = compares. pal = matrix palette uniform. GPAL = palette size (21 matrices). reg = GPU register. UC2D = first-bad-word log line. sdmc = Azahar virtual SD card folder.
