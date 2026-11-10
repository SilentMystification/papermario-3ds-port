# Row HC4: finish DIRECT with typed ops (HC4e, HC4f). Index row.
HC4a-d are done (11cf60814). Left: no N64 word read in the ELF. Sub-rows run in this order: row_HC4e1, row_HC4e2a, row_HC4e2b, row_HC4e2c (after SEAM2), row_HC4e3, row_HC4f1, row_HC4f2, row_HC4f3, row_HC4e4. HC5/HC6 follow (plan 5.3, 5.4).
Gates per sub-row: nd_host PASS; `bench.py cmp` byte-equal on spot_mac01_det and battle_start_f182_cold against stored merge-base shots; plus mdl5_iwa10_det for e3 and f2/f3. hotset_size after e4.
This row file is deleted by the HC4e4 commit.

## Oracle notes (batch 2)
Model per row: every row below can be Haiku. The one exception is HC4e3. I recommend Sonnet for HC4e3 because it flips the encoding of every static and local list for each base-macro group. A byte diff on its gate must be traced across the include/PR/gbi.h F3DEX2 variants, nd_gx_w and the consumers. I cannot split it smaller than one base-macro group, because one base macro builds several ops. If the user says Haiku, it stays Haiku with commits per group and 2 tries per group.
Order: HC4e1 -> HC4e2a, HC4e2b (in parallel) -> HC4e2c (after SEAM2) -> HC4e3 -> HC4f1 -> HC4f2 -> HC4f3 -> HC4e4.
The design point that makes it splittable: HC4e1 makes nd_op_xlate pass typed units through. Every reader that already calls it then accepts both forms, so each slice can be byte-equal.

## plan_hotcode.md 5.6 replacement (apply in the HC4e1 commit)
| HC4e1 | nd_op.h ND_X0/ND_X1 constant-expression translator (mirror of nd_op_xlate; MOVEWORD offsets by a constant chain, no division); nd_op_xlate passes a typed unit through (returns 1); nd_cmd_test.c equality | nd_op.h, nd_op.c, 3ds/tools/nd_cmd_test.c | - |
| HC4e2a/b/c | readers of static/local lists accept typed units (u = typed ? q : xlate(q)) and compare NDO_* ids; G_ENDDL writers write NDO_END | a: nd_ent.c; b: src/entity_model.c, animator.c, imgfx.c, model.c (R4), effects/hieroglyphs.c, underwater.c, energy_orb_wave.c, flo_10/waterfx.c; c: nd_shape.c | - |
| HC4e3 | gs* and g* base macros emit ND_X units at compile time, by group (scalar, texture, geometry and control); nd_gx_w reads NDO_RECT/NDO_END/NDO_CALL | nd_gbi.h, include/gbi_custom.h, nd_rec.c | - |
| HC4f1 | host converter 3ds/tools/nd_shape_conv.c: ROM asset table, Yay0, fix_shape mirror -> native LE shape file + reloc table, lists still words; build.sh runs it into build3ds/<lane>/romfs/shape/ | nd_shape_conv.c (new), build.sh | - |
| HC4f2 | romfs in the 3dsx (ctr_create_3dsx ROMFS, romfsInit), loader: read, relocate (one pass over reloc), shpchk equality with fix_shape, then fix_shape deleted | 3ds/CMakeLists.txt, pm_main.c, pm_map.c | ShapeFix word walk (pm_map.c:46-296) |
| HC4f3 | converter types the lists in place (one unit per word: same offsets) | nd_shape_conv.c | - |
| HC4e4 | delete the xlate fallbacks: nd_walk_state branch, reader branches, nd_op_xlate/nd_op_type_run/nd_rm_build out of the ELF (nd_op.c host-only), rec_op word path; done-check grep | nd_walk.c, nd_op.c, nd_rec.c, nd_recs.c, nd_ent.c, nd_shape.c, CMakeLists.txt, the 2b files | word readers, row_HC4.md |
Also replace 5.7 S2 "writes a sidecar ... NdShp{...}" with: "writes the whole decoded shape file, byte-swapped to native, with its lists typed in place (one 8-byte unit per word, so all offsets hold), plus a table of the offsets of every pointer field; the loader adds the buffer base to those fields (no list read, no node walk)."
Flag for the user: these romfs shape files are ROM-derived data inside the build output. ASSETFREE (parked) must later move them to first boot.
