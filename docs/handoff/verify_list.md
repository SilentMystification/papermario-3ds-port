# verify_list: reported faults with a merged fix and no console confirmation (next push test list; edit in place)
Rule (user): only items the user has not confirmed go on a test list. Confirmed by the user: select menu flash, pipes, map change crash, the white and black rectangles, grey HUD, dim Mario and black menu part (HWFIX1, build 0f39de90cb, O3DS). The 60 FPS row (FPS60MENU) works on the O3DS. Build 0148237d47 (NST2A) on the O3DS: no clear fault seen. Performance is not measured until the optimization pass after the display list work.
Binary check before the push (FACT: every count above 0 in build3ds/pm_3ds.elf of 2026-11-16 02:53, binchk OK):
`python 3ds/tools/binchk.py build3ds/pm_3ds.elf "pauseh f%u held 30 resets %u" nd_dual_keep ndnu_D_0900BE40_3F7CA0 "anim mesh f%u units %d dev %d" nd_mesh_size`
nd_dual_keep and nd_mesh_size are names of static inline functions: they are in the ELF debug data only, not in the 3dsx. Check the ELF.
## a. Pause menu background black instead of the freeze frame
Fix: Merge PAUSEH 6860d1a0fd (the copy of the first pause frame is kept, pm_gpu.c:2946, src/background_gfx.c).
Proof string: `pauseh f%u held 30 resets %u` (FACT: count 1).
User: in the field press START, wait 10 s: the dimmed field picture stays behind the menu and does not go black.
## b. Black coins and field items
Fix: Merge COIN d0973f57aa (nd_comb.c:520 clears dual_on when the combine is not the shaded sprite combine, nd_dual.h:3).
Proof value: `nd_dual_keep` (FACT: count 1).
User: in a map with shaded sprites (example hos_01, Shooting Star Summit) look at coins and items on the ground: they show their colours.
## c. Chapter title card and Munchlesia mesh
Fix 1 (chapter title): Merge CHCONV ae13cabde6 (chapter_change.c joins the converter, 7 glyph draws). Proof: `ndnu_D_0900BE40_3F7CA0` (FACT: count 2).
Fix 2 (Munchlesia): Merge MESH1 1828f683b0 and Merge MESHST 6a94add657. Proof: `anim mesh f%u units %d dev %d` (FACT: count 1, MESH1)
  and `nd_mesh_size` (FACT: count 1, MESHST).
User: at a chapter start, read the chapter title card: every letter and the chapter name draw whole.
User: in Jade Jungle jan_09, walk into the Munchlesia: its mesh draws whole while it grabs, chews and spits Mario.
Row VINE (merge 006a7015ad, no bench reaches kzn_19 or kmr_22): lava piranha vine lists moved to twins. User: in kzn_19 and kmr_22, look at the lava piranha vine: it draws whole.
Row VCFIX (vertex copy entry, model.c mdl_make_local_vertex_copy). User: in Toad Town mac_01 look at Rowf's rug (it ripples); in a Shooting Star
  Summit battle look at the stage stars (they twinkle); in Club 64 (mac_05) look at the sign. Each draws whole, no crash.
Row TATTLEAR (wide tattle view, cam_main.c set_cam_viewport, 16C8E0.c scissor). User: in battle, use Tattle on an enemy in widescreen: the zoomed-in enemy picture fills the tattle window at its true proportions (not stretched 1.25 x).
## d. Bottom screen with Logging off (BOTSCR)
Fix: Merge BOTSCR (pm_log_sink_kick from the HUD, footer and menu close: pm_gpu_ui.c, pm_log.c). Host test botscr.sh.
User: select menu > Debug > Logging page, set Logging Off, close the menu. The bottom screen shows the HUD (fps row updates) and the footer.
## e. Option rows (OPTIONS) and Speed x2
Fix: Merge OPTIONS (pm_options.c, pm_dbg_game.c, pm_speedx2.h). Host test speedx2.sh.
User: on an O3DS the C-Stick dead zone, ZL, ZR and C-Stick map rows are hidden. Stats and Position HUD show grey while HUD is off. Fixed Seed shows On or Off at restart. Speed x2: turn it on, change map: the walk speed stays at 2 x the base and does not grow.
