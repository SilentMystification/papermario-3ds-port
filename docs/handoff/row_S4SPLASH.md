# Row S4SPLASH: water_splash drops in one particle run with one previous-frame bind (NATIVEAPI C2)

Model: Sonnet (user decides). Why not Haiku: step 2 changes when the unit-1 window flushes a batch (t1_fold), a rule other
two-texture draws share; the byte result must stay equal for them. Why not Opus: one effect, one named flush arm.

## Blocker and base
CONSOLE C2 done (XL `fxtest=all` after S4E3D, S4SNAK, S4WBLK); S4PDIR merged (the setter runner). Base `git merge-base HEAD main`.
`sh 3ds/tools/wt_new.sh S4SPLASH`; PM_ROW=S4SPLASH PM_LANE=S4SPLASH.

## Facts (code read)
- water_splash_appendGfx (src/effects/water_splash.c:169-233): per drop a gDPLoadMultiTile of a 32 x 32 window of the previous colour
  buffer into tile 1 (tmem 0x100, RGBA16, width 320, window (t2 + 0xA0, t3 + 0x78) to (+0xBF, +0x97), wrap, masks 5 and 5), then
  gSPMatrix (push, mul), gDPSetPrimColor, gDPSetEnvColor, gSPDisplayList(D_E00BEAF0[(i + frameCounter / 2) & 3]), gSPPopMatrix. The
  G_PM_ND_SPLASH bracket (:191, :230) makes grab_cur copy the buffer once per frame (nd_target.c:76-82).
- fxcen xl4a: water_splash walk 12.9 ms; FXFNCOV: class D and E per drop (a flush per drop).
- On core 0 each drop's window moves only the tile 1 origin. nd_part_draw (nd_geom.c:385-388) calls t1_fold_tri, and "a changed
  window map flushes the open batch (flush_gen)": one batch per drop.

## Step 1 (the particle run, no rule change)
Put the drops in an NdParts run: pre units = the LoadMultiTile units (as the macro expands into a local Gfx array), the matrix, prim
and env; the call; post = pop. Count the units of gDPLoadMultiTile from its macro (gbi.h) and set pre to match. Keep the SPLASH
bracket outside the run. If nd_parts_end's decoder (S4PDIR) rejects TIMG, TILE or LOADTILE units, the run takes nd_parts_run (the
unit path). This is correct, only slower; count it in partsrun.
Gate 1: fxpic_89 byte-equal (base at the merge-base); fncov fxfncov_89: nd_part_draw above 0.

## Step 2 (no flush when only the unit-1 origin moves)
In t1_fold_tri's window-change arm: when the bound unit-1 texture, its wrap and its size are the same and only the origin differs,
fold the new origin into the copied vertex texture coordinates (the fold is already per vertex) and do not flush. A different
texture, wrap or size still flushes. Readers of the same arm: every two-texture draw on the ring (S4COL particles, KZN04 shaded
sprites, BOMBLIT). Grep t1_fold_tri callers and list them in the commit.
Gate 2: fxpic_89 byte-equal; fxpic_105 (embers: tile sizes per particle on tile 1) byte-equal; spot_mac01_det byte-equal. fncov
fxfncov_89: flush_draw per frame drops from about the drop count to at most 3. Budget: 8 Azahar starts for both steps.

## Stop
Step 1 failed: stop before step 2. A byte difference in step 2: stop and revert step 2 in a new commit, then hand back. Delete
this row file in the completing commit. Console: CONSOLE C3 after S4INST.
