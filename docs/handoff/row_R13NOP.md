# Row R13NOP: special-tile model records refused since HC4e4 (walls and floors missing on many maps)

(Model: Haiku. Base: main e15453f9c or later. Edits 3ds/src/nd_rec.c nd_rec_model only: disjoint from STACKANY, PMUROT, FXCEN2.
Reads only this file and CLAUDE.md.)

## User facts
O3DS: hos_01 (Shooting Star Summit) all walls missing (still after WALLSHADE); iwa_10 wall textures missing; "ian_08" walls and floor
missing, the missing floor is systemic.

## Cause (oracle code read, main e15453f9c)
- A model whose texture has an aux tile (EXTRA_TILE_AUX_INDEPENDENT or EXTRA_TILE_4) and a MODEL_PROP_KEY_SPECIAL property (src/model.c
  :1626-1638) writes make_texture_gfx (model.c:3491-3750) as R13 units after its record. Its macros (gDPLoadTextureTile,
  gDPScrollMultiTile, gDPLoadTLUT_pal16/256, gDPPipeSync) contain syncs. Since HC4e3/HC4e4 (51587f9ed, 2026-11-10) every macro writes a
  typed unit; a sync becomes the unit NDO_W0(NDO_MISC, NDO_MISC_NOP << 16) (nd_op.h:77, :115). nd_gx_w stores a unit written into a
  local buffer as it is (nd_rec.c:349: recBuf is not gMainGfxPos) and nd_rec_model copies the payload as it is (nd_rec.c:214). Before
  HC4e4 the record path dropped the syncs; the render mode table still drops them (nd_rec.c:142).
- The native draw takes an R13 texture block only when every unit is a texture, tile, load, othermode, combine or G_TEXTURE op
  (nd_model.c:77-98 nd_r13_texonly; its `case 0x00u` is the old raw G_NOOP word, never a typed unit). A NOP unit returns 0: r13 = 0,
  aux_ok = 0, and nd_model_state refuses with why 2 (nd_model.c:325-327). Since MDL-5e (856d74716) a refused record draws nothing
  (nd_recs.c:295: walker geometry only under ndmcheck or nomodelnat). The NOP units also count toward `n > 32` (nd_model.c:79).
- MDL-5e's gate had fb 0 on iwa_10 (before HC4e4). The later byte gates compare with bases that refuse too, so they stay equal.
- Fix: nd_rec_model drops the NOP units of R13, as before HC4e4 and as nd_rm_build does. Readers of R13 (the count q[26] and the units):
  nd_r13_texonly, nd_aux_state, hit_delta/hd_run (nd_recs.c:146), rec_aux (nd_recs.c:59), aux_hash (pm_gpu.c:3150), nd_mkey_a,
  rec_run's return count (nd_recs.c:301, :329), the POND nested-record skip (nd_model.c:290-296), mdl0_note2: all read the payload's
  count and units, so they stay right with fewer units; the walker skips a NOP anyway.

## Start
`sh 3ds/tools/wt_new.sh R13NOP`. PM_ROW=R13NOP PM_LANE=R13NOP on every build and bench call.

## Exact edits (3ds/src/nd_rec.c nd_rec_model, :208-220)
Keep the model_bad test and the rm_build line at the top. Replace the rest of the body by:
`const uint32_t* r = (const uint32_t*)rec; unsigned keep = 0;`
`for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) keep++; /* R13NOP: a typed sync is not an R13 op */`
`rec_open(); uint32_t* p = rec_take(2u + 2u * (G_PM_REC_WORDS + keep)); ndc_n[NDC_MODEL]++;`
`memcpy(p + 2, rec, G_PM_REC_WORDS * 8u); uint32_t* q = p + 2; uint32_t* o = q + 2 * G_PM_REC_WORDS;`
`for (unsigned k = G_PM_REC_WORDS; k < words; k++) if (!IS_NOP(r[2 * k])) { o[0] = r[2 * k]; o[1] = r[2 * k + 1]; o += 2; }`
`q[26] = keep; p[0] = NDC_HDR(NDC_MODEL, G_PM_REC_WORDS + keep, 0); p[1] = nd_mkey_a(q);`
IS_NOP is defined at nd_rec.c:125, above this function. Edit the comment above the function: the R13 syncs are dropped (R13NOP).

## Gate (CLAUDE.md 2; Azahar budget 6 of 8)
- Build, memcheck OK; `bench.py run nd_host --row R13NOP --why "R13 copy without syncs"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py base mdl5_iwa10_det`, `bench.py base sprshade_hos01_pic`; one list
  `bench.py run spot_mac01_det,mdl5_iwa10_det,sprshade_hos01_pic --row R13NOP --why "special-tile records drawn again"`; `bench.py cmp`
  of each. No pixel pass/fail: write the per-shot diff counts in the commit (changes in iwa_10 and hos_01 are the expected result).
- Mechanical gate: the `ndm f300 ... why ...` line of the mdl5_iwa10_det LOG: copy has no `2:` entry, or a smaller one than the base
  log (build3ds/basecache/<commit>/mdl5_iwa10_det/run.log in the main folder). Write both lines in the commit.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file
in the completing commit. Hand back the hash, gates, memcheck line, both ndm lines. The look is the user's console check.
