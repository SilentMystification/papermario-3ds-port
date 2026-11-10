# Row HC4e4: delete the word fallbacks

(Model: Haiku; after HC4f3. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e4`; PM_ROW=PM_LANE=HC4e4 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Work
- nd_walk_state: the !nd_op_typed branch becomes op_unknown.
- Delete the xlate branches of the readers (e2a/b/c), nd_op_type_run, the nd_rm_build xlate (use the typed ModelRenderModes directly), and the rec_op word path.
- nd_op.c leaves the ELF sources and stays in the nd_cmd_test build.
- Done-check: grep 3ds/src and src for `>> 24` against G_*/OP_*. List the hits in the commit. Expected: none outside host tools.
- Delete row_HC4.md.
## Gate
nd_host. Byte-equal of the DIRECT set. hotset_size.
