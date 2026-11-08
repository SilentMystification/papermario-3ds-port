# Row AUDV: audio voices missing from time to time

## User facts (2026-11-08, console O3DS XL)
"We're still missing some audio channels from time to time, perhaps there are still some voices that are not being properly mapped?" No scene named; ask nothing more: find the cases by code and by the audiostat census.

## Leads (orchestrator code read, to check, not conclusions)
- 3ds/src/pm_dspaud.c:10: "the N64 voices play on ndsp channels 0 to 22 as mono PCM16" = 23 channels. 3ds/src/pm_au2.c:219 counts ticks with `used >= 24` (vs_full): the engine can use 24 voices. If the N64 engine asks for a 24th voice (or more), one has no ndsp channel. Name the engine voice count (AU synth voice pool, src/audio/*), the mapping engine voice -> ndsp channel, and what happens to a voice past 22 (dropped, stolen, or overwrites another).
- Other ways a voice goes silent: a voice start while the channel still has a queued wave buffer (ndspChnWaveBufClear timing), a sample whose decode is not ready at start (decode once at first play, A3), a non-ADPCM or looped sample path (bankstat "non-adpcm", "looped"), bus/fx sends folded to gain 0, pitch above the ndsp rate limit (vstat "pitch>2"), a voice update after a stop, an AU2 job dropped from the ring (AU2_SLOTS full), or audio on the wrong bank after a map load.

## Goal
Trace by code first (CLAUDE.md rule 5) and name each silent-voice cause with file, line and value; then fix each. Use the existing audiostat census (pm_au2_vstat, pm_dspaud.c stats) plus a new per-tick count of voices the engine starts that get no channel or are cut (one line per window under audiostat), in 3 scenes: a field map with music + sound effects (mac_01), a battle (battle_start_f182_cold), and a busy music scene (pick by the census: highest voice count). The audiohash (det) is a near-equality check (CORE2-1 notes); a fix that changes the audio output changes it.

## Start state
Worktree build3ds/wt/audv, branch audv from main (current). Copy assets/us and ver/us/build into it.

## Touch only
3ds/src/pm_dspaud.c, pm_au2.c, pm_audio.c, pm_rsp_audio.c, the audio files the trace names (src/audio/* only under #ifdef TARGET_3DS).

## Done criteria
1. Trace in the handback: engine voice count, the channel mapping, each silent cause.
2. Build; nd_host PASS.
3. The census line shows 0 voices without a channel and 0 cut voices in the 3 scenes after the fix (and the count before).
4. The 4 byte gates (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015) byte-equal (audio does not change pictures).
5. A console push (the user listens) to confirm; name the scene to test.
Affected scenarios (no others): the 3 audio census scenes, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11, docs/handoff/hw_guidelines.md. PM_LANE=audv PM_ROW=audv on every build.sh and bench call. No git stash. No toggles, no stubs. 2 tries per named cause. Scenario and bench in the same commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=audv PM_LANE=audv sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=audv PM_LANE=audv python 3ds/tools/bench.py run <bench> --row audv --lane audv --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "audv ready <hash>".
