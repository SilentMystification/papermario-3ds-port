# FX1 effect table

Made by `python 3ds/tools/fxtest_report.py run all` (game switch `fxtest=all` with `shots`, preset toad, player position, 44 frame slot per effect).
Rules: drew = 15 or more dense cells (8 x 8 cell with 12 or more changed pixels) in shot A or B against the clean shot of the slot. Idle animation alone gives 4 to 12 cells, so weak = at most 14 cells: tiny, late or not drawn, check the sheet. black = at least 300 changed pixels turned near black and at least 40 percent of the changed pixels.
solid colour = at least 800 changed pixels and at least half of them in one colour (4 bit per channel), SOLID marks a flat block. zoomed or clipped = dense cells cover over 60 percent of the frame by bounding box, or touch 3 or more frame edges.
The heuristics flag candidates. The user looks at the contact sheets (3ds/logs/fx/sheet_N.png, not committed).

| # | effect | state | drew | changed px | dense cells | black | black px | solid colour % | zoomed or clipped | dense bbox % | edges |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | big_smoke_puff | ok | yes | 2059 | 66 | no | 0 | 5 | no | 40 | 0 |
| 1 | landing_dust | ok | yes | 1477 | 40 | no | 0 | 6 | no | 42 | 0 |
| 2 | walking_dust | ok | weak | 166 | 6 | no | 0 | 0 | no | 1 | 0 |
| 3 | flower_splash | ok | weak | 315 | 12 | no | 0 | 0 | no | 15 | 0 |
| 4 | flower_trail | ok | weak | 252 | 12 | no | 0 | 0 | no | 18 | 0 |
| 5 | cloud_puff | ok | yes | 1487 | 32 | no | 0 | 43 | no | 40 | 0 |
| 6 | cloud_trail | ok | weak | 304 | 10 | no | 0 | 0 | no | 15 | 0 |
| 7 | footprint | ok | weak | 280 | 11 | no | 0 | 0 | no | 42 | 0 |
| 8 | floating_flower | ok | yes | 447 | 17 | no | 0 | 0 | no | 42 | 0 |
| 9 | snowflake | ok | weak | 275 | 10 | no | 0 | 0 | no | 40 | 0 |
| 10 | star | ok | yes | 415 | 17 | no | 0 | 0 | no | 42 | 0 |
| 11 | emote | ok | weak | 146 | 6 | no | 0 | 0 | no | 1 | 0 |
| 12 | sparkles | ok | yes | 719 | 19 | no | 0 | 0 | no | 16 | 0 |
| 13 | shape_spell | ok | weak | 135 | 6 | no | 0 | 0 | no | 1 | 0 |
| 14 | gather_energy_pink | ok | weak | 232 | 9 | no | 0 | 0 | no | 42 | 0 |
| 15 | drop_leaves | ok | weak | 338 | 10 | no | 0 | 0 | no | 28 | 0 |
| 16 | dust | ok | yes | 572 | 18 | no | 0 | 0 | no | 42 | 0 |
| 17 | shattering_stones | ok | yes | 385 | 15 | no | 0 | 0 | no | 42 | 0 |
| 18 | smoke_ring | ok | yes | 631 | 23 | no | 0 | 0 | no | 40 | 0 |
| 19 | damage_stars | ok | yes | 834 | 26 | no | 0 | 19 | no | 42 | 0 |
| 20 | explosion | ok | yes | 5780 | 107 | no | 0 | 11 | no | 32 | 0 |
| 21 | lens_flare | ok | yes | 11654 | 224 | no | 0 | 6 | no | 42 | 0 |
| 22 | got_item_outline | ok | yes | 3381 | 74 | no | 108 | 18 | no | 23 | 0 |
| 23 | spiky_white_aura | ok | yes | 3023 | 71 | no | 0 | 7 | no | 40 | 0 |
| 24 | smoke_impact | ok | yes | 1003 | 24 | no | 0 | 9 | no | 16 | 0 |
| 25 | damage_indicator | ok | yes | 3333 | 75 | no | 0 | SOLID 65 | no | 42 | 0 |
| 26 | purple_ring | ok | yes | 1280 | 41 | no | 0 | 7 | no | 42 | 0 |
| 27 | flame | ok | yes | 2534 | 51 | no | 0 | SOLID 95 | no | 40 | 0 |
| 28 | stars_burst | ok | yes | 360 | 15 | no | 0 | 0 | no | 42 | 0 |
| 29 | stars_shimmer | ok | yes | 718 | 20 | no | 0 | 0 | no | 16 | 0 |
| 30 | rising_bubble | ok | weak | 179 | 8 | no | 0 | 0 | no | 15 | 0 |
| 31 | ring_blast | ok | weak | 419 | 14 | no | 0 | 0 | no | 16 | 0 |
| 32 | shockwave | ok | yes | 6757 | 129 | no | 0 | 13 | no | 42 | 0 |
| 33 | music_note | ok | weak | 373 | 12 | no | 42 | 0 | no | 22 | 0 |
| 34 | smoke_burst | ok | weak | 273 | 11 | no | 0 | 0 | no | 42 | 0 |
| 35 | sweat | ok | weak | 295 | 11 | no | 0 | 0 | no | 42 | 0 |
| 36 | sleep_bubble | ok | weak | 245 | 9 | no | 0 | 0 | no | 40 | 0 |
| 37 | windy_leaves | ok | yes | 486 | 17 | no | 0 | 0 | no | 42 | 0 |
| 38 | falling_leaves | ok | weak | 394 | 12 | no | 0 | 0 | no | 16 | 0 |
| 39 | stars_spread | ok | yes | 955 | 27 | no | 0 | 35 | no | 22 | 0 |
| 40 | steam_burst | ok | weak | 241 | 10 | no | 0 | 0 | no | 15 | 0 |
| 41 | stars_orbiting | ok | yes | 611 | 18 | no | 0 | 0 | no | 40 | 0 |
| 42 | big_snowflakes | ok | yes | 1260 | 28 | no | 0 | 32 | no | 17 | 0 |
| 43 | debuff | ok | weak | 279 | 11 | no | 0 | 0 | no | 42 | 0 |
| 44 | green_impact | ok | yes | 3649 | 75 | no | 0 | 3 | no | 42 | 0 |
| 45 | radial_shimmer | ok | yes | 1251 | 38 | no | 0 | 17 | no | 40 | 0 |
| 46 | ending_decals | ok | weak | 281 | 11 | no | 0 | 0 | no | 42 | 0 |
| 47 | light_rays | ok | yes | 3327 | 91 | no | 0 | 7 | no | 55 | 0 |
| 48 | lightning | ok | yes | 6498 | 137 | no | 0 | 3 | no | 53 | 0 |
| 49 | fire_breath | ok | weak | 145 | 6 | no | 0 | 0 | no | 1 | 0 |
| 50 | shimmer_burst | ok | yes | 1119 | 33 | no | 0 | 18 | no | 43 | 0 |
| 51 | energy_shockwave | ok | yes | 13002 | 306 | no | 0 | 14 | YES | 63 | 0 |
| 52 | shimmer_wave | ok | weak | 344 | 14 | no | 0 | 0 | no | 45 | 0 |
| 53 | aura | ok | weak | 278 | 11 | no | 0 | 0 | no | 42 | 0 |
| 54 | bulb_glow | ok | yes | 9568 | 173 | no | 0 | SOLID 63 | no | 40 | 0 |
| 55 | effect_3D | ok | yes | 3583 | 75 | no | 0 | 33 | no | 42 | 0 |
| 56 | blast | ok | yes | 2917 | 71 | no | 0 | 17 | no | 27 | 0 |
| 57 | fire_flower | ok | yes | 1927 | 49 | no | 0 | 21 | no | 24 | 0 |
| 58 | recover | ok | weak | 307 | 11 | no | 0 | 0 | no | 14 | 0 |
| 59 | disable_x | ok | weak | 200 | 7 | no | 0 | 0 | no | 40 | 0 |
| 60 | bombette_breaking | ok | weak | 106 | 4 | no | 0 | 0 | no | 1 | 0 |
| 61 | firework | ok | weak | 465 | 12 | no | 0 | 0 | no | 42 | 0 |
| 62 | confetti | ok | yes | 1238 | 50 | no | 0 | 8 | no | 42 | 0 |
| 63 | snowfall | ok | yes | 1338 | 37 | no | 0 | 23 | no | 60 | 0 |
| 64 | effect_46 | ok | yes | 602 | 22 | no | 0 | 0 | no | 42 | 0 |
| 65 | gather_magic | ok | yes | 923 | 25 | no | 0 | 11 | no | 31 | 0 |
| 66 | attack_result_text | ok | yes | 1617 | 35 | YES | 721 | 47 | no | 12 | 0 |
| 67 | small_gold_sparkle | ok | weak | 222 | 10 | no | 0 | 0 | no | 15 | 0 |
| 68 | flashing_box_shockwave | ok | weak | 222 | 9 | no | 0 | 0 | no | 42 | 0 |
| 69 | balloon | ok | weak | 318 | 10 | no | 0 | 0 | no | 8 | 0 |
| 70 | floating_rock | ok | yes | 777 | 27 | no | 0 | 0 | no | 42 | 0 |
| 71 | chomp_drop | ok | yes | 3125 | 78 | no | 0 | 21 | no | 42 | 0 |
| 72 | quizmo_stage | ok | yes | 8503 | 173 | no | 398 | 13 | YES | 63 | 0 |
| 73 | radiating_energy_orb | ok | yes | 832 | 31 | no | 0 | 9 | no | 42 | 0 |
| 74 | quizmo_answer | ok | weak | 148 | 6 | no | 0 | 0 | no | 1 | 0 |
| 75 | motion_blur_flame | ok (crashes only after earlier effects in one run) | yes | 483 | 15 | no | 190 | 0 | no | 40 | 0 |
| 76 | energy_orb_wave | ok | yes | 7939 | 174 | no | 0 | 6 | no | 60 | 0 |
| 77 | merlin_house_stars | ok | yes | 5152 | 149 | no | 0 | 30 | no | 60 | 0 |
| 78 | quizmo_audience | ok | yes | 10101 | 199 | no | 778 | 38 | no | 53 | 0 |
| 79 | butterflies | ok | weak | 137 | 5 | no | 0 | 0 | no | 12 | 0 |
| 80 | stat_change | ok | yes | 1634 | 37 | no | 232 | 26 | no | 16 | 0 |
| 81 | snaking_static | ok | weak | 401 | 14 | no | 0 | 0 | no | 40 | 0 |
| 82 | thunderbolt_ring | ok | yes | 693 | 22 | no | 0 | 0 | no | 23 | 0 |
| 83 | squirt | ok | weak | 359 | 14 | no | 0 | 0 | no | 42 | 0 |
| 84 | water_block | ok | yes | 4050 | 75 | no | 0 | 14 | no | 47 | 0 |
| 85 | waterfall | ok | yes | 1381 | 38 | no | 0 | 7 | no | 40 | 0 |
| 86 | water_fountain | ok | yes | 6543 | 132 | no | 0 | 8 | no | 42 | 0 |
| 87 | underwater | ok | yes | 50680 | 859 | no | 0 | 7 | YES | 63 | 0 |
| 88 | lightning_bolt | ok | weak | 220 | 8 | no | 0 | 0 | no | 3 | 0 |
| 89 | water_splash | ok | yes | 522 | 18 | no | 0 | 0 | no | 16 | 0 |
| 90 | snowman_doll | ok | yes | 13866 | 248 | no | 324 | 25 | no | 42 | 0 |
| 91 | fright_jar | ok | yes | 7557 | 151 | no | 0 | 12 | no | 25 | 0 |
| 92 | stop_watch | ok | yes | 5222 | 111 | no | 0 | 4 | no | 53 | 0 |
| 93 | effect_63 | ok | yes | 2026 | 43 | no | 389 | 20 | no | 42 | 0 |
| 94 | throw_spiny | ok | yes | 1100 | 26 | no | 0 | 30 | no | 40 | 0 |
| 95 | effect_65 | ok | weak | 269 | 11 | no | 0 | 0 | no | 42 | 0 |
| 96 | tubba_heart_attack | ok | yes | 1065 | 32 | no | 0 | 20 | no | 33 | 0 |
| 97 | whirlwind | ok | yes | 1550 | 35 | no | 0 | 9 | no | 16 | 0 |
| 98 | red_impact | ok | yes | 28694 | 496 | no | 140 | 11 | no | 43 | 0 |
| 99 | floating_cloud_puff | ok | weak | 365 | 12 | no | 0 | 0 | no | 40 | 0 |
| 100 | energy_in_out | ok | weak | 153 | 4 | no | 0 | 0 | no | 1 | 0 |
| 101 | tattle_window | ok | yes | 16294 | 277 | no | 0 | SOLID 89 | no | 48 | 0 |
| 102 | shiny_flare | ok | weak | 281 | 11 | no | 0 | 0 | no | 42 | 0 |
| 103 | huff_puff_breath | ok | yes | 12098 | 215 | no | 0 | 15 | no | 52 | 0 |
| 104 | cold_breath | ok | yes | 3775 | 74 | no | 0 | 13 | no | 42 | 0 |
| 105 | embers | ok | weak | 145 | 6 | no | 0 | 0 | no | 1 | 0 |
| 106 | hieroglyphs | ok | yes | 1631 | 45 | no | 0 | 8 | no | 22 | 0 |
| 107 | misc_particles | ok | weak | 160 | 7 | no | 1 | 0 | no | 14 | 0 |
| 108 | static_status | ok | weak | 339 | 11 | no | 0 | 0 | no | 42 | 0 |
| 109 | moving_cloud | ok | weak | 106 | 4 | no | 0 | 0 | no | 1 | 0 |
| 110 | effect_75 | ok | weak | 271 | 11 | no | 0 | 0 | no | 42 | 0 |
| 111 | firework_rocket | ok | weak | 281 | 11 | no | 0 | 0 | no | 42 | 0 |
| 112 | peach_star_beam | ok | yes | 11622 | 219 | no | 1442 | 24 | no | 47 | 0 |
| 113 | chapter_change | ok | yes | 5404 | 115 | no | 0 | 3 | no | 52 | 0 |
| 114 | ice_shard | ok | weak | 313 | 10 | no | 0 | 0 | no | 6 | 0 |
| 115 | spirit_card | ok | yes | 1754 | 42 | no | 90 | SOLID 59 | no | 20 | 0 |
| 116 | lil_oink | ok | weak | 135 | 6 | no | 0 | 0 | no | 1 | 0 |
| 117 | something_rotating | ok | yes | 2918 | 71 | no | 0 | 3 | no | 40 | 0 |
| 118 | breaking_junk | ok | weak | 228 | 7 | no | 15 | 0 | no | 17 | 0 |
| 119 | partner_buff | ok | weak | 279 | 11 | no | 0 | 0 | no | 42 | 0 |
| 120 | quizmo_assistant | ok | yes | 995 | 27 | no | 0 | 16 | no | 45 | 0 |
| 121 | ice_pillar | ok | yes | 1270 | 31 | no | 1 | 16 | no | 40 | 0 |
| 122 | sun | ok | weak | 281 | 11 | no | 0 | 0 | no | 42 | 0 |
| 123 | star_spirits_energy | ok | weak | 202 | 8 | no | 0 | 0 | no | 14 | 0 |
| 124 | pink_sparkles | ok | yes | 447 | 16 | no | 0 | 0 | no | 15 | 0 |
| 125 | star_outline | ok | yes | 5454 | 128 | no | 107 | 3 | no | 41 | 0 |
| 126 | effect_86 | ok | weak | 222 | 9 | no | 0 | 0 | no | 42 | 0 |

