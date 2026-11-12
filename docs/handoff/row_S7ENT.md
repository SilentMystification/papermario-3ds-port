# Row S7ENT: entity model lists converted at build time (segment tables) and used by ent_compile (NATIVEAPI S7, entity design core)

Model: Sonnet (pre-approved for the night run).
- Why not Haiku: a new converter input (the entity model blobs, linked at segment 0x0A as the ROM holds them) and a new run-time
  key (blob, segment offset) that ent_compile must use; the tables must give what ent_flat and ent_sdef make from the raw words
  today. These are design choices in the build and in nd_ent.c (shared by every entity draw).
- Why not Opus: the table form (NdGTab, NdGDef from nd_scomp_analyse on the host), the tool and the host compare exist (S7CONV,
  plan_nativeapi.md section 6); the runner exists (S7RUN). This row adds an input set and a lookup, not a new form.
- Why not smaller: the converter output has no reader until the lookup exists, and the lookup has no proof without the host compare.
  The C call sites of src/entity (about 169 gDP/gSP lines: setupGfx functions) are NOT in this row: Haiku rows S7ENT1.. per file
  group are written after this merges (the S6E2/S6E3 pattern plus nd_rec_dl).

## Facts (ORACLE6 code read, main 8301c42fb)
- Non-animated entities copy their ROM blob into the entity heap: src/entity.c:892-897 and :1049 (`dma_copy(bp->dma.start,
  bp->dma.end, gfxData)`); the words are the N64 big-endian list words with segment 0x0A addresses. Animated ones use the compiled
  C nodes (entity.c:886-890, :1076-1090, vertexSegment 0xA). The blobs: 72 ranges in 3ds/stub_include/entity_model_offsets.h
  (gen_entity_rom.py from splat.yaml); their decomp sources are src/entity/model/*.c (72 files; names such as D_0A000080_E59F80
  are segment addresses).
- nd_ent.c: ent_compile (:201-264) flattens a list (ent_flat :132; raw words typed through nd_op.c:14) into segments and gaps
  (EntSeg, nd_int.h:342; the S6E gap def ent_sdef :197, a calloc per gap); `useseg` (:214) refuses a list whose VTX sources are
  segment addresses (k->why 9); ent_gdl (:343-376) keys useseg entries by ent_segkey (:339, the hash of segbase[1..15]).
- ptr_of resolves a segment address through nd_ws.segbase (set_mw NDO_MW_SEG, nd_set.c:112-115).

## Blocker and base
S7RUN merged (nd_gtab_run, nd_rec_dl; its `# S7ENT` line in gen_gfx_tab.sh). `sh 3ds/tools/wt_new.sh S7ENT`; PM_ROW=S7ENT
PM_LANE=S7ENT. Base `git merge-base HEAD main`.

## Design (fixed; the coder chooses the inner steps)
1. Input: the non-animated entity blobs whose source is in src/entity/model/. gen_gfx_tab.sh compiles each such source for the
   host and links it at 0x0A000000 (one link per blob: a linker script with `. = 0x0A000000;` and the data of that object only),
   so every pointer word is the segment address the ROM blob holds. The symbol map (gen_gfx_syms.py) records each symbol's link
   address (a segment address) and its blob (the entity_model_<Name>_ROM_START value).
2. Host compare first (nd_host, a new 3ds/tools/nd_ent_blob_test.c): for each blob, the host-linked bytes, swapped to big-endian
   per word, equal the ROM range bytes of ver/us/baserom.z64 (ASSET_BASE, as gen_shapes reads it). A blob that differs is left
   out of the set (tool output `gconv ent <name> skip blob`) and never converted.
3. Tables: one NdGTab per Gfx symbol of a blob, flags `NDG_F_SEG` (new bit): NDG_VTX p and NDG_CALL targets are segment
   addresses (0x0Axxxxxx), resolved at run time with ptr_of; NDG_DEF and NDG_UNIT p point into the host-built copy that the
   device build links as const data (the typed units of the same list), so defs are analysed on the host.
4. Lookup: `const NdGTab* nd_gtab_find_seg(uint32_t rom_start, uint32_t seg_addr)` (nd_gtab.c, a second sorted index
   nd_gtab_sidx[] {rom_start, seg_addr, tab}). Run time: src/entity.c (TARGET_3DS, after each dma_copy of a blob) calls
   `nd_ent_blob_add(void* heap, uint32_t rom_start, uint32_t len)` (nd_ent.c, a small table, cleared at map load with the entity
   heap); ent_compile, for a list l inside a registered blob, takes the table by (rom_start, 0x0A000000 + (l - heap)).
5. Use: with a table, ent_compile builds the EntSeg gaps from the table's NDG_DEF ops (the def is &g->d: no ent_sdef calloc, no
   analyse at run time) and keeps its segment and draw logic; useseg lists (why 9) take their vertex sources from NDG_VTX p
   through ptr_of at compile, so they compile instead of walking. A list without a table keeps today's path (counted).
6. Census: the entsync line (nd_ent.c:318) gets ` tab <n> notab <m>` (lists compiled from a table, lists without one).

## Readers and writers
- Writers: gen_gfx_tab.sh, gen_gfx_syms.py, nd_gfx_conv.c (the SEG flag), src/entity.c (blob registration).
- Readers: nd_gtab_find_seg (core 1), ent_compile (nd_ent.c), nd_gtab_run (NDG_F_SEG: VTX and CALL through ptr_of: add the arm),
  nd_part_draw (must refuse NDG_F_SEG tables: particle lists are effects), nd_gtab_test.c (round trip of the SEG form).
  grep `NDG_F_\|nd_gtab_find` in 3ds and src before the commit; name each reader in the commit message.

## Files and places
3ds/tools/gen_gfx_tab.sh (the `# S7ENT` line and the entity compile and link block), gen_gfx_syms.py, nd_gfx_conv.c, nd_gtab.h
(flag, find_seg), nd_gtab.c (end), nd_gtab_test.c, nd_host_test.sh (the blob compare line), 3ds/src/nd_ent.c (ent_compile, the blob
table, the entsync line), src/entity.c (two TARGET_3DS lines after the dma_copy calls :897, :1049), nd_walk.c (nd_gtab_run SEG arm).
S7EFX rows touch only their GCONV_SRC line and src/effects: no overlap. S7TEX2 edits nd_gtab_run's NDG_TEX arm: if both run, the
later one merges main first.

## Gate (expected values)
- Build, memcheck OK (the tables add const data: hand back the memcheck line; a margin below 0 is a stop).
- nd_host PASS: the blob compare line `ent blobs <n> equal <n>`; gtab `bad 0` with the entity lists.
- `bench.py base ent_kmr03_det`, `bench.py base spot_mac01_det`; one list `bench.py run ent_kmr03_det,spot_mac01_det,
  ent_kmr03_pic --row S7ENT --why "S7ENT: entity tables"` (3 starts, all normal): cmp 6/6 and 6/6 byte-equal; ent_kmr03_pic
  entsync `tab` above 0 (hand back tab, notab, sc and walks). Budget 8.

## Stop
A byte difference, a blob compare difference on a blob the tool converts, or gtab bad above 0: stop, commit, hand back
(2 tries per named cause). Delete this row file in the completing commit. Hand back the converted blob list (the S7ENT1.. rows
use it).
