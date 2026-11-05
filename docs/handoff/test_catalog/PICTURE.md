## Picture compare rule (NP1B, 2026-10-29)

- Tool: `python 3ds/tools/shot_cmp.py <dirA> <dirB>` (copy the shot folder of each run first, `build3ds\azt\shots\<scenario>`, because the next run of that scenario overwrites it).
- Top-screen shots (every .ppm whose name does not start with `bot_`) must be byte-equal. The tool exits 1 on any difference or on a name mismatch.
- Bottom-screen shots (`bot_*.ppm`) are not compared. HUD numbers and the refresh interval change between runs of the same binary.
- Proof: two HEAD runs of yard (build lane nphead, source at HEAD e16fee4b9) give 1 of 1 top shots byte-equal (`0000_0_11.ppm`).
- The earlier NP1 drop (18 to 140 px on yard bottom shots) was a bottom-shot difference, not a top-shot difference.

