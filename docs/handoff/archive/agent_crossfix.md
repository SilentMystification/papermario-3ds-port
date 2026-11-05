# Handoff: CROSSFIX epic, agent_crossfix

## State
- Rows run in order: CROSS1, CROSS2, DS3, EN2, NP1, T1B. Done: CROSS1 (dropped), CROSS2 (committed), DS3 (dropped). Not started: EN2, NP1, T1B.
- Working tree: 3ds/src/pm_gpu.c equals HEAD. 3ds/src/map_restore_table.c is the user file (modified, untouched by this epic). 3ds/src/map_restore_table.c.bak equals it.
- Baseline (HEAD e8b37866c, built before any edit): shots in build3ds/azt/shots_headbase/mac01 and build3ds/azt/shots_headbase/mac01t. Log of the mac01t HEAD run: build3ds/azt/... copy at scratchpad base_mac01t_log.txt (not in repo). mac01t log check fails at HEAD and after: mdlsum tot ops 2315, gate 1900 to 2200 (known, lane toadt).

## Commits
- ec9a7dc13 CROSS2: second kick split at 85 % of draws (pm_gpu.c kick_check, kick_at2). Shots byte-equal (mac01 2, mac01t 2). mac01t dr1 splits per frame f600 to f900: 1.0 to 2.0.

## Dropped rows
- CROSS1 (one linear block for pool, ring, index): the premise fails. Pool is VERT_SLOTS 32768 x sizeof(Vert) 40 B = 1.31 MB; ring (GRING_VTX+GSTAT_VTX) x 16 = 512 KB; idx GRING_IDX x 2 = 128 KB. Per frame each uses about 3.5, 4.5 and 4.5 KB at its head. A single flush over the bounding span would cover about 1.8 MB. Not run, no edit kept. Next option: a per-frame dense arena (bump allocation of pool, ring, index), or shrink VERT_SLOTS after a max-use counter. Both change the layout; not in this epic.
- DS3 (generation counters for uvso/cmul/cadd/zmix and g_fx in the pal block): shots byte-equal (mac01t 2, mac01 2). dcsum pal us per close 3 at HEAD, 3 after (mac01t f899, mac01 f299). Gate needs 20 % down (2 us or less). Reverted. The 1 us resolution of dcsum may hide a change below 1 us. A counter in the console build is the next check if this matters.

## Open rows (not started)
- EN2 (plan_entity_npc.md section 3): flat G_PM_TEXI copies of Entity_RenderCircularShadow and Entity_RenderSquareShadow in src/entity/Shadow.c (the G_DL calls to Entity_Shadow_LoadTex* need flattening; check tb_inline/tex_op word set in pm_gbi.c around lines 2741 and 3319 first). Gate mac01t: entity ops per draw 26.5 to 16 or less, closes 0, PPM byte-equal.
- NP1: sprite texture load as G_PM_TEXI in src/sprite.c spr_appendGfx_component_flat (copy msg_draw.c:1758 and 1802). Gate mac01t: npc ops per draw 18.1 to 12 or less, PPM byte-equal.
- T1B: name the tail (src/main_loop.c gfx_draw_frame, src/cam_main.c render_frame) with pm_gbi_mdl_mark under TARGET_3DS and pm_ph_on. Gate mac01t: row 0xF0FE near 0, PPM byte-equal.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_BUILD_CAP=900 sh 3ds/build.sh; then cp 3ds/src/map_restore_table.c.bak 3ds/src/map_restore_table.c.
- Test: PM_ROW=<row> powershell -NoProfile -ExecutionPolicy Bypass -File 3ds/tools/azt.ps1 mac01 -Row <row>; mac01t needs -Try. Copy build3ds/azt/shots/<scenario>/*.ppm and build3ds/last_log.txt after each run (the next run overwrites both).
- Starts used: CROSS1 2 (baseline), CROSS2 2, DS3 2. Budget max 6 per row.
