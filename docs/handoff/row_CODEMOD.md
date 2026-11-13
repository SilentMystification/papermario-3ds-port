# Row CODEMOD: the rewrite tool for the value calls of the call sites (strategy A, Sonnet design core)

Model: Sonnet (strategy pick A + C + D2, user 2026-11-12). Reason: one tool decides the rewrite of about 2000 call sites. The
mapping, the cases it must refuse and its tests are design. Why not Opus: the native targets are fixed by plan section 2.

## Facts (ORACLE8, main 344822275, row_STRATEGY.md section 1)
- In src/effects and src/world (1720 active calls), 7 value calls make 73 %: gSPMatrix 294, gSPPopMatrix 237, gDPPipeSync 239,
  gDPSetPrimColor 188, gSPSegment 129, gDPSetTileSize 86, gDPSetEnvColor 82. The static list of an effect is called through
  nd_rec_dl (321 sites in src/effects, S7EFX). Idiom I1 (7 of 10 sampled files): PipeSync, Segment 0x09, Matrix push,
  PrimColor and EnvColor, static list, PopMatrix.
- Native targets of plan section 2: nd_xf_push(mtx, flags) and nd_xf_pop(which) (NXF1), nd_draw_list(list, entry) (CONV2F),
  colour values as draw values (NDN_COL, NPSO2: prim and env colour the uncoloured pso at the draw), a segment base as an input
  of the next static list (CONV2 section 7). Not yet in main: NXF1, CONV2F, the colour and segment calls.
- After N64DEL1-3 every call site is active (no `#ifdef TARGET_3DS` branch left).

## Design (fixed. The coder chooses the inner steps)
1. Tool 3ds/tools/codemod.py `[--dry] <file>...`: a tokenizer-level rewrite (not regex on whole lines): it finds each call of a
   mapped name with its full argument list (balanced parentheses, macros as arguments), checks the first argument is
   `gMainGfxPos++` (else refuse the site), and replaces the call by its native form. A site it cannot map exactly stays as it is
   and is printed `codemod keep <file>:<line> <name> <reason>`. Never a partial rewrite of a statement.
2. Mapping (the coder writes the native signatures in the commit and adds the missing record calls in a new 3ds/src/nd_val.c
   with their core 0 runners, NDC_NAPI records as nd_rect does): gSPMatrix -> nd_xf_push, gSPPopMatrix -> nd_xf_pop,
   gDPPipeSync -> removed (the native draw orders itself), gDPSetPrimColor -> nd_val_prim(minlod, lodfrac, r, g, b, a),
   gDPSetEnvColor -> nd_val_env(r, g, b, a), gSPSegment -> nd_val_seg(seg, base), gDPSetTileSize -> nd_val_tile_size(tile, uls,
   ult, lrs, lrt), nd_rec_dl(list) and gSPDisplayList(gMainGfxPos++, list) -> nd_draw_list(list, entry) with the entry the file
   states (a named ND_PSO_DESC). The runners do what the walker does for these ops today (same matrix palette, same colour
   values at the next draw), so the golden master of the file stays equal.
3. --dry prints per file `codemod file <path> value <v> keep <k> state <s>` (state = the other gDP/gSP calls the tool leaves to
   the hand rows) and a total. This is the share per group that the strategy names as unknown (row_STRATEGY.md section 5).
4. Tests 3ds/tests/codemod/: one input and one expected output file per idiom I1 to I4 and per refuse case (a call whose first
   argument is not gMainGfxPos++, a call inside a macro definition, a call split by a comment). test runs the tool and compares
   byte for byte.

## Readers and writers
Writers: codemod.py (call sites, run by the CODEMOD-FX and CODEMOD-WLD rows, not here), nd_val.c. Readers: the core 0 runners,
the golden masters (GOLD rows) that prove each rewritten file.

## Places
New 3ds/tools/codemod.py, 3ds/src/nd_val.c, 3ds/include/nd_val.h, 3ds/tests/codemod/. No game file in this row.

## Gate (expected values)
- `sh 3ds/tests/codemod/run.sh`: every case equal, a seeded fault (one wrong mapping) shown first.
- `python 3ds/tools/codemod.py --dry src/effects/*.c src/world/**/*.c`: totals printed, value share 70 to 76 % of the active
  calls of these files (the sample said 73 %). Hand back the per-group totals.
- Build, memcheck OK. nd_host PASS. No Azahar. Budget 0.
- Blockers for this row's runners only: NXF1 and CONV2F merged before nd_val.c calls them. The tool and the tests need none.

## Stop
A mapping the walker semantics of the op cannot give exactly, or a value share outside 70 to 76 %: stop, commit, hand back
(2 tries per named cause). Delete this row file in the completing commit.
