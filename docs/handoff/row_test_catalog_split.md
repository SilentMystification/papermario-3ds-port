# Row TCS: split docs/handoff/test_catalog.md (test catalog split into index and topic files)

## Goal
test_catalog.md is 98 KB. Agents read it whole. Make it an index of about 6 KB. Move the per-topic notes into small files.

## Start state
Branch nds1. Do not touch 3ds/src/map_restore_table.c. No game source changes in this row.

## Do (in order)
1. Read docs/handoff/test_catalog.md once. Lines 1 to 27 (how to use, the verified and unverified summary tables) stay in test_catalog.md.
2. For each "## " section from line 28 on (Verified, Unverified, POSHUD, POSHUD2, Completion only, Lane 3 notes, LOGCLEAN, MENULAG, ... to the end), write the section text unchanged to docs/handoff/test_catalog/<SECTION>.md (name = first word of the heading, upper case, for example POSHUD2.md, PAUSE5A.md). The two big tables "## Verified" and "## Unverified" go to test_catalog/VERIFIED.md and test_catalog/UNVERIFIED.md.
3. In test_catalog.md keep lines 1 to 27 and replace the moved sections with one table: topic, one-line summary (copy the first sentence of the section), file link. Keep every scenario name findable: add a table "scenario -> file" listing every scenario name that appears in 3ds/tools/azt_scenarios.json with the topic file that documents it (grep the name in the new files).
4. Check: no text was lost. Run `cat docs/handoff/test_catalog/*.md | wc -c` and compare with the old size (git show HEAD:docs/handoff/test_catalog.md | wc -c). The new files together must be within 3 percent of the old size plus the new index.
5. Check: test_catalog.md is under 10 KB. Every scenario in azt_scenarios.json appears in the index.
6. Commit docs/handoff/test_catalog.md and docs/handoff/test_catalog/ only. No attribution in the message.

## Blockers
- A section has no clear name: use the line number range as the file name suffix and say so in the index.
- Handback only when steps 4 and 5 pass.
