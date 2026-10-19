# FX1 effect table

Made by `python 3ds/tools/fxtest_report.py run all` (game switch `fxtest=all` with `shots`, preset toad, player position, 44 frame slot per effect).
Rules: drew = 15 or more dense cells (8 x 8 cell with 12 or more changed pixels) in shot A or B against the clean shot of the slot. Idle animation alone gives 4 to 12 cells, so weak = at most 14 cells: tiny, late or not drawn, check the sheet. black = at least 300 changed pixels turned near black and at least 40 percent of the changed pixels.
solid colour = at least 800 changed pixels and at least half of them in one colour (4 bit per channel), SOLID marks a flat block. zoomed or clipped = dense cells cover over 60 percent of the frame by bounding box, or touch 3 or more frame edges.
The heuristics flag candidates. The user looks at the contact sheets (3ds/logs/fx/sheet_N.png, not committed).

| # | effect | state | drew | changed px | dense cells | black | black px | solid colour % | zoomed or clipped | dense bbox % | edges |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | big_smoke_puff | ok | yes | 2010 | 63 | no | 0 | 5 | no | 40 | 0 |
| 1 | landing_dust | ok | yes | 1509 | 41 | no | 0 | 5 | no | 42 | 0 |
| 2 | walking_dust | ok | weak | 161 | 6 | no | 0 | 0 | no | 1 | 0 |
| 3 | flower_splash | ok | weak | 328 | 12 | no | 0 | 0 | no | 15 | 0 |
| 4 | flower_trail | ok | weak | 250 | 12 | no | 0 | 0 | no | 18 | 0 |
| 5 | cloud_puff | ok | yes | 1545 | 33 | no | 0 | 34 | no | 40 | 0 |
| 6 | cloud_trail | ok | weak | 275 | 10 | no | 0 | 0 | no | 15 | 0 |
| 7 | footprint | ok | weak | 304 | 13 | no | 0 | 0 | no | 42 | 0 |
| 8 | floating_flower | ok | yes | 469 | 18 | no | 0 | 0 | no | 42 | 0 |
| 9 | snowflake | ok | weak | 297 | 11 | no | 0 | 0 | no | 40 | 0 |
| 10 | star | ok | yes | 433 | 18 | no | 0 | 0 | no | 42 | 0 |
| 11 | emote | ok | weak | 146 | 6 | no | 0 | 0 | no | 1 | 0 |
| 12 | sparkles | ok | yes | 758 | 21 | no | 0 | 0 | no | 16 | 0 |
| 13 | shape_spell | ok | weak | 135 | 6 | no | 0 | 0 | no | 1 | 0 |
| 14 | gather_energy_pink | ok | weak | 256 | 11 | no | 0 | 0 | no | 42 | 0 |
| 15 | drop_leaves | ok | weak | 335 | 10 | no | 0 | 0 | no | 28 | 0 |
| 16 | dust | ok | yes | 439 | 16 | no | 0 | 0 | no | 42 | 0 |
| 17 | shattering_stones | ok | weak | 341 | 12 | no | 0 | 0 | no | 42 | 0 |
| 18 | smoke_ring | ok | yes | 671 | 23 | no | 0 | 0 | no | 40 | 0 |
| 19 | damage_stars | ok | yes | 858 | 27 | no | 0 | 18 | no | 42 | 0 |
| 20 | explosion | ok | yes | 5799 | 107 | no | 0 | 11 | no | 32 | 0 |
| 21 | lens_flare | ok | yes | 11843 | 223 | no | 0 | 5 | no | 42 | 0 |
| 22 | got_item_outline | ok | yes | 3368 | 73 | no | 99 | 17 | no | 23 | 0 |
| 23 | spiky_white_aura | ok | yes | 3088 | 73 | no | 0 | 6 | no | 40 | 0 |
| 24 | smoke_impact | ok | yes | 1003 | 24 | no | 0 | 8 | no | 16 | 0 |
| 25 | damage_indicator | ok | yes | 3380 | 76 | no | 0 | SOLID 64 | no | 42 | 0 |
| 26 | purple_ring | ok | yes | 1302 | 42 | no | 0 | 7 | no | 42 | 0 |
| 27 | flame | ok | yes | 2529 | 53 | no | 0 | SOLID 95 | no | 40 | 0 |
| 28 | stars_burst | ok | yes | 431 | 16 | no | 0 | 0 | no | 42 | 0 |
| 29 | stars_shimmer | ok | yes | 704 | 21 | no | 0 | 0 | no | 15 | 0 |
| 30 | rising_bubble | ok | weak | 174 | 7 | no | 0 | 0 | no | 14 | 0 |
| 31 | ring_blast | ok | weak | 377 | 14 | no | 0 | 0 | no | 16 | 0 |
| 32 | shockwave | ok | yes | 6886 | 132 | no | 0 | 12 | no | 42 | 0 |
| 33 | music_note | ok | weak | 350 | 9 | no | 29 | 0 | no | 19 | 0 |
| 34 | smoke_burst | ok | weak | 293 | 12 | no | 0 | 0 | no | 42 | 0 |
| 35 | sweat | ok | weak | 319 | 12 | no | 0 | 0 | no | 42 | 0 |
| 36 | sleep_bubble | ok | weak | 267 | 10 | no | 0 | 0 | no | 40 | 0 |
| 37 | windy_leaves | ok | yes | 572 | 20 | no | 0 | 0 | no | 42 | 0 |
| 38 | falling_leaves | ok | weak | 214 | 9 | no | 0 | 0 | no | 12 | 0 |
| 39 | stars_spread | ok | yes | 963 | 28 | no | 0 | 33 | no | 22 | 0 |
| 40 | steam_burst | ok | weak | 243 | 10 | no | 0 | 0 | no | 15 | 0 |
| 41 | stars_orbiting | ok | yes | 629 | 19 | no | 0 | 0 | no | 40 | 0 |
| 42 | big_snowflakes | ok | yes | 1292 | 29 | no | 0 | 25 | no | 17 | 0 |
| 43 | debuff | ok | weak | 303 | 13 | no | 0 | 0 | no | 42 | 0 |
| 44 | green_impact | ok | yes | 3664 | 76 | no | 0 | 3 | no | 42 | 0 |
| 45 | radial_shimmer | ok | yes | 1294 | 38 | no | 0 | 4 | no | 40 | 0 |
| 46 | ending_decals | ok | weak | 305 | 13 | no | 0 | 0 | no | 42 | 0 |
| 47 | light_rays | ok | yes | 3349 | 91 | no | 0 | 7 | no | 55 | 0 |
| 48 | lightning | ok | yes | 6545 | 136 | no | 0 | 2 | no | 51 | 0 |
| 49 | fire_breath | ok | weak | 147 | 7 | no | 0 | 0 | no | 14 | 0 |
| 50 | shimmer_burst | ok | yes | 1301 | 46 | no | 0 | 10 | no | 42 | 0 |
| 51 | energy_shockwave | ok | yes | 13432 | 309 | no | 0 | 10 | YES | 63 | 0 |
| 52 | shimmer_wave | ok | weak | 343 | 14 | no | 0 | 0 | no | 43 | 0 |
| 53 | aura | ok | weak | 300 | 12 | no | 0 | 0 | no | 42 | 0 |
| 54 | bulb_glow | ok | yes | 8931 | 163 | no | 0 | 16 | no | 40 | 0 |
| 55 | effect_3D | ok | yes | 3604 | 77 | no | 0 | 30 | no | 42 | 0 |
| 56 | blast | ok | yes | 2903 | 63 | no | 0 | 17 | no | 23 | 0 |
| 57 | fire_flower | ok | yes | 1945 | 49 | no | 0 | 20 | no | 24 | 0 |
| 58 | recover | ok | weak | 317 | 11 | no | 0 | 0 | no | 14 | 0 |
| 59 | disable_x | ok | weak | 222 | 8 | no | 0 | 0 | no | 40 | 0 |
| 60 | bombette_breaking | ok | weak | 106 | 4 | no | 0 | 0 | no | 1 | 0 |
| 61 | firework | ok | yes | 487 | 15 | no | 0 | 0 | no | 42 | 0 |
| 62 | confetti | ok | yes | 1386 | 55 | no | 0 | 7 | no | 45 | 0 |
| 63 | snowfall | ok | yes | 1370 | 38 | no | 0 | 18 | no | 60 | 0 |
| 64 | effect_46 | ok | yes | 614 | 22 | no | 0 | 0 | no | 42 | 0 |
| 65 | gather_magic | ok | yes | 1001 | 33 | no | 0 | 7 | no | 32 | 0 |
| 66 | attack_result_text | ok | yes | 1636 | 35 | no | 596 | 38 | no | 12 | 0 |
| 67 | small_gold_sparkle | ok | weak | 242 | 10 | no | 0 | 0 | no | 15 | 0 |
| 68 | flashing_box_shockwave | ok | weak | 244 | 10 | no | 0 | 0 | no | 42 | 0 |
| 69 | balloon | ok | weak | 358 | 12 | no | 0 | 0 | no | 8 | 0 |
| 70 | floating_rock | ok | yes | 733 | 27 | no | 0 | 0 | no | 42 | 0 |
| 71 | chomp_drop | ok | yes | 3254 | 82 | no | 0 | 15 | no | 42 | 0 |
| 72 | quizmo_stage | ok | yes | 8502 | 174 | no | 301 | 11 | YES | 63 | 0 |
| 73 | radiating_energy_orb | ok | yes | 696 | 22 | no | 0 | 0 | no | 42 | 0 |
| 74 | quizmo_answer | ok | weak | 148 | 6 | no | 0 | 0 | no | 1 | 0 |
| 75 | motion_blur_flame | ok | weak | 136 | 5 | no | 0 | 0 | no | 14 | 0 |
| 76 | energy_orb_wave | ok | yes | 7811 | 171 | no | 0 | 7 | no | 60 | 0 |
| 77 | merlin_house_stars | ok | yes | 5466 | 154 | no | 0 | 25 | no | 58 | 0 |
| 78 | quizmo_audience | ok | yes | 10387 | 201 | no | 486 | 32 | no | 53 | 0 |
| 79 | butterflies | ok | weak | 317 | 13 | no | 0 | 0 | no | 42 | 0 |
| 80 | stat_change | ok | yes | 1827 | 41 | no | 223 | 18 | no | 40 | 0 |
| 81 | snaking_static | ok | yes | 394 | 15 | no | 0 | 0 | no | 42 | 0 |
| 82 | thunderbolt_ring | ok | yes | 732 | 24 | no | 0 | 0 | no | 24 | 0 |
| 83 | squirt | ok | weak | 204 | 8 | no | 0 | 0 | no | 12 | 0 |
| 84 | water_block | ok | yes | 3946 | 70 | no | 0 | 17 | no | 19 | 0 |
| 85 | waterfall | ok | yes | 1504 | 40 | no | 0 | 6 | no | 40 | 0 |
| 86 | water_fountain | ok | yes | 6392 | 126 | no | 0 | 10 | no | 17 | 0 |
| 87 | underwater | ok | yes | 50750 | 859 | no | 0 | 7 | YES | 63 | 0 |
| 88 | lightning_bolt | ok | yes | 434 | 17 | no | 0 | 0 | no | 42 | 0 |
| 89 | water_splash | ok | yes | 589 | 23 | no | 0 | 0 | no | 40 | 0 |
| 90 | snowman_doll | ok | yes | 18473 | 332 | no | 560 | 21 | no | 42 | 0 |
| 91 | fright_jar | ok | yes | 7564 | 149 | no | 0 | 11 | no | 26 | 0 |
| 92 | stop_watch | ok | yes | 5132 | 108 | no | 0 | 4 | no | 53 | 0 |
| 93 | effect_63 | ok | weak | 135 | 6 | no | 0 | 0 | no | 1 | 0 |
| 94 | throw_spiny | ok | yes | 1110 | 28 | no | 0 | 19 | no | 42 | 0 |
| 95 | effect_65 | ok | weak | 107 | 4 | no | 0 | 0 | no | 1 | 0 |
| 96 | tubba_heart_attack | ok | yes | 1309 | 43 | no | 0 | 8 | no | 55 | 0 |
| 97 | whirlwind | ok | yes | 1705 | 41 | no | 0 | 9 | no | 42 | 0 |
| 98 | red_impact | ok | yes | 28796 | 500 | no | 88 | 12 | YES | 63 | 0 |
| 99 | floating_cloud_puff | ok | yes | 546 | 18 | no | 0 | 0 | no | 42 | 0 |
| 100 | energy_in_out | ok | weak | 230 | 8 | no | 0 | 0 | no | 15 | 0 |
| 101 | tattle_window | ok | yes | 16128 | 270 | no | 0 | SOLID 90 | no | 43 | 0 |
| 102 | shiny_flare | ok | weak | 138 | 6 | no | 0 | 0 | no | 1 | 0 |
| 103 | huff_puff_breath | ok | yes | 12079 | 214 | no | 0 | 21 | no | 52 | 0 |
| 104 | cold_breath | ok | yes | 3661 | 66 | no | 0 | 12 | no | 15 | 0 |
| 105 | embers | ok | weak | 290 | 12 | no | 0 | 0 | no | 42 | 0 |
| 106 | hieroglyphs | ok | yes | 1907 | 54 | no | 0 | 9 | no | 47 | 0 |
| 107 | misc_particles | ok | weak | 295 | 11 | no | 0 | 0 | no | 40 | 0 |
| 108 | static_status | ok | weak | 389 | 14 | no | 0 | 0 | no | 42 | 0 |
| 109 | moving_cloud | ok | weak | 145 | 6 | no | 0 | 0 | no | 1 | 0 |
| 110 | effect_75 | ok | weak | 121 | 5 | no | 0 | 0 | no | 13 | 0 |
| 111 | firework_rocket | ok | weak | 138 | 6 | no | 0 | 0 | no | 1 | 0 |
| 112 | peach_star_beam | ok | yes | 11819 | 221 | no | 1455 | 24 | no | 47 | 0 |
| 113 | chapter_change | ok | yes | 5222 | 108 | no | 1369 | 27 | no | 25 | 0 |
| 114 | ice_shard | ok | yes | 423 | 16 | no | 0 | 0 | no | 42 | 0 |
| 115 | spirit_card | ok | yes | 1663 | 42 | no | 70 | 47 | no | 42 | 0 |
| 116 | lil_oink | ok | weak | 261 | 10 | no | 0 | 0 | no | 40 | 0 |
| 117 | something_rotating | ok | yes | 2965 | 75 | no | 0 | 3 | no | 42 | 0 |
| 118 | breaking_junk | ok | weak | 328 | 14 | no | 8 | 0 | no | 18 | 0 |
| 119 | partner_buff | ok | weak | 107 | 5 | no | 0 | 0 | no | 14 | 0 |
| 120 | quizmo_assistant | ok | yes | 866 | 22 | no | 0 | 14 | no | 10 | 0 |
| 121 | ice_pillar | ok | yes | 1423 | 32 | no | 0 | 11 | no | 40 | 0 |
| 122 | sun | ok | weak | 265 | 7 | no | 0 | 0 | no | 5 | 0 |
| 123 | star_spirits_energy | ok | weak | 349 | 14 | no | 0 | 0 | no | 42 | 0 |
| 124 | pink_sparkles | ok | yes | 633 | 23 | no | 0 | 0 | no | 42 | 0 |
| 125 | star_outline | ok | yes | 5667 | 133 | no | 107 | 3 | no | 47 | 0 |
| 126 | effect_86 | ok | weak | 305 | 13 | no | 0 | 0 | no | 42 | 0 |

Spawned and shot: 127 of 127. Weak or nothing: 50. Black block: 0. Zoomed or clipped: 4. Solid colour block: 3.

FX1b sweep 2026-10-18 (after R13c, V05b, V15, V12d, V12c), sheets in 3ds/logs/fx/sheet_1.png to sheet_11.png. The script limit of 300 s stopped the first start at effect 79, a second start did effects 80 to 126. No crash.
- Fixed since the first sweep: bulb_glow 54 (no longer solid, still a white square, see row FX2c), quizmo_audience 78 (no longer clipped, dark shapes are probably the real look).
- Still wrong: flame 27 (flat red block), bulb_glow 54 (white square), motion_blur_flame 75 (now nothing drawn, black blob gone), effect_63 93 (now nothing drawn, black blob gone), peach_star_beam 112 (black block), huff_puff_breath 103 (yellow block), energy_shockwave 51 (clipped, may be a real wide ring). Rows FX2a to FX2g in 3ds/SLICES.md.
- New flags that look right: underwater 87 (full screen tint), quizmo_stage 72 (stage strip), red_impact 98 (large red burst). tattle_window 101 and chapter_change 113 need game context.
- The heuristics do not see a wrong colour or a wrong shape. Only the sheets show that.
