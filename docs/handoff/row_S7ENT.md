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

## Try 2 (ORACLE6, after try 1 at 1ab43e903: tab 22 notab 5 gapt 0 gaps 22; byte gates and nd_host pass)
- Cause of gapt 0 (code read of try 1): entity Render lists are call chains (Entity_YellowBlock_Render = CALL RenderSides, CALL
  RenderTopBottom; each = PipeSync, CycleType, CALL LoadTexture, geometry, VTX, TRI). ent_flat inlines the calls, so one flat
  gap spans units of the caller and the callees. The tables hold one def per list part, so ent_gapdef (exact match of start
  and length) never matches and every gap falls back to ent_sdef.
- Design, fixed: option (b). The gap runs the chain of table pieces that ent_tdwalk already finds, in flat order. Option (a)
  (new flat-gap defs in the tool) adds table data and a second analysis of the same units; option (c) keeps the run-time
  analysis the chain is meant to remove.
  1. ent_tdwalk records every op it passes in a gap, not only DEF ops: `{at, n, kind, p}`, where kind is DEF (p = NdGDef*) or
     UNIT (p = the unit). Sync and no-op units are skipped, as ent_flat counts them.
  2. A gap [a, b) is "table-covered" when the recorded pieces tile it exactly (consecutive, first at a, last ending at b; a
     piece that crosses a or b fails the tiling). EntSeg gets `uint16_t pc0, pcn` (a range in a per-key piece array, malloc'd
     with the key and freed in ent_sdef_free) instead of sd for such a gap.
  3. ent_gap: for a table-covered gap, apply the list start rules ent_gap applies today once, then run the pieces in order:
     DEF with `d.nv == d.pre` -> nd_scomp_exec_own(&g->d); a DEF with later values, or a UNIT -> nd_walk_state(unit, n). This is
     the op order (S6E rule). A gap that is not covered keeps ent_sdef (counted in gaps).
  4. Remove ent_gapdef's exact-match path (the chain covers it: a one-piece chain is the exact match).
  5. Census: `gapt` = gaps run as chains, `gaps` = gaps from ent_sdef in tabled lists, `gapn` = gaps of lists without a table.
- notab 5: hand back the list addresses with their symbols (ELF parse). SaveBlock.c stays out of the set (gen_gfx_syms does
  not handle its version-conditioned symbols); that fix is a later row.
- Memory: try 1 adds 136 KB static (margin 610 -> 466 KB, device o3ds_xl: memcheck OK, a soft limit). This row adds no table
  data. The S7EFX rows will add more; the oracle writes a table-packing row if the margin falls below 200 KB.
- Merge decision: try 1 does not merge alone. With gapt 0 the tables are unused at run time, so it would add 136 KB for no use
  (CLAUDE.md 7: no stubs). Try 2 continues on branch S7ENT.

## Gate (expected values)
- Build, memcheck OK (hand back the line; a margin below 0 is a stop: then drop files from the set).
- nd_host PASS: the gtab line with `bad 0` and the list count above S7RUN's (hand back the line).
- Bases: ent_kmr03_det, spot_mac01_det. One list `bench.py run ent_kmr03_det,spot_mac01_det,ent_kmr03_pic --row S7ENT
  --why "S7ENT: entity tables"` (3 starts, all normal): cmp 6/6 and 6/6 byte-equal; ent_kmr03_pic entsync `tab` above 0, `gapt` above 0 and `gaps` 0
  (hand back tab, notab, gapt, gaps, gapn, sc, walks). Budget 8 (reset once for try 2).

## Base of spot_mac01_det (ORACLE7, after try 2 at 3aa495373: not a row fault, no budget change)
- Why the base at 3d4bc81d5 was not stored: its two runs were one warm restart and one cold start after a failed warm restart
  (azslots/pool.log 2026-11-12 14:14:30-14:14:39, slot 2 pid 35736 `warm fail`, `start cold: restart failed`). They differed on
  0000_1_2.ppm. Cold and warm starts are not byte-equal for this scene (bench.py cmp refuses a cold row run for the same reason,
  TOOLB9); the base command took the cold run as a base run. A third (warm) run did not repair the pair. Tool fact, not a code
  fact: open a tool row (the base command must retry a cold run warm, as cmp says) if the orchestrator wants it.
- Rule for this row: 3d4bc81d5, ddc2f813e and f8b5d03fb have the same code tree (their diffs touch only docs/handoff and
  3ds/logs: `git diff --stat 3d4bc81d5 f8b5d03fb -- . ':!docs' ':!3ds/logs'` prints nothing). f8b5d03fb's spot_mac01_det base
  is stored (two warm runs). So the base of this row is that stored set:
  `bench.py cmp spot_mac01_det --row S7ENT --commit f8b5d03fb` (0 Azahar starts: the try 2 head run is the row run).
  Expected 6/6 byte-equal (the try 2 head shots have the md5 of every stored mac_01 base from 192232c37 to f8b5d03fb).
- If the orchestrator merges main into S7ENT first (needed anyway when gen_gfx_tab.sh conflicts), the new merge-base gets its own
  base: `bench.py base spot_mac01_det` (2 warm runs), then the list run again (3 starts; budget 8 holds it).

## Stop
A byte difference, gtab bad above 0, `tab` 0 or `gaps` above 0 in ent_kmr03_pic: stop, commit, hand back (2 tries per named cause). Delete this
row file in the completing commit. Hand back the file set of the `# S7ENT` line (the S7ENT1.. rows use it).
