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
| 5 | cloud_puff | ok | yes | 1624 | 34 | no | 0 | 32 | no | 40 | 0 |
| 6 | cloud_trail | ok | weak | 265 | 10 | no | 0 | 0 | no | 15 | 0 |
| 7 | footprint | ok | weak | 261 | 10 | no | 0 | 0 | no | 40 | 0 |
| 8 | floating_flower | ok | yes | 469 | 18 | no | 0 | 0 | no | 42 | 0 |
| 9 | snowflake | ok | weak | 326 | 13 | no | 0 | 0 | no | 42 | 0 |
| 10 | star | ok | yes | 3957 | 85 | no | 0 | 11 | no | 43 | 0 |
| 11 | emote | ok | weak | 313 | 13 | no | 0 | 0 | no | 42 | 0 |
| 12 | sparkles | ok | yes | 758 | 21 | no | 0 | 0 | no | 16 | 0 |
| 13 | shape_spell | ok | yes | 6840 | 130 | no | 0 | 3 | no | 40 | 0 |
| 14 | gather_energy_pink | ok | weak | 326 | 4 | no | 0 | 0 | no | 1 | 0 |
| 15 | drop_leaves | ok | weak | 335 | 10 | no | 0 | 0 | no | 28 | 0 |
| 16 | dust | ok | yes | 470 | 17 | no | 0 | 0 | no | 42 | 0 |
| 17 | shattering_stones | ok | weak | 327 | 12 | no | 1 | 0 | no | 42 | 0 |
| 18 | smoke_ring | ok | yes | 671 | 23 | no | 0 | 0 | no | 40 | 0 |
| 19 | damage_stars | ok | yes | 858 | 27 | no | 0 | 18 | no | 42 | 0 |
| 20 | explosion | ok | yes | 5795 | 107 | no | 0 | 10 | no | 31 | 0 |
| 21 | lens_flare | ok | yes | 11843 | 223 | no | 0 | 5 | no | 42 | 0 |
| 22 | got_item_outline | ok | yes | 3368 | 73 | no | 99 | 17 | no | 23 | 0 |
| 23 | spiky_white_aura | ok | yes | 3088 | 73 | no | 0 | 6 | no | 40 | 0 |
| 24 | smoke_impact | ok | yes | 1003 | 24 | no | 0 | 8 | no | 16 | 0 |
| 25 | damage_indicator | ok | yes | 3380 | 76 | no | 0 | SOLID 64 | no | 42 | 0 |
| 26 | purple_ring | ok | yes | 647 | 20 | no | 0 | 0 | no | 40 | 0 |
| 27 | flame | ok | yes | 747 | 24 | no | 0 | 0 | no | 42 | 0 |
| 28 | stars_burst | ok | yes | 434 | 17 | no | 0 | 0 | no | 40 | 0 |
| 29 | stars_shimmer | ok | yes | 889 | 28 | no | 0 | 5 | no | 42 | 0 |
| 30 | rising_bubble | ok | weak | 240 | 8 | no | 0 | 0 | no | 40 | 0 |
| 31 | ring_blast | ok | weak | 362 | 13 | no | 0 | 0 | no | 15 | 0 |
| 32 | shockwave | ok | yes | 6877 | 132 | no | 0 | 12 | no | 25 | 0 |
| 33 | music_note | ok | yes | 522 | 17 | no | 15 | 0 | no | 43 | 0 |
| 34 | smoke_burst | ok | yes | 1073 | 25 | no | 0 | 10 | no | 17 | 0 |
| 35 | sweat | ok | weak | 311 | 12 | no | 0 | 0 | no | 42 | 0 |
| 36 | sleep_bubble | ok | yes | 1103 | 38 | no | 0 | SOLID 77 | no | 42 | 0 |
| 37 | windy_leaves | ok | yes | 589 | 19 | no | 0 | 0 | no | 43 | 0 |
| 38 | falling_leaves | ok | yes | 539 | 20 | no | 0 | 0 | no | 42 | 0 |
| 39 | stars_spread | ok | yes | 1004 | 30 | no | 0 | 32 | no | 23 | 0 |
| 40 | steam_burst | ok | yes | 46307 | 882 | no | 6 | 12 | YES | 66 | 0 |
| 41 | stars_orbiting | ok | yes | 24006 | 614 | no | 29 | 6 | YES | 66 | 0 |
| 42 | big_snowflakes | ok | yes | 1156 | 26 | no | 0 | 45 | no | 4 | 0 |
| 43 | debuff | ok | yes | 28514 | 656 | no | 1 | 5 | YES | 66 | 0 |
| 44 | green_impact | ok | yes | 25771 | 625 | no | 0 | 4 | YES | 66 | 0 |
| 45 | radial_shimmer | ok | yes | 1161 | 40 | no | 0 | 9 | no | 22 | 0 |
| 46 | ending_decals | ok | yes | 19883 | 534 | no | 0 | 6 | YES | 66 | 0 |
| 47 | light_rays | ok | yes | 36006 | 767 | no | 0 | 16 | YES | 66 | 0 |
| 48 | lightning | ok | yes | 6967 | 138 | no | 0 | 3 | no | 29 | 0 |
| 49 | fire_breath | ok | yes | 39158 | 658 | no | 0 | 27 | no | 56 | 0 |
| 50 | shimmer_burst | ok | yes | 1196 | 38 | no | 0 | 14 | no | 35 | 0 |
| 51 | energy_shockwave | ok | yes | 10551 | 270 | no | 0 | 10 | no | 41 | 0 |
| 52 | shimmer_wave | ok | yes | 614 | 18 | no | 0 | 0 | no | 13 | 0 |
| 53 | aura | ok | yes | 2103 | 54 | no | 0 | 6 | no | 40 | 0 |
| 54 | bulb_glow | ok | yes | 5133 | 99 | no | 0 | 8 | no | 16 | 0 |
| 55 | effect_3D | ok | yes | 5822 | 113 | no | 0 | 39 | no | 27 | 0 |
| 56 | blast | ok | yes | 3209 | 68 | no | 0 | 20 | no | 21 | 0 |
| 57 | fire_flower | ok | yes | 2716 | 66 | no | 0 | 16 | no | 17 | 0 |
| 58 | recover | ok | weak | 352 | 11 | no | 0 | 0 | no | 7 | 0 |
| 59 | disable_x | ok | yes | 448 | 15 | no | 0 | 0 | no | 9 | 0 |
| 60 | bombette_breaking | ok | yes | 1009 | 28 | no | 0 | 20 | no | 11 | 0 |
| 61 | firework | ok | yes | 13953 | 269 | no | 4945 | 36 | no | 48 | 1 |
| 62 | confetti | ok | yes | 1231 | 49 | no | 0 | 7 | no | 42 | 0 |
| 63 | snowfall | ok | yes | 1059 | 36 | no | 0 | 18 | no | 60 | 0 |
| 64 | effect_46 | ok | yes | 599 | 25 | no | 0 | 0 | no | 16 | 0 |
| 65 | gather_magic | ok | yes | 1015 | 37 | no | 0 | 7 | no | 25 | 0 |
| 66 | attack_result_text | ok | yes | 1674 | 37 | no | 596 | 37 | no | 12 | 0 |
| 67 | small_gold_sparkle | ok | yes | 411 | 18 | no | 0 | 0 | no | 15 | 0 |
| 68 | flashing_box_shockwave | ok | weak | 290 | 12 | no | 0 | 0 | no | 42 | 0 |
| 69 | balloon | ok | yes | 597 | 19 | no | 0 | 0 | no | 16 | 0 |
| 70 | floating_rock | ok | weak | 447 | 14 | no | 0 | 0 | no | 37 | 0 |
| 71 | chomp_drop | ok | yes | 3953 | 106 | no | 0 | 14 | no | 32 | 0 |
| 72 | quizmo_stage | ok | yes | 9436 | 188 | no | 398 | 11 | no | 43 | 0 |
| 73 | radiating_energy_orb | ok | yes | 1340 | 43 | no | 0 | 5 | no | 37 | 0 |
| 74 | quizmo_answer | ok | weak | 329 | 14 | no | 0 | 0 | no | 15 | 0 |
| 75 | motion_blur_flame | ok | weak | 175 | 7 | no | 0 | 0 | no | 14 | 0 |
| 76 | energy_orb_wave | ok | yes | 6551 | 158 | no | 0 | 27 | no | 57 | 0 |
| 77 | merlin_house_stars | ok | yes | 9585 | 244 | no | 0 | 24 | no | 46 | 0 |
| 78 | quizmo_audience | ok | yes | 10499 | 204 | no | 468 | 31 | YES | 63 | 0 |
| 79 | butterflies | ok | yes | 462 | 17 | no | 0 | 0 | no | 30 | 0 |
| 80 | stat_change | ok | yes | 1971 | 46 | no | 223 | 15 | no | 32 | 0 |
| 81 | snaking_static | ok | weak | 285 | 10 | no | 0 | 0 | no | 16 | 0 |
| 82 | thunderbolt_ring | ok | yes | 845 | 28 | no | 0 | 9 | no | 40 | 0 |
| 83 | squirt | ok | yes | 388 | 15 | no | 0 | 0 | no | 42 | 0 |
| 84 | water_block | ok | yes | 4081 | 70 | no | 0 | 9 | no | 23 | 0 |
| 85 | waterfall | ok | yes | 1381 | 36 | no | 0 | 7 | no | 19 | 0 |
| 86 | water_fountain | ok | yes | 6409 | 127 | no | 0 | 15 | no | 18 | 0 |
| 87 | underwater | ok | yes | 50743 | 859 | no | 0 | 7 | YES | 63 | 0 |
| 88 | lightning_bolt | ok | weak | 194 | 7 | no | 0 | 0 | no | 2 | 0 |
| 89 | water_splash | ok | yes | 665 | 27 | no | 0 | 0 | no | 42 | 0 |
| 90 | snowman_doll | ok | yes | 16928 | 304 | no | 487 | 22 | no | 42 | 0 |
| 91 | fright_jar | ok | yes | 7582 | 151 | no | 0 | 11 | no | 42 | 0 |
| 92 | stop_watch | ok | yes | 5231 | 111 | no | 0 | 4 | no | 53 | 0 |
| 93 | effect_63 | ok | yes | 4653 | 92 | no | 0 | 21 | no | 15 | 0 |
| 94 | throw_spiny | ok | yes | 1022 | 24 | no | 0 | 22 | no | 11 | 0 |
| 95 | effect_65 | ok | weak | 135 | 6 | no | 0 | 0 | no | 1 | 0 |
| 96 | tubba_heart_attack | ok | yes | 1302 | 40 | no | 0 | 9 | no | 55 | 0 |
| 97 | whirlwind | ok | yes | 1611 | 36 | no | 0 | 8 | no | 17 | 0 |
| 98 | red_impact | ok | yes | 28128 | 485 | no | 0 | 16 | YES | 63 | 0 |
| 99 | floating_cloud_puff | ok | yes | 556 | 18 | no | 0 | 0 | no | 42 | 0 |
| 100 | energy_in_out | ok | yes | 411 | 17 | no | 0 | 0 | no | 40 | 0 |
| 101 | tattle_window | ok | yes | 16313 | 278 | no | 0 | SOLID 89 | no | 48 | 0 |
| 102 | shiny_flare | ok | weak | 321 | 11 | no | 0 | 0 | no | 42 | 0 |
| 103 | huff_puff_breath | ok | yes | 4137 | 80 | no | 0 | 7 | no | 45 | 0 |
| 104 | cold_breath | ok | yes | 3661 | 66 | no | 0 | 12 | no | 15 | 0 |
| 105 | embers | ok | weak | 298 | 12 | no | 0 | 0 | no | 22 | 0 |
| 106 | hieroglyphs | ok | yes | 1885 | 52 | no | 0 | 4 | no | 42 | 0 |
| 107 | misc_particles | ok | weak | 140 | 7 | no | 0 | 0 | no | 13 | 0 |
| 108 | static_status | ok | weak | 380 | 14 | no | 0 | 0 | no | 42 | 0 |
| 109 | moving_cloud | ok | weak | 303 | 12 | no | 0 | 0 | no | 42 | 0 |
| 110 | effect_75 | ok | weak | 274 | 11 | no | 0 | 0 | no | 40 | 0 |
| 111 | firework_rocket | ok | weak | 292 | 12 | no | 0 | 0 | no | 42 | 0 |
| 112 | peach_star_beam | ok | yes | 15173 | 281 | no | 0 | 34 | no | 47 | 0 |
| 113 | chapter_change | ok | yes | 5238 | 109 | no | 1369 | 27 | no | 27 | 0 |
| 114 | ice_shard | ok | weak | 272 | 10 | no | 0 | 0 | no | 26 | 0 |
| 115 | spirit_card | ok | yes | 1776 | 42 | no | 69 | SOLID 50 | no | 42 | 0 |
| 116 | lil_oink | ok | yes | 555 | 17 | no | 4 | 0 | no | 18 | 0 |
| 117 | something_rotating | ok | yes | 2968 | 75 | no | 0 | 2 | no | 42 | 0 |
| 118 | breaking_junk | ok | yes | 459 | 17 | no | 0 | 0 | no | 42 | 0 |
| 119 | partner_buff | ok | weak | 261 | 10 | no | 0 | 0 | no | 40 | 0 |
| 120 | quizmo_assistant | ok | yes | 1024 | 28 | no | 0 | 15 | no | 45 | 0 |
| 121 | ice_pillar | ok | yes | 1408 | 29 | no | 0 | 14 | no | 16 | 0 |
| 122 | sun | ok | weak | 265 | 7 | no | 0 | 0 | no | 5 | 0 |
| 123 | star_spirits_energy | ok | weak | 165 | 6 | no | 0 | 0 | no | 14 | 0 |
| 124 | pink_sparkles | ok | yes | 606 | 23 | no | 0 | 0 | no | 40 | 0 |
| 125 | star_outline | ok | yes | 5932 | 135 | no | 0 | 5 | no | 47 | 0 |
| 126 | effect_86 | ok | yes | 775 | 21 | no | 0 | 0 | no | 16 | 0 |

Spawned and shot: 127 of 127. Weak or nothing: 32. Black block: 0. Zoomed or clipped: 9. Solid colour block: 4.
