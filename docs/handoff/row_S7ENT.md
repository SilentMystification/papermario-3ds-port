# Row S7ENT: entity model lists through the S7 tables (GCONV_SRC over src/entity/model; ent_compile takes the table's defs)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: ent_compile (nd_ent.c:201-264) builds EntSeg segments and S6E gap defs from its own flatten of the list. Taking
  them from the table must give the same segments, gaps and draw order. That is a choice inside the entity path shared by every
  entity draw. The tool must also accept the entity sources' symbol kinds (INCLUDE_IMG/PAL, Mtx).
- Why not Opus: the table form, the tool and the host compare exist (S7CONV, plan_nativeapi.md section 6); S7RUN runs tables.
- Why not smaller: the input set has no reader until ent_compile uses it, and that use has no proof without the host compare.
  The C call sites of src/entity (about 169 gDP/gSP lines in setupGfx functions) are NOT here. Haiku rows S7ENT1.. per file
  group come after this merges (the S6E2/S6E3 pattern plus nd_rec_dl).

## Facts (ORACLE6 correction, after the coder's read at a57c824cb; the first version of this row assumed ROM blobs: wrong)
- include/entity.h:64-67: under TARGET_3DS `ENTITY_ADDR(entity, type, data)` is `(type)(data)`. The model data (vtx, gfx,
  textures) is compiled and linked in place, so a list pointer is the ELF symbol (Chest.c:100-104, HiddenPanel.c:31).
- src/entity/model/*.c (72 files) use C symbols: `Vtx Entity_X_vtx..[]`, `Gfx ...[]`, `Mtx Entity_X_mtx.. = RDP_MATRIX(...)`
  (ArrowSign.c:32, BlueSwitch.c:37), and `INCLUDE_IMG/INCLUDE_PAL` symbols named D_0A00xxxx (ArrowSign.c:17-18). Those are
  link-time symbols, not segment literals. The words are ND_GS typed units (3ds/include/nd_gbi.h), the S7CONV effect form.
- The heap copy of the ROM blob (src/entity.c:892-897, :1049) reaches the draw only as entity->gfxBaseAddr ->
  model->vertexArray -> gSPSegment(D_80154374) (src/entity_model.c:506-507, :855-856). That is the animated or per-entity
  vertex segment, not a walked list.
- nd_ent.c: ent_compile flattens (ent_flat :132) into segments and S6E gaps (ent_sdef :197, a calloc and an analyse per gap at
  run time). `useseg` (:214) marks a list whose VTX sources are segment addresses (why 9; ent_gdl keys those by ent_segkey :339).
- gen_gfx_tab.sh:25 builds the input list as `src/effects/gfx/$g.c` from GCONV_SRC. gen_gfx_syms.py:10 scans `Gfx` and `Vtx`
  definitions plus INCLUDE_IMG/PAL/RAW. Mtx is not scanned, so a gsSPMatrix of an entity Mtx stays an NDG_UNIT, which is correct
  (a value unit).

## Blocker and base
S7RUN merged (done: 6220a2775). Branch S7ENT at a57c824cb: `git merge main` first. PM_ROW=S7ENT PM_LANE=S7ENT.

## Design (fixed; the coder chooses the inner steps)
1. Input set: gen_gfx_tab.sh accepts a GCONV_SRC entry that ends in `.c` as a path (used as given); other entries stay effect
   names. The `# S7ENT` line lists the src/entity/model/*.c files whose Gfx symbols the device build links: the tables must not
   pull model data the game does not link. Check each file's main Gfx symbol in build3ds/lane-S7ENT/pm_3ds.elf .symtab, with the
   python ELF parse of the oracle 5 note. Name the files left out in the commit. One line, sorted.
2. Tool: no new op and no SEG flag. VTX, CALL, IMG and PAL pointers map to device link addresses through the symbol map, as for
   the effects. Mtx pointers stay NDG_UNIT. If a list fails the tool (exit 1), drop its file from the set and name it.
3. ent_compile: for a list l, `t = nd_gtab_find(l)` (core 1; ent_compile runs where today). With a table:
   - the S6E gap defs come from the table's NDG_DEF ops (`&g->d`, st 1, analysed on the host: no ent_sdef calloc), mapped to the
     same gaps by unit index (NdGDef k0 and the def's n);
   - a gap the table splits differently (a def boundary inside one gap, or an NDG_UNIT inside it) keeps ent_sdef for that gap;
   - segments and the draw stay ent_compile's.
   Without a table: today's path, counted.
4. useseg lists (why 9) are out of this row: they stay as they are, counted.
5. Census: the entsync line (nd_ent.c:318) gets ` tab <n> notab <m> gapt <a> gaps <b>`: lists with a table, lists without one,
   gaps taken from the table, gaps from ent_sdef.

## Readers and writers
- Writers: gen_gfx_tab.sh (the `.c` path rule and the `# S7ENT` line), gen_gfx_syms.py (only if a symbol form of the entity
  sources is missed: name it), nd_gfx_conv.c (only if needed).
- Readers: ent_compile and ent_sdef use (nd_ent.c), nd_gtab_find (nd_gtab.c), nd_part_draw (entity tables have no NDG_F_PARTS:
  no change), nd_gtab_test.c (its compare covers the new lists without an edit; check that it runs them), memcheck (const data
  added). grep `nd_gtab_find\|ent_sdef` in 3ds before the commit; name each reader in the commit message.

## Files and places
3ds/tools/gen_gfx_tab.sh (line 25 rule and the `# S7ENT` line only), gen_gfx_syms.py (only if needed), 3ds/src/nd_ent.c
(ent_compile, the entsync line). No src/entity edit in this row. S7EFX rows edit only their own GCONV_SRC lines.

## Gate (expected values)
- Build, memcheck OK (hand back the line; a margin below 0 is a stop: then drop files from the set).
- nd_host PASS: the gtab line with `bad 0` and the list count above S7RUN's (hand back the line).
- Bases: ent_kmr03_det, spot_mac01_det. One list `bench.py run ent_kmr03_det,spot_mac01_det,ent_kmr03_pic --row S7ENT
  --why "S7ENT: entity tables"` (3 starts, all normal): cmp 6/6 and 6/6 byte-equal; ent_kmr03_pic entsync `tab` above 0 and
  `gapt` above 0 (hand back tab, notab, gapt, gaps, sc, walks). Budget 8.

## Stop
A byte difference, gtab bad above 0, or `tab` 0 in ent_kmr03_pic: stop, commit, hand back (2 tries per named cause). Delete this
row file in the completing commit. Hand back the file set of the `# S7ENT` line (the S7ENT1.. rows use it).
