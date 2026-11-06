# Row GIT: branch cleanup, finish near-complete branches, bring main up to date

## Goal (user, 2026-11-06)
1. Merge every complete or nearly complete feature branch into nds1 (finish the small rest first).
2. Delete stale branches that newer work replaced or that conflict with newer work.
3. Bring main up to date with nds1.

## Start state
Main folder on nds1 (ecbfb479a or later). main is 40408f05f, 533 commits behind nds1. 22 local branches, 12 not merged into nds1. Worktrees: .claude/worktrees/agent-* on menu, s4, hud1 (hud1 is LOCKED: another session owns it).

## Steps
1. INVENTORY (no change): for each branch: tip, date, commits not in nds1 (`git log --oneline nds1..<b>`), files they touch, whether the same change is already in nds1 by content (`git cherry nds1 <b>`, or the subject in nds1's log), and a verdict: MERGED (all content in nds1), STALE (replaced by newer nds1 work, or conflicts with it and is older), NEAR (small rest left), OPEN (real unfinished work). Write the table in docs/handoff/git_branches.md (new file, INDEX.md line in the same commit) with the tip hash of every branch (the recovery record).
2. NEAR branches: merge into nds1 in a separate worktree (`git worktree add <scratch> nds1` is not possible while nds1 is checked out in the main folder; use `git worktree add -b merge-<b> <scratch> nds1`, merge there, build there with PM_LANE=merge, run the branch's own scenario once, then fast-forward nds1 with `git branch -f`? NO: nds1 is checked out. Instead: in the main folder run `git merge --ff-only merge-<b>` only if `git status` shows no conflict with the uncommitted files of other agents; else `git cherry-pick` the merge result files by pathspec). Finish the small rest in that worktree. One branch per commit.
3. Delete MERGED and STALE branches with `git branch -D` only after their tip is in git_branches.md. Remove their worktrees (`git worktree remove`) only for worktrees of this session's finished agents (menu, s4: check no process uses them). Never touch hud1 or a locked worktree. OPEN branches stay; list them for the user.
4. main: if main is an ancestor of nds1 (`git merge-base --is-ancestor main nds1`), move it with `git branch -f main nds1` (no checkout). If not, merge main into nds1 in a scratch worktree first, then move main. Never git push.

## Rules
Main folder: other agents (POND, BG1, BG2, WIDE1) have uncommitted edits on nds1. Never git checkout, switch, reset, stash, restore, clean, or commit -a in the main folder. Never commit 3ds/src/map_restore_table.c. Builds only in scratch worktrees under build3ds/ or .claude/worktrees with their own lane; copy assets/us and ver/us/build in, run gen_map_restore.py there, copy the main 3ds/src/map_restore_table.c over. No git push, no attribution. Hand back with the table (branch, verdict, action) and the new main and nds1 hashes.
