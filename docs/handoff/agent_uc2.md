# agent_uc2: handoff (2026-11-01)

Epic UC2 (record check, reccheck: compare a stored display-list record with the words of a frame). Main folder, branch main. No push.

## State
- HEAD fdcca4d8f: UC2E, log-only. uc2d_first prints role (p0, hdr, data), ni (non-incremental bit) and doff (data index) of word j.
- Confirmed by one Azahar start (yard_rec, lane UC2, START 5 in 3ds/logs/perf/status_UC2.txt): `UC2D first j 75 a 00000000 b 00000110 reg 104 hdr 803f0104 cmd 72 n 196 role data ni 1 doff 1`.
  - Word j=75 is data word 1 of a non-incremental command. In a non-incremental command every data word writes the header register. So word 75 writes register 0x104 (the header register). This is CONFIRMED by the log line.
  - Record value a[75] = 0x00000000. Frame value b[75] = 0x00000110.
- Gate attempt 1 FAILED and was reverted (no code kept):
  - Change: RecKey got a field `at` (alpha test state, pm_gpu_atest(), r_atest at capture end). rec_check rejects a hit when `at` differs from the frame state and recaptures the slot.
  - Result (yard_rec, START 6, window 2 f349): rat 0, hit 2375, bad 4800, miss 0. Bad count unchanged. The rejection never fired, so r_atest at group end equals the record state for all 2375 hits.
  - Conclusion: r_atest is not the source of the frame value 0x110. The diag_reg104 hypothesis (alpha test state of the draw) is not supported by the test. Register name 0x104 = alpha test is still unverified (libctru header not on disk).
  - Reverted: pm_gpu.c, pm_gpu.h (checkout), and the alpha lines in pm_gbi.c. The mac01t_rec gate was not run (yard gate failed).
- Map user file 3ds/src/map_restore_table.c: NOT committed. INCIDENT: the first build in this session ran gen_map_restore.py, and my backup copy command failed (wrong temp path), so the session-start bytes of the file were not saved. The file now equals HEAD (git diff empty) and equals 3ds/src/map_restore_table.c.bak (Oct 23 copy, 75222 bytes). The session-start state was "M" (modified). The user must check whether the lost bytes matter. The second gen run left the file unchanged.
- Working tree: 3ds/src/map_restore_table.c is the user file (not staged). Nothing else in code is uncommitted from this agent.

## Budget (row UC2)
- 4 of 8 starts used (13:28 mac01t_rec, 13:29 yard, 14:08 yard_rec log-only build, 14:10 yard_rec alpha build). 4 remain.

## Open items
1. Source of the frame value 0x110 at register 0x104 (data word 75 of group cmd 72, yard_rec). The alpha test state (r_atest) is not the source. Next: a fresh investigation agent (log or code read), not a code agent. Candidate questions: which code path writes 0x104 besides sh_alpha_test (grep for 0x104 / 0x104u and GPUCMD writes of the register id); what the value 0x110 encodes in the PICA register layout (check libctru or citro3d headers when present).
2. Gate on mac01t_rec not run (blocked by open item 1).
3. Pictures: not compared for this epic (no draw change was kept).

## Next steps
1. Start a fresh investigation agent (docs/handoff/diag_reg104.md section 5 open points, plus the attempt-1 result above).
2. Do not start UC3 until the UC2 gate passes (rule 4 of the orchestrator handoff).

## Commands used
- Lane build: `python 3ds/tools/gen_map_restore.py`, then `PM_ROW=UC2 PM_LANE=UC2 sh 3ds/build.sh`. Restore the user file after the gen if it changed (cmp against a saved copy first).
- Test: `PM_ROW=UC2 powershell -File 3ds/tools/azt.ps1 yard_rec -Row UC2 -Build UC2 -Try`.
- Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt` (grep `UC2D first` and `rec f34`).

## Acronyms
UC2 = reccheck epic (record compare). UC2D, UC2E = log line rows of UC2. RC1 = record capture. RK1, RK2 = record key fixes. RecKey = record key struct. GPU = graphics processing unit. PICA = the 3DS GPU command set (PICA200). HEAD = last commit. Azahar = the 3DS emulator used for tests. ni = non-incremental flag (header bit 31). doff = data word index in a command. r_atest = alpha test state of the current render mode.
