# Slice queue (open rows only, plan for the next round)

Format: `[ ] id | task | done check`. `[~]` = partly done. Done rows are in docs/SLICES_DONE.md. The full original text of each row below is in docs/HISTORY.md (section "Original text of the open slice rows"): grep for the id.
Rule: build the real decomp file. Never write a stub. If a stub looks like the only option, write `BLOCKED: <reason>` in HANDOFF.md and stop.
Model tier per row: see the MODEL RULE in 3ds/ORCHESTRATOR.md.

[ ] S28 | Hardware run on the O3DS (192.168.1.168) and the N3DS: boot line (real memory limit), audio cost, FPS. Needs the user: O3DS on, dspfirm.cdc on the SD card | numbers in HANDOFF.md, settles Q2 and Q7
[ ] S30b | Link the remaining 154 id-only maps (kpa, kzn, flo, jan, sam, pra, see 3ds/maps_skip.txt). Needs the answer to Q7 (about -1.1 MB at the current spare) | maps_skip.txt area lines removed, presets and tour clean
[~] V07 | Item entities flicker when the camera moves. NOT REPRODUCED in Azahar (see docs/HISTORY.md V07): item draws and pixels are stable in 8 maps with a moving camera, `nocull` A/B identical. Needs the exact scene from the user (QUESTIONS.md) or a hardware run | cause named with evidence, fixed
[~] V06 | Ground disappears when entering a building. Cause found: shroud fade by fog colour alpha, blender mix added. The user must confirm this is the problem (QUESTIONS.md). Follow-up: kmr_20 and kmr_02 doors | the user confirms, or the other case is found and fixed
[ ] V09 | (intro tints) S25 changed the hos_05 intro tints (shots 70 to 77) to dark or red. Check against the decomp and paperboat, A/B with `nocomb`. Also nok_01 `au_syn_begin_audio_frame` unmapped access (tour only) | verdict per shot, nok_01 cause
[ ] V09 | (kmr_22) The "A Plea from the Stars" card shows a garbled rainbow bar above the title (`warp=kmr_22:0 story=-107`). Check the card texture format in src/world/area_kmr/kmr_22 | the card shows its picture
[ ] V10 | Battle Strategies submenu and level-up box draw as a white and brown checkerboard (draw_box pattern path `box_pattern_rect` is the suspect) | Strategies list readable in a shot
[ ] V11 | kmr_05 from entry 0: Mario stops at x 75 to 88, the first Goomba is not reachable. Real geometry or a collision fault | Mario reaches Goomba_01
[ ] X02 | Darkness overlay (kgr_01, kgr_02, omo_12, omo_14, kzn_08) draws the whole screen black. Needs a generic render to image for a non-cfb G_SETCIMG (64x64 target, I8 reinterpretation), about 32 KB VRAM and 3 KB code | `warp=kgr_01:0 partner=1` shows a soft dark circle around Mario
[ ] S31 | Check whether the crash screen can use the debug output (Q5). The shims crash_screen_init, is_debug_init, load_obfuscation_shims stay empty unless this works | verdict written, shims kept or replaced
[~] F01 | Fault sweep remainder: static remainder and the 3 maps with unmapped accesses (kmr_23, nok_01, tst_12); paths: pause, save and load, zones | list of causes, each fixed or recorded
[ ] F03 | `is_point_visible` (src/model.c:4497) reads a NULL z-buffer (log `tex ptr 0000fd7e`, hos_05 lamp). Give the platform layer a depth read back for one pixel | `DepthCopyBuffer` holds real depth, no `tex ptr` line
[ ] T02 | Low priority. Make 3ds/run_azahar.ps1 safe for the user's Azahar config: per-run config copy, try/finally around the body, named mutex, loud failure on a missing key | the real qt-config.ini is never written
[ ] F08 | Fog remainder. V06 added the blend and fog colour mix. G_FOG per-vertex factor and the fog table are not done (about 1.3M draws in the tour use fog) | one fog map renders fog, maps without fog unchanged
[ ] PO2 | Performance: transform each vertex once at G_VTX (CPU only). Needs the user answer (QUESTIONS.md) | phase timer shows less transform time, same pictures
[ ] PO3 | Performance: GPU vertex transform (raw Vtx attributes, index list, matrix in the vertex shader). Plan first (opus allowed for the plan only) | plan in docs/, then a working A/B
[ ] AU1 | Performance: audio on core 1 or the DSP (ndsp), with libctru natives. Plan first | audio cost per tick measured on the O3DS
[ ] MA1 | Performance: ARM11 math audit (slow divides, sqrt, libm calls) with the replay check (hash of game state per N frames). Game-logic math changes need the replay check and the user approval | same hashes, lower cost
[ ] F06 | LATER, big. 30/60 fps toggle by interpolated extra frames (logic stays at 30 Hz). Plan only after the hardware run | plan in docs/
[ ] F07 | LATER. Widescreen mode (research in docs/HISTORY.md, section "Widescreen and 60 fps research"). Plan after the hardware run | plan in docs/
[~] S18 | Close-out: pause is real and the demo and language select were done in S18b and S17. Check and move to docs/SLICES_DONE.md | one check run, then moved
[ ] S19c | Close-out: black and olive polygons in attacks. S25 may have fixed them (24 shots clean in M03). Check one attack-heavy battle | no polygons, then moved to docs/SLICES_DONE.md
