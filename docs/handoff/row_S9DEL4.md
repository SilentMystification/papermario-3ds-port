# Row S9DEL4: delete the Gfx type, gMainGfxPos and every N64 `#else` list branch from the game code

Model: Sonnet (user decides).
- Why not Haiku: about 4000 call sites under `#else` across src (effects, world, entity and root files), plus the display context
  structs (gDisplayContext->mainGfx, backgroundGfx, matrixStack use) and the nusys task code (nuGfxTaskStart with list pointers).
  The coder removes them file group by file group and keeps the 3DS paths byte-equal.
- Why not Opus: deletion.
- Split into worktrees by directory if needed: S9DEL4a src/effects, S9DEL4b src/world, S9DEL4c src/entity and src/battle,
  S9DEL4d the root files and include. The display context and nusys go in the last one.

## Decision (2026-11-12, chosen for the user as best practice)
Delete the N64 branches; the port stops building for N64. Recovery: the tag `pre-list-removal` (S9DEL1 step 0).

## Blocker and base
S9DEL3 merged. `sh 3ds/tools/wt_new.sh S9DEL4a` (and b, c, d).

## Pieces
- Every `#else` (or `#ifndef TARGET_3DS`) branch that holds gDP/gSP/gs macros or Gfx data: delete the branch and its `#if`
  lines.
- The Gfx type use in src and include: the Gfx members of the display context, `Gfx*` parameters, `Gfx X[]` arrays left after
  S7.
- gMainGfxPos and nd_rec_task's list role: the frame stream keeps its own writer pointer under a native name.
- gbi.h includes in src and include.

## Gate (expected values)
- Full build; nd_host PASS.
- The byte set of S9DEL1: every shot byte-equal.
- Per worktree: `grep -rnE "\bg[DS]P[A-Za-z]|\bgs[DS]P[A-Za-z]|\bGfx\b|gMainGfxPos" <its directories>` returns nothing.
- listcen build, gfxpos, data and gfxtype 0 for its directories. Budget 8 each.

## Stop
A byte difference or a build error that needs a design change: stop, commit, hand back. The last worktree deletes this row
file in its completing commit.
