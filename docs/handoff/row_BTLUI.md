# Row BTLUI: battle damage star missing; action command button icon (oracle plan 2026-11-08, main d3b9968c5)

## User facts (bugs, console O3DS XL)
1. In battle, when someone does damage, the star behind the number no longer appears; only the number shows.
2. Action command prompts show the Japanese button icon instead of the English one.

## Bug 1: cause (oracle trace, confirmed by code)
damage_indicator (src/effects/damage_indicator.c:195-290) loads the star corner vertices in lists with only gsSPVertex (D_09002150 slot 0, D_09002160 slots 1-2, D_09002170 slot 3, D_09002180 slots 0-3), each under its own matrix, and draws the triangles in separate lists with only gsSP2Triangles (D_090021A8, D_09002190). On the port every static G_DL goes to ent_gdl (nd_walk.c G_DL case; nd_ent.c:310). A vertex-only list compiles to one segment with 0 triangles (nd_ent_segments, nd_dlc.c:300-316) and is stored ENT_OK; ent_run skips its G_VTX (nd_ent.c:376 `if (!e->ntri) continue;`) and clears all walker slots (nd_ent.c:383 memset(vert_live, 0, ...)). The triangle-only list is refused (NDR_SLOT, nd_dlc.c:321), runs on the walker, finds vert_live 0 and drops the triangles. Older than today (LS-2 6b281c06e / SGEN-1).
FIX: in nd_ent.c ent_compile, right after `for (unsigned s = 0; s < ns; s++) ntri += ent_sbuf[s].ntri;`, add:
```c
    /* BTLUI: a list whose vertex loads feed a later list (no triangle after its last G_VTX, e.g. the damage star corners of
     * damage_indicator D_09002150..80) stays on the walker: ent_run skips loads without triangles and clears the walker slots. */
    { unsigned lastv = 0, lastt = 0;
      for (unsigned i = 0; i < nops; i++) { unsigned op = ent_flat_buf[i].words.w0 >> 24;
          if (op == G_VTX) lastv = i + 1; else if (op == G_TRI1 || op == G_TRI2) lastt = i + 1; }
      if (ntri == 0 || lastv > lastt) { k->why = 8; return; } }
```
(why 8 = existing "none" refusal: the key stays ENT_REF and the walker runs the list, nd_ent.c:335.) Check the names (ent_flat_buf, nops, ntri, k) against the code before the edit.
Scenario dmg_star (azt_scenarios.json + bench_defs.json, same commit): `warp=kmr_20:0 battle=BTL_KMR_1_FORMATION_00:BTL_KMR_1_STAGE_01 battlef=30 autoa det fixedseed shots`, shots around the frame after Mario's first jump hits (find it from one run's `battle:` lines plus a shot burst); until-line `battle: start` with a 25 s cap. Pass: the shot shows the coloured star burst behind the "1".

## Bug 2: diagnostic only (no fix, no commit of the log line)
Ruled out: the icon is HES_AButton/HES_PressAButton/HES_MashAButton (src/global_hud_scripts.c:2313-2335, used in src/battle/action_cmd/*.c e.g. jump.c:34), hs_SetCI with ui_input_a_button_unpressed/pressed compiled from assets/us/ui/input/a_button_*.png (US); no region #ifdef; no ROM read; not a message glyph.
Steps: (1) in the dmg_star run take a shot when the jump prompt shows (about 20 frames after the jump starts); compare with assets/us/ui/input/a_button_unpressed.png. If Azahar shows the correct blue "A", the fault is console-only: report and stop. (2) Else add a one-time pm_log in hud_element.c (inside an existing #ifdef TARGET_3DS block near line 472) for an element whose imageAddr == ui_input_a_button_unpressed_png: imageAddr, paletteAddr, tile size, pm_tex_cur_idx() after the load; and with switch `texword` (pm_tex.c:803) the slot's img, tlut, w, h, fmt. (3) Report the shot and the two log lines.

## Start state
Worktree build3ds/wt/btlui, branch btlui from main. Copy assets/us and ver/us/build into it.

## Done criteria
1. Build; nd_host PASS.
2. dmg_star shot shows the star.
3. spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det 0000-0015 byte-equal; if a shot changes, open it, name the effect that now appears, and put it in the commit message.
4. Bug 2 report (shot + log lines).
Affected scenarios (no others): dmg_star, the 4 byte gates.

## Rules
CLAUDE.md rules 5, 7, 9, 11. PM_LANE=btlui PM_ROW=btlui on every build.sh and bench call. No git stash. No toggles, no stubs.

## Commands
- Build: python 3ds/tools/gen_map_restore.py; PM_ROW=btlui PM_LANE=btlui sh 3ds/build.sh; restore 3ds/src/map_restore_table.c.
- Test: PM_ROW=btlui PM_LANE=btlui python 3ds/tools/bench.py run <bench> --row btlui --lane btlui --why "<text>"

## At the end
Delete this row file and .wt_keep in the last commit. SendMessage main "btlui ready <hash>" with the bug 2 report.
