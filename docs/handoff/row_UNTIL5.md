# Row UNTIL5: an Azahar run ends within 5 s of its until-line (tool bug)

## User facts
CLAUDE.md rule 1 TIME: a test ends within 5 s of its until-line. Row AUDNAP (2026-11-09): 2 runs of toad_nap_aud matched the until
at 7.8 to 9.3 s but ended at 38 to 40 s (33.7 s after the until). Runs: runs.jsonl ids 161071, 161683. Bench toad_nap_aud
(azt_scenarios.json, until "audio drops w\d+: ...", cap 45).

## Goal
Find by code read why the run kept going after the until matched, fix it, and show one run that ends within 5 s of its until.

## Start state
`sh 3ds/tools/wt_new.sh UNTIL5`, branch UNTIL5. PM_ROW=UNTIL5, PM_LANE=UNTIL5. One Haiku agent.

## Premise and its check
Premise: the until match is seen late or ignored (log flush delay, a second condition such as a wait for the padscript end or a
shot-cut wait, or a regex matched on a later poll). Check first by code read of 3ds/tools/azt.ps1 and 3ds/run_azahar.ps1: name the
file, line and condition that keeps the run alive after the match. Also check the run log timestamps of run 161683.

## Touch only
3ds/tools/azt.ps1, 3ds/run_azahar.ps1, 3ds/tools/bench.py (only if the until handling is there). No game source.

## Done criteria
1. The named cause (file, line) in the commit message.
2. One run of a short until scenario that is not audio-specific (pick one with until in under 15 s) ends within 5 s of its until.
   One run of toad_nap_aud ends within 5 s of its until. At most 3 Azahar starts in total (diagnostic row).
3. Commit; delete this row file in the completing commit.

## Rules
CLAUDE.md. STOP RULE: 90 minutes or 150 tool calls. No Monitor or tail loops.
