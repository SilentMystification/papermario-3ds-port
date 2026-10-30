# agent_dlf: epic DLF state

Branch: main (the run before used thunder-box; that branch is no longer the base). Commits in this epic:
- 37fb8665c DLF1 (gbidump removed, streak calls gated on pm_ph_on).
- b17262105 DLF23 (PDL compiler pdl_tpl/pdl_try_m/pdl_try/pdl0, GbiState, PSO path deleted; pm_pdl.h removed; flush_tev_rebuild noinline).
- 640255940 DLF4 (entity render-mode list as G_PM_RM).
- 4267a3536 DLF5 (sprite setup lists as G_PM_RM; rm_apply runs G_SETCOMBINE and G_TEXTURE through tex_op).

## State
- DLF23 gate: gbi_run_body + flush_draw 33868 -> 31372 B (gate 31868 or less). mac01t mdlsum f899 tot ops 1335 (HEAD 1335, gate 1175-1495). Top shots 0000_1_2, 0001_1_2 byte-equal to HEAD. HEAD reproduces (HEAD run A and B byte-equal). Starts on DLF23 row: 3.
- DLF4 gate: mac01t mdlsum tot ops 1335 -> 1255. Census fn appendGfx_entity_model (0x0020df40 at HEAD, moved to 0x0020df60 after DLF5) ops 443 -> 363 over 30 draws (14.8 -> 12.1 per draw). Shots byte-equal. Starts on DLF4 row: 1.
- DLF5 gate: mac01t tot ops 1255 -> 1215. Census appendGfx_npc (0x001cb818 -> 0x001cb838) ops 197 -> 162 over 27 draws (7.3 -> 6.0 per draw). Shots byte-equal. Starts on DLF5 row: 1.
- Plan note: the plan gate numbers for DLF4 (17.3 -> 14.3) and DLF5 (11.1 -> 7) used older builds. The measured values above are the current numbers. spr_appendGfx_component is not a top-12 census row; appendGfx_npc is the row that moved.
- DLF6 (TXR1, G_PM_GLYPHS) NOT STARTED. Reason: the plan gate (walk down at least half of glyph + TLUT time, about 2.9 ms) is above the R0 numbers that R1 can remove. R0 Azahar (plan_text_batch 1a): 0b 1.1 ms/frame (removable with the header and load words), e4 4.1 ms/frame (one texrect per glyph; the rect work stays), f0 0.6 ms/frame. The R1 design is about 150 lines (producer in msg_draw.c, record format, handler with atlas cell math, fallback path). Decision needed from the user: run R1 anyway as a measurement, or drop DLF6 on the gate arithmetic.
- DLF epic remaining: DLF6 only. CFI, CFD and CON epics not started.

## Open items
- DLF6 decision (above).
- Scenario catalog: mac01t is the scenario for DLF23, DLF4 and DLF5. Its until line is mdlsum f899 r12.
- The entity RM switch at src/entity_model.c:617 (another function) still runs G_DL for its lists. Not in DLF4 scope.
- RS_PDL and RS_PDLM enum names remain in 3ds/include/pm_port.h (counters with no add sites now).
- The DLF23 patch scripts are in the scratchpad (dlf23_patch.py, dlf23_patch2.py). They are not needed after the commit.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=<row> sh 3ds/build.sh; restore 3ds/src/map_restore_table.c from a copy saved before the generator (the user file is never committed).
- Test: PM_ROW=<row> powershell -File 3ds/tools/azt.ps1 <scenario> -Row <row>. Shots: build3ds/azt/shots/<scenario>/.
- Map sizes: python scratchpad mapsz.py build3ds/pm_3ds.map <names> (sums .text.<name> sizes).
