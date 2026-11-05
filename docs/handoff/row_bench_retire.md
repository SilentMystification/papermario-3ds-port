# Row BENCH5: retire dead benches, clean push recipe

## Goal
The walker model path was deleted (commit c856da583): the switches mdldiff (rec_diff) and texdiff (tb_bind, tb_run) no longer exist. Their benches cannot pass. The console push recipe adds stat switches that skew prof numbers.

## Start state
Branch nds1. Tools and docs only. No game source. No console push. Another session commits game source in this folder: stage and commit only your files with `git commit -- <file>`. Never touch map_restore_table.c. Files: 3ds/tools/bench_defs.json, 3ds/tools/azt_scenarios.json, docs/handoff/test_catalog.md.

## Do
1. Delete the benches mdldiff and texdiff from bench_defs.json (the run history in runs.jsonl stays untouched, append only). Delete the scenarios mac01_mdldiff, mac01_texdiff and mac01_walkdiff from azt_scenarios.json, and remove their names from the `legacy` list if present. Search both files and docs/handoff/test_catalog.md for any other reference to these names or to rec_diff / texdiff and fix it.
2. Set `push_base` in bench_defs.json to "" (empty): a console bench push line carries only the switches of the bench recipe (spot_*_prof: `warp=<map>:0 pos=x:y:z prof=4`). Check bench.py builds the push line correctly with an empty base (no double space, no trailing space). A bench that needs stat switches lists them in its own recipe.
3. In docs/handoff/test_catalog.md rule section: one line "Console recipes carry only the switches named in the bench recipe (prof=4 for spots). No stat switches by default: they change the frame time." Keep the file length.

## Done criteria (all must pass; handback only then)
1. `bench.py rebuild` clean, `bench.py list` no longer shows mdldiff and texdiff, `bench.py show spot_mac01_prof` prints a push command that contains no audiostat, texstat, pacestat or logstat.
2. `grep -n "mdldiff\|texdiff\|walkdiff" 3ds/tools/bench_defs.json 3ds/tools/azt_scenarios.json docs/handoff/test_catalog.md` prints nothing.
3. Commit only your files. No attribution. No push. Delete this row file in the last commit.

## At the end
Overwrite the BENCH4 STATE block of docs/handoff/agent_bench1.md with a BENCH5 STATE block (10 lines or fewer). Do not append.
