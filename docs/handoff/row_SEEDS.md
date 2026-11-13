# Seed table of the codemod batch rows (ORACLE8, 2026-11-13, main f1f9c6835). mkrow.py reads it. Data, not a row to run.

Rule (row_TEMPLATE.md): the seed is + 20.0f on the y argument of the first guTranslateF or guPositionF of <file>_appendGfx,
with no branch before it in the function. When the y text is not unique on its line, the field holds the arguments up to
and with y (for example `sp20, 0.0f, 0.0f`), and + 20.0f goes after that text. Every draw of the effect moves; drec records the matrix uniforms. The draw
proof is the base run log: `fxtest: live 1` and `fxcenb <id> tri` above the base window. Columns: file | fxtest id |
codemod --dry value keep state dl | seed line | the call | the y argument | verdict (seedprobe.py: live, dead, unprobed).

aura | 53 | 10 0 0 5 | 307 | `guTranslateF(translateMtx, data->posB.x, data->posB.y, data->posB.z);` | `data->posB.y` | unprobed
balloon | 69 | 5 0 0 2 | 96 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
big_snowflakes | 42 | 4 0 0 2 | 123 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
blast | 56 | 6 1 0 3 | 111 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
bombette_breaking | 60 | 9 0 0 3 | 221 | `guTranslateF(sp20, data->center.x, data->center.y, data->center.z);` | `data->center.y` | unprobed
butterflies | 79 | 6 0 1 3 | 176 | `guTranslateF(sp18, data->unk_0C, data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f, data->unk_14);` | `data->unk_10 + D_E00AA6EC[data->unk_2C] * 0.3f` | unprobed
chomp_drop | 71 | 7 0 1 6 | 153 | `guPositionF(sp60, 0.0f, 0.0f, 0.0f, data->unk_24, data->unk_04 * 100.0f, data->unk_08 * 100.0f, data->unk_0C * 100.0f);` | `data->unk_08 * 100.0f` | unprobed
cold_breath | 104 | 12 0 2 2 | 200 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
confetti | 62 | 9 0 3 2 | 274 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
debuff | 43 | 9 0 0 2 | 148 | `guTranslateF(mtxTranslate, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
drop_leaves | 15 | 4 0 0 2 | 141 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
effect_3D | 55 | 5 3 0 2 | 229 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
effect_46 | 64 | 7 0 0 2 | 219 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
effect_65 | 95 | 6 0 2 1 | 254 | `guTranslateF(mtx, 0.0f, 0.0f, 0.0f);` | `mtx, 0.0f, 0.0f` | unprobed
effect_75 | 110 | 9 0 0 2 | 207 | `guTranslateF(mtxTransfrom, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
effect_86 | 126 | 7 0 0 2 | 122 | `guTranslateF(sp10, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
ending_decals | 46 | 9 0 4 2 | 149 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
energy_in_out | 100 | 9 0 0 2 | 259 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, scale, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
energy_shockwave | 51 | 9 0 0 2 | 164 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
falling_leaves | 38 | 4 0 0 2 | 136 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
fire_flower | 57 | 12 0 2 4 | 218 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
flashing_box_shockwave | 68 | 12 0 0 4 | 154 | `guTranslateF(mtxTransform, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
floating_cloud_puff | 99 | 7 0 0 2 | 128 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
floating_flower | 8 | 5 0 0 1 | 154 | `guPositionF(mtxTransform, part->rot.x, part->rot.y, 0.0f, 1.0f, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
floating_rock | 70 | 6 0 2 2 | 121 | `guPositionF(mtxTransform, data->rot.x, data->rot.z, data->rot.y, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
fright_jar | 91 | 14 0 11 1 | 130 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
gather_energy_pink | 14 | 11 0 0 2 | 179 | `guTranslateF(sp20, part->posB.x, part->posB.y, part->posB.z);` | `part->posB.y` | unprobed
gather_magic | 65 | 8 0 0 2 | 168 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | unprobed
green_impact | 44 | 12 0 4 2 | 168 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, 1.0f, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
hieroglyphs | 106 | 14 1 1 1 | 130 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
huff_puff_breath | 103 | 8 0 0 2 | 141 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
ice_pillar | 121 | 7 0 0 2 | 165 | `guTranslateF(sp10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
ice_shard | 114 | 11 0 1 3 | 147 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
landing_dust | 1 | 14 0 0 3 | 290 | `guTranslateF(mtx1, part->x, part->y, part->z);` | `part->y` | unprobed
light_rays | 47 | 9 0 0 2 | 295 | `guTranslateF(mtxTranslate, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
merlin_house_stars | 77 | 9 0 1 2 | 144 | `guTranslateF(sp10, data->unk_0C, data->unk_10, data->unk_14);` | `data->unk_10` | unprobed
moving_cloud | 109 | 7 0 0 2 | 190 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
pink_sparkles | 124 | 3 3 0 2 | 251 | `guTranslateF(sp98, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
purple_ring | 26 | 7 0 0 2 | 204 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
quizmo_assistant | 120 | 6 0 0 2 | 112 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
quizmo_audience | 78 | 8 0 0 2 | 194 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
quizmo_stage | 72 | 20 0 1 13 | 118 | `guTranslateF(sp18, data->origin.x, data->origin.y, data->origin.z);` | `data->origin.y` | unprobed
red_impact | 98 | 12 0 4 2 | 175 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, 1.0f, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
ring_blast | 31 | 11 0 4 2 | 102 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
shimmer_wave | 52 | 12 0 0 2 | 202 | `guTranslateF(sp20, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
shiny_flare | 102 | 7 0 0 2 | 113 | `guTranslateF(sp10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
small_gold_sparkle | 67 | 9 0 0 3 | 126 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | unprobed
smoke_burst | 34 | 9 0 2 2 | 117 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_10, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
smoke_impact | 24 | 13 0 2 2 | 138 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
snowflake | 9 | 7 0 0 2 | 130 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
snowman_doll | 90 | 7 0 0 2 | 302 | `guTranslateF(sp18, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
sparkles | 12 | 5 0 2 1 | 282 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
squirt | 83 | 7 0 1 1 | 179 | `guTranslateF(sp10, 0.0f, 0.0f, 0.0f);` | `sp10, 0.0f, 0.0f` | unprobed
stars_shimmer | 29 | 6 0 1 2 | 299 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
stars_spread | 39 | 7 0 0 2 | 134 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
static_status | 108 | 8 0 0 2 | 204 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
steam_burst | 40 | 11 0 0 2 | 127 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
stop_watch | 92 | 11 0 1 1 | 196 | `guTranslateF(sp20, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
sweat | 35 | 4 0 0 2 | 92 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
throw_spiny | 94 | 6 0 0 2 | 172 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
thunderbolt_ring | 82 | 11 0 0 2 | 115 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_28, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
tubba_heart_attack | 96 | 9 0 0 2 | 318 | `guTranslateF(sp18, data->centerX, data->centerY, data->centerZ);` | `data->centerY` | unprobed
waterfall | 85 | 6 0 1 1 | 136 | `guTranslateF(sp10, data->unk_04, data->unk_08, data->unk_0C);` | `data->unk_08` | unprobed
whirlwind | 97 | 14 0 3 1 | 179 | `guTranslateF(sp20, 0.0f, 0.0f, 0.0f);` | `sp20, 0.0f, 0.0f` | unprobed

radiating_energy_orb | 73 | 20 0 2 7 | 150 | `guTranslateF(sp18, part->unk_08, part->unk_0C, part->unk_10);` | `part->unk_0C` | unprobed
damage_indicator | 25 | 14 0 4 15 | 208 | `guTranslateF(mtxTransform, part->basePos.x, part->basePos.y, part->basePos.z);` | `part->basePos.y` | unprobed
peach_star_beam | 112 | 15 0 0 8 | 211 | `guTranslateF(sp20, data->pos.x, data->pos.y - (((f32) (255 - data->unk_3C) * 400.0) / 255.0), data->pos.z);` | `data->pos.y` | unprobed
snowfall | 63 | 4 3 0 2 | 214 | `guTranslateF(sp18, data->unk_08, data->unk_0C, data->unk_10);` | `data->unk_0C` | unprobed

The 4 lines above `Later` (radiating_energy_orb, damage_indicator, peach_star_beam, snowfall) are oracle picks: the call is at
function level after closed branches (snowfall: inside its draw guard, which holds whenever the effect draws).

energy_orb_wave | 76 | 10 0 5 5 | 269 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
fire_breath | 49 | 10 0 0 4 | 214 | `guTranslateF(transformMtx, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
flame | 27 | 8 0 4 4 | 203 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
radial_shimmer | 45 | 11 0 0 2 | 328 | `guTranslateF(sp20, data->unk_10, data->unk_14, data->unk_18);` | `data->unk_14` | unprobed
shape_spell | 13 | 10 0 1 1 | 251 | `guPositionF(mL, 0.0f, yawOut, 0.0f, 1.0f, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
smoke_ring | 18 | 12 0 0 2 | 144 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
star_outline | 125 | 10 0 6 4 | 234 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->unk_38 * 0.4, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
water_fountain | 86 | 13 0 0 3 | 241 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
windy_leaves | 37 | 4 0 0 2 | 248 | `guTranslateF(sp18, part->unk_04.x, part->unk_04.y, part->unk_04.z);` | `part->unk_04.y` | unprobed

Oracle picks of 2026-11-13 (second pass): the first guTranslateF or guPositionF at the top level of the function body (brace depth 1),
after closed branches.

attack_result_text | 66 | 9 0 4 5 | 196 | `guTranslateF(mtxA, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
breaking_junk | 118 | 7 0 0 2 | 146 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale * 0.5, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
cloud_puff | 5 | 6 0 0 2 | 131 | `guScaleF(sp60, part->unk_18, part->unk_1C, part->unk_20);` | `part->unk_1C` | unprobed
cloud_trail | 6 | 6 0 0 2 | 132 | `guScaleF(sp60, part->unk_1C, part->unk_20, part->unk_24);` | `part->unk_20` | unprobed
damage_stars | 19 | 7 0 0 2 | 257 | `guTranslateF(mtxTransform, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
disable_x | 59 | 13 0 4 11 | 219 | `guTranslateF(sp18, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
effect_63 | 93 | 9 0 0 2 | 250 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
emote | 11 | 13 0 0 4 | 225 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
firework | 61 | 11 1 1 1 | 203 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, unk_28, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
got_item_outline | 22 | 11 0 5 2 | 132 | `guTranslateF(mtxTransform, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
lightning | 48 | 13 0 0 2 | 259 | `guTranslateF(sp20, data->unk_04, data->unk_08, -500.0f);` | `data->unk_08` | unprobed
lightning_bolt | 88 | 6 0 2 1 | 212 | `guScaleF(sp10, 0.1f, 0.1f, 0.1f);` | `sp10, 0.1f, 0.1f` | unprobed
misc_particles | 107 | 8 0 0 2 | 318 | `guPositionF(mtxTransform, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, particle->scale * alphaScale, particle->pos.x, particle->pos.y, particle->pos.z);` | `particle->pos.y` | unprobed
recover | 58 | 10 0 4 10 | 180 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
rising_bubble | 30 | 9 0 0 4 | 132 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
shattering_stones | 17 | 7 0 0 3 | 154 | `guTranslateF(sp20, part->unk_00, part->unk_04, part->unk_08);` | `part->unk_04` | unprobed
shockwave | 32 | 12 0 6 2 | 327 | `guTranslateF(sp20, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
something_rotating | 117 | 10 0 0 6 | 272 | `guRotateF(sp60, part->unk_1C, 0.0f, 0.0f, 1.0f);` | `part->unk_1C` | unprobed
spiky_white_aura | 23 | 5 0 0 2 | 190 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
spirit_card | 115 | 6 0 0 4 | 143 | `guPositionF(sp20, 0.0f, data->yaw - temp, 0.0f, data->unk_18, data->pos.x + 2.0f, data->pos.y, data->pos.z + 2.0f);` | `data->pos.y` | unprobed
star_spirits_energy | 123 | 12 0 6 3 | 361 | `guPositionF(sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f, 0.0f);` | `sp20, 0.0f, 0.0f, 0.0f, data->unk_40[i], 0.0f, 0.0f` | unprobed
stars_burst | 28 | 5 0 0 2 | 158 | `guTranslateF(sp18, part->unk_04, part->unk_08, part->unk_0C);` | `part->unk_08` | unprobed
stars_orbiting | 41 | 7 0 0 2 | 120 | `guTranslateF(sp18, part->pos.x, part->pos.y, part->pos.z);` | `part->pos.y` | unprobed
stat_change | 80 | 10 0 4 8 | 249 | `guPositionF(sp20, 0.0f, -gCameras[gCurrentCameraID].curYaw, 0.0f, data->scale, data->pos.x, data->pos.y, data->pos.z);` | `data->pos.y` | unprobed
sun | 122 | 10 0 0 2 | 176 | `guTranslateF(mtx, 0.0f, 0.0f, 0.0f);` | `mtx, 0.0f, 0.0f` | unprobed
walking_dust | 2 | 10 0 0 2 | 157 | `guTranslateF(sp18, data->unk_08, data->unk_0C, data->unk_10);` | `data->unk_0C` | unprobed

Oracle picks of 2026-11-13 (third pass, the files above this note down to the second-pass note): the first gu* transform call
(Translate, Position, Scale or Rotate, F or not) in the draw function at any depth; + 20.0f goes on the y argument (the angle
for Rotate). A guard that is false in the scene shows as a dead verdict, and the oracle then picks again.

Later (no batch yet): file | value | reason
bulb_glow | 5 | no transform call in its draw functions
chapter_change | 7 | no transform call in its draw functions
firework_rocket | 4 | no transform call in its draw functions
flower_splash | 6 | no transform call in its draw functions
flower_trail | 7 | no transform call in its draw functions
footprint | 6 | no transform call in its draw functions
lil_oink | 5 | no transform call in its draw functions
motion_blur_flame | 6 | no transform call in its draw functions
partner_buff | 5 | no transform call in its draw functions
quizmo_answer | 6 | no transform call in its draw functions
tattle_window | 5 | no transform call in its draw functions
underwater | 9 | no transform call in its draw functions
