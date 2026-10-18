# H05 Part C findings (static audit, 2026-10-18)

Method: code reading and grep of 3ds/src, src and docs. No build. No Azahar run. Old logs in 3ds/logs only as hints.
Format: id | area | evidence | proposed fix row. Rows named NEW are not yet in 3ds/SLICES.md.
Needs row 14: every check of the final picture (C2 full counts, lava, fog look, light look). Those steps are not done here.

## C1 GBI, RDP and RSP features against the port

- C1-01 | FIXED by L01 (pm_gbi.c light table, nolight switch) | G_LIGHTING | pm_gbi.c:425 vert_color uses one fixed light (0.25, 0.85, 0.45). G_MOVEMEM light index 0x0a, G_MW_NUMLIGHT 0x02 and G_MW_LIGHTCOL are ignored (pm_gbi.c:1445 and 1853). 18 files in src set lights (entity_model.c, hud_element.c, imgfx.c, item_entity.c, level_up*.c, battle/move/item/*.c) | NEW L01: read lights from G_MOVEMEM into a light table, do the RSP dot product per vertex, apply ambient. Done before S28.
- C1-02 | Fog | G_MW_FOG ignored, vertex alpha not replaced by the fog factor. Only the blender A=A_FOG mix exists (V06). 24 files, about 1.3M draws per tour (F03_F08.md) | F08 (open).
- C1-03 | G_TEXTURE_GEN | geom bit counted only (st_tgen), no vertex code. One user: world/area_mac/mac_01/crystal_ball_gfx.c:26 | NEW T01: texgen in xform_vtx, or merge into B3 (mac_01 case 6). Done before S28.
- C1-04 | G_TEXTURE scale | pm_gbi.c:1474 reads only the on bit. The s and t scale is ignored. Non-0xFFFF scale with G_ON: screen_overlays.c:141 (0x8000) | NEW T02: low risk. Check that the overlay uses a texrect. Else apply the scale to the vertex UV.
- C1-05 | LOD, mipmap | no G_LOD set and no G_TL_LOD found in src | accepted (proved by grep).
- C1-06 | G_BRANCH_Z, G_CULLDL, G_LOAD_UCODE, G_BGRECT | no user in src (grep gSPBranchLessZ, gSPCullDisplayList, gSPBgRect: 0 hits). The default case counts them | accepted (proved by grep).
- C1-07 | G_MODIFYVTX | 6 macro uses, one file (effects/underwater.c). The opcode falls to note() (skipped) | B5 (open).
- C1-08 | Chroma key G_SETKEYR, G_SETKEYGB, G_SETCONVERT | 3 and 16 macro lines. flame.c and ring_blast.c. Not handled | B6 (open). Add ring_blast to B6.
- C1-09 | Render to image cases 5, 6, 7, 8, 9, 13 of render_gaps.md | case 5 verify, case 6 flo_10 and mac_01, case 7 underwater, case 8 flame, case 9 soft reset, case 13 current-frame grab (8 effects) | B1, B2, B3, B5, B6 (open). Case 5 and case 9 have no row: NEW R05 (verify with xtrace, one frame). Case 13 sub-cases beyond flo_10 (water_splash, motion_blur_flame, bulb_glow, effect_utils, kgr_02/unused) have no row: NEW R13.
- C1-10 | Case 3 darkness stencil (X02) | render_gaps.md lists kgr_01, kgr_02, omo_12, omo_14, kzn_08, jan_11, kpa_01, kpa_03, battle toy_tank, big_lantern_ghost, Watt light. The fix is uncommitted in the working tree (pm_gbi.c, pm_gpu.c) | Commit X02 after a check. Needs row 14 to judge the picture.
- C1-11 | Case 4 depth copy (F03) | model.c is_point_visible reads a NULL z-buffer. pm_depth.c, pm_depth.h and test_zenc.c are uncommitted | F03 (open).
- C1-12 | G_LOADBLOCK | ignored for data. V09 notes a TMEM replace. 17 uses. Works in golden frames | accepted.
- C1-13 | Alpha and colour dither, noise | ignored, no PICA equivalent | accepted (cosmetic).
- C1-14 | Texture tile animation (scroll, pan, tile size) and CI palettes | G_SETTILESIZE negative start handled (S25, pm_gbi.c:1720). G_LOADTLUT handled (1704). The texture look needs a picture check | needs row 14. Include the V13 lava maps (src/world/area_kzn).
- C1-15 | Frame buffer as texture source (case 13) and gDPLoadMultiTile from a frame buffer | dual_bind breaks for a frame buffer source (render_gaps.md section C) | covered by NEW R13.

## C2 Unhandled-op counters on all content

- C2-01 | Counters exist | pm_gbi.c:1036 note() logs `gbi skip <op>`. Lines `gbi ignore movemem` and `gbi ignore moveword` exist at 1445 and 1853. Old logs show op f3 (LOADBLOCK, now handled), movemem 0a, moveword 02, 04, 0e | The data is old. A run is needed.
- C2-02 | 421 maps, every effect, every battle stage, actor type, menu | needs row 14 and a new tour build. The counters are ready. Proposed NEW C2R: one `tour=` run for all 421 maps, one for the battle stages, one for effects. List the `gbi skip` and `gbi ignore` lines by type and count in this file.
- C2-03 | Moveword 04 (G_MW_CLIP), 0e (G_MW_PERSPNORM) | ignored. These change no picture here | accepted (confirm in C2R).

## C3 Stand-ins, shims and approximations in code

Search words: stub, shim, approximate, TODO, not implemented, unsupported, FIXME, hack in 3ds/src and 3ds/include. The result is short.
- C3-01 | 3ds/src/pm_actor_stubs.c | defines the macro STUB with no use. The file has no function | NEW C3A: remove the file from the build if it is empty. No game logic.
- C3-02 | 3ds/src/pm_kmr_stubs.c | holds only the buffers gBackgroundImage_buf and pm_world_entity_heap. Old comments say "not ported" for the partner system | NEW C3B: check by nm that no partner symbol is undefined, then rename the file. No game logic.
- C3-03 | 3ds/src/pm_later.c | comment lines and an empty `crash_screen_set_draw_info()` | accepted.
- C3-04 | 3ds/src/game_stubs.c | `crash_screen_init`, `is_debug_init`, `load_obfuscation_shims` are empty. The `shim_*_obfuscated` functions call the real functions | accepted. The N64 obfuscation holds no game logic.
- C3-05 | 3ds/src/pm_os.c | osMapTLB, osUnmapTLB, osViSetMode, osViSetSpecialFeatures, osInvalICache, osInvalDCache, osWritebackDCache and osSetIntMask are empty or return the input. osAiGetStatus returns 0 | accepted (N64 hardware only). Cache rules belong to H05 Part B.
- C3-06 | 3ds/src/pm_nusys.c | nuSiSendMesg returns 0 (no rumble), nuGfxPreNMIFuncSet and nuGfxTaskAllEndWait are empty, nuContInit returns 1 | accepted. The 3DS has no Rumble Pak. Ask the user about rumble in 3ds/QUESTIONS.md.
- C3-07 | pm_rsp_audio.c:548 | `TODO: sse/neon`. Speed only | AU1 (after S28).
- C3-08 | Decomp ifdef TARGET_3DS paths (74 sites in src and include) | Read all groups: byte swap of ROM data (audio, imgfx, msg, model textures, background, demo_api, hos_05 intro), pointer tests (encounter.c:2535, pause_gfx.c:629, msg.c:2035), animated entity node fix (entity.c:874 and 1064), model.c:4079 u64 read. No game rule is removed | accepted, with one risk. encounter.c:2535 tests `init >= 0x00100000`. A map with a real init value under that number would lose its init script. Evidence: machi and kzn_04 hold 0x1F64 which is not a pointer. NEW C3C: scan the map NPC tables of all 421 maps for init values between 0x1000 and 0x100000. RESULT C3C FIXED (accepted, no fault): 5 map files hold a literal non-pointer init: kzn_04, kzn_11, machi (0x1F64, 13 NPCs) and trd_08, kzn_11 (0x00004003, 45 NPCs). Both builds drop them (N64: s32 init >= EVT_LIMIT gives nullptr, 3DS: u32 init < 0x00100000 gives nullptr). All other init fields are `&N(...)` pointers. No init script is lost.
- C3-09 | model.c:4079 `// TODO required to match` | original decomp comment, not a port stand-in | accepted.
- C3-10 | ROM remap (rom_remap.h, packed ROM image) | ROM reads go through a remap table. A wrong entry gives wrong data, not missing logic | covered by the R01 rom_suspect log. Needs row 14 (full run) to close.

## C4 Game logic and maths against the decomp

- C4-01 | RNG | src/43F0.c is the decomp LCG. The seed is `gRandSeed += osGetCount()` (main.c:100). osGetCount returns the 3DS system tick (268 MHz, pm_os.c:41), not the 46.875 MHz N64 count | accepted. Only the seed differs.
- C4-02 | Timers | src/os/settimer.c, timerintr.c and gettime.c are in the tree. pm_os.c defines no osSetTimer or osGetTime. grep shows osGetTime users only in src/os and crash_screen.c | NEW C4A: confirm by nm which os/*.c files the link takes. No game code uses osGetTime. RESULT C4A FIXED: build3ds/pm_3ds.map links from src/os only coss, frustum, guMtxXFML, guOrtho, guRotate, lookathil, lookatref, perspective, position, sins, sqrtf and nusys/nucontrmbcheck, nucontrmbforcestop, nucontrmbforcestopend, nucontrmbmodeset, nucontrmbstart, nupireadromoverlay. settimer, timerintr and gettime are not linked.
- C4-03 | Math wrappers | 3ds/src/pm_gu.c replaces guMtxIdent, guMtxCatF, guMtxF2L, guMtxL2F, guMtxXFMF, guScale, guRotateRPYF, guTranslate and guNormalize. The decomp src/os has guMtxXFML.c, guRotate.c, sins.c and sqrtf.c in the link list | NEW C4B: compare guMtxF2L rounding (fixed point split) and the guRotateRPYF order with the libultra source in reference/. MA1 (after S28) covers speed, not this. RESULT C4B: guMtxF2L matches src/os/guMtxF2L.s and the sm64 C version (x65536, truncate, integer halves in words 0..7, fractions in words 8..15). guRotateRPYF has the same term order as src/os/rotateRPY.c. One difference: pm_gu.c uses libm sinf and cosf, the decomp uses sin_rad and cos_rad (sins.c table, 1/4096 turn steps, error up to about 8e-4). OPEN, new row C4F (graphics).
- C4-04 | Audio | The RSP mixer is a C port in pm_rsp_audio.c (aADPCMdec, aResample, aEnvMixer, aMix, aInterleave, aPoleFilter, aSetLoop, aLoadBuffer, aSaveBuffer, aClearBuffer, aDMEMMove, aSetVolume). The engine in src/audio is the decomp. Byte swap patches are in bgm_player.c:1383 and engine.c:964 and 998 | NEW C4C: list the RSP commands that the engine emits (grep the a* macros in src/audio) and check that each has an Impl. A PCM hash check against N64 output needs row 14. RESULT C4C FIXED: the engine emits aClearBuffer, aDMEMMove, aLoadADPCM, aMix, aSetLoop, n_aADPCMdec, n_aEnvMixer, n_aInterleave, n_aLoadBuffer, n_aPoleFilter, n_aResample, n_aSaveBuffer and n_aSetVolume. 3ds/include/pm_rsp_audio.h and 3ds/stub_include/PR/n_abi.h map each one to an Impl in pm_rsp_audio.c. aDisableImpl is empty, no engine call emits it (n_aNoop has no user).
- C4-05 | Save | fio.c is the decomp. The flash chip is an array backed by save.flash (pm_os.c:399). osFlashReadArray, osFlashWriteBuffer, osFlashWriteArray and osFlashSectorErase are replaced | NEW C4D: read the four functions against src/os/flash*.c for page size, erase size and status words. The save and load round trip needs row 14. RESULT C4D FIXED: page 128 bytes (FLASH_BLOCK_SIZE), chip 0x20000 bytes (FLASH_SIZE), sector 128 pages = 16 KB, status words OK 0 and ERROR -1 (FLASH_STATUS_*). fio.c erases before each write, so write-as-copy equals the chip AND-write. Port osFlashReadArray and osFlashWriteBuffer return void, fio.c ignores the value.
- C4-06 | Controller | nuContDataGet and nuContDataGetEx are replaced (pm_nusys.c:33) | NEW C4E: compare the stick scale and the C button map with the N64 range. The user reviews the button map.
- C4-07 | Message queues and threads | osCreateMesgQueue, osSendMesg, osRecvMesg and osCreateThread are replaced in pm_os.c. One audio thread exists. Other N64 threads run in the main loop | H05 Part B (threads). No row here.
- C4-08 | Frame pacing | pm_frame_loop replaces the nusys scheduler | PC01 (done). Hardware speed needs row 14.

## Summary

- Covered by an open row: C1-02 (F08), C1-07 (B5), C1-08 (B6, add ring_blast), C1-09 (B1, B2, B3), C1-10 (X02), C1-11 (F03), C3-07 (AU1).
- New rows to add: L01, T01, T02, R05, R13, C2R, C3A, C3B, C3C, C4A, C4B, C4C, C4D, C4E.
- Needs row 14: C1-10 look, C1-14 look (lava, scroll), C2-02 counts, C3-10 full run, C4-04 hash, C4-05 round trip.
- Highest risk: C1-01 (fixed light on every lit model), C1-02 (fog on 24 files), C1-09 case 13 (8 effects read the frame).
