# Row S5EMDL: entity models and animator nodes record a compiled state range, not state ops (NATIVEAPI E4 + E5)

Model: Sonnet (user decides).
- Why not Haiku: it is the first caller of the S6T compiler outside the S6T row. The state before each entity and node draw is
  chosen by an enum (32 render-mode lists), fog values and flags, and the range key must cover all of them.
- Why not Opus: the compiler form is fixed by S6T. This row only calls it.
- E4 (animator) and E5 (entity model) are one row: both prepend the same kind of state range to an NDC_ENT record
  (nd_rec_ent_draw).

## Blocker and base
S6T merged (nd_scomp.c: compiled ranges, range ids in an NDC_NAPI record). `sh 3ds/tools/wt_new.sh S5EMDL`; PM_ROW=S5EMDL
PM_LANE=S5EMDL.

## Code read
- appendGfx_entity_model (src/entity_model.c), per draw:
  - a matrix;
  - a state range: PipeSync, render mode TEX_EDGE, prim, combine MODULATEIA_PRIM, alpha compare, SETOTHERMODE_H.
  - or one of the 22 Gfx_RM1_* / Gfx_RM3_* render-mode lists chosen by model->renderMode, then the fog colour and position, clear
    lighting, combine MODULATEIA or PM_CC_42/MULTIPLY_SHADE or the shadow combine, and a segment;
  - the entity list (NDC_ENT).
  - napicen3: entity_model.o 78 (mac01), 36 (talk), 18 (btl).
- appendGfx_animator_node (src/animator.c:45-), per node: a matrix, PipeSync, texture off, LOD, LUT, clear lighting, cull back,
  combine DECALRGBA or PM_CC_42/MULTIPLY_SHADE, then nd_rec_ent_draw (DIRECT D3, :86-91), or for vertex-buffer nodes a built list
  (:153-201). napicen3 talk: animator.o 103.
- The state values per draw are few: renderMode (enum), fog (4 colour bytes, 2 distances), the flag bits that pick the combine,
  prim (white). Everything else is fixed per arm.

## Design
1. Replace each state range by one compiled-range reference from S6T: the range id is chosen from (arm, renderMode, combine
   choice), and the per-draw values (fog colour, fog position, prim) are carried as values. The matrix ops stay as they are (S6T
   keeps matrices as run-time ops).
2. Where the arm builds a list at run time (the animator vertex buffer nodes :153-201), keep the old path, counted.
3. NDC_ENT records are unchanged.

## Files and places
src/entity_model.c (appendGfx_entity_model under TARGET_3DS), src/animator.c (appendGfx_animator_node under TARGET_3DS).

## Gate (expected values)
- Build, memcheck OK; nd_host PASS.
- One list: spot_mac01_det (entities), msgbox_talk_nowide_pic or the talk det scene that napicen_talk uses (animators: use the det
  one; pre-flight `bench.py show` for a det talk scene with shots, and name it in the commit), mdl5_iwa10_det. 3 starts. Every
  shot byte-equal.
- napicen_mac01 and napicen_talk (2 starts): entity_model.o and animator.o drop to the per-draw values (hand back the numbers).
- `s6chk` (S6T debug switch) on spot_mac01_det: 0 mismatches.
- Budget 8 Azahar starts.

## Stop
A byte difference or an s6chk mismatch: stop, commit, hand back (2 tries per named cause). Delete this row file in the completing
commit.