Spawned and shot: 127 of 127. Weak or nothing: 52. Black block: 1. Zoomed or clipped: 3. Solid colour block: 5.

## Notes (FX1a, 2026-10-18)
- Run: preset toad, player position, 44 frame slot, shots A and B 10 and 30 frames after the spawn. 127 of 127 effects spawned. 3dsx size: 13072248 to 13081216 bytes (+8968).
- Spawned alone (nothing else needed): all 127. Effects that need game context and show only a part of the real picture: emote (needs an Npc, the first NPC of the map is used), effect_46 (needs a PlayerStatus, the real one is used), damage_indicator and tattle_window (UI parts of a battle or menu), quizmo_stage, quizmo_audience, quizmo_assistant and quizmo_answer (the quiz scene sets them up), sun (the real call passes only a direction), chapter_change (full screen title).
- Crash: in one 127 effect run the game stops with svcBreak in C3D_RenderTargetDelete from pm_gpu_off_begin (an off-screen slot changes size inside a frame). It hit bulb_glow (54) in the first sweep and motion_blur_flame (75) in the last. Each of them runs fine alone. New row V05b in 3ds/SLICES.md.
- Look wrong in the sheets (user decides): flame 27 (flat red block), bulb_glow 54 (white square, V12c fixed type 0 and this run uses type 2), motion_blur_flame 75 (black blob, row R13b), effect_63 93 (black blob), peach_star_beam 112 (black block behind the stars), huff_puff_breath 103 (yellow blocks), quizmo_audience 78 (black shapes, may be the real dark look). Clipped: energy_shockwave 51. Full screen tint: underwater 87 (probably right).
- Not visible or tiny (weak): see the weak rows. The idle animation of Mario gives up to 12 dense cells, so weak rows need the sheet.
- The heuristics do not see a wrong colour or a wrong shape. Only the sheets show that.
