## UNTILCHK (2026-10-29): until check and stall stop (azt.ps1)

- Static check before each start (no start on a fail): the until line must hold each literal run of 4 or more letters in the C source of 3ds/src, 3ds/include or src. A frame in the until (`frame N\b`) must sit in shotwin when the debug string sets one. An `fN` frame must be at most cap times 60. A fail exits 7 with "SCENARIO REJECTED".
- Stall: no new log line for 40 s after the Azahar start stops the run and exits 6 (SCENARIO FAILED: no new log line). Not seen live: the yard run with `preset=yard` logs fs, dlc and audio lines every few seconds, so no quiet run was made. The rule itself is in the self-test.
- Self-test (no Azahar start): `powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 -SelfTest`. Writes 4 scenario copies to %TEMP% with bad or good until lines and checks the stall rule. Result: SELFTEST PASS (5 of 5). Current scenarios: no NOTE line (0 of 36 rejected).

