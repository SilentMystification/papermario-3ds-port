# Seed table of the codemod batch rows (ORACLE8, 2026-11-13, main f1f9c6835). mkrow.py reads it. Data, not a row to run.

Rule (row_TEMPLATE.md): the seed is + 20.0f on the y argument of the first guTranslateF or guPositionF of <file>_appendGfx,
with no branch before it in the function. Every draw of the effect moves; drec records the matrix uniforms. The draw
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
effect_65 | 95 | 6 0 2 1 | 254 | `guTranslateF(mtx, 0.0f, 0.0f, 0.0f);` | `0.0f` | unprobed
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
squirt | 83 | 7 0 1 1 | 179 | `guTranslateF(sp10, 0.0f, 0.0f, 0.0f);` | `0.0f` | unprobed
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
whirlwind | 97 | 14 0 3 1 | 179 | `guTranslateF(sp20, 0.0f, 0.0f, 0.0f);` | `0.0f` | unprobed

Later (no batch yet): file | value | reason
attack_result_text | 9 | no unguarded gu*F before the first branch of appendGfx
breaking_junk | 7 | no unguarded gu*F before the first branch of appendGfx
bulb_glow | 5 | no unguarded gu*F before the first branch of appendGfx
chapter_change | 7 | no unguarded gu*F before the first branch of appendGfx
cloud_puff | 6 | no unguarded gu*F before the first branch of appendGfx
cloud_trail | 6 | no unguarded gu*F before the first branch of appendGfx
damage_indicator | 14 | no unguarded gu*F before the first branch of appendGfx
damage_stars | 7 | no unguarded gu*F before the first branch of appendGfx
disable_x | 13 | no unguarded gu*F before the first branch of appendGfx
effect_63 | 9 | no unguarded gu*F before the first branch of appendGfx
emote | 13 | no unguarded gu*F before the first branch of appendGfx
energy_orb_wave | 10 | no unguarded gu*F before the first branch of appendGfx
fire_breath | 10 | no unguarded gu*F before the first branch of appendGfx
firework | 11 | no unguarded gu*F before the first branch of appendGfx
firework_rocket | 4 | no unguarded gu*F before the first branch of appendGfx
flame | 8 | no unguarded gu*F before the first branch of appendGfx
flower_splash | 6 | no unguarded gu*F before the first branch of appendGfx
flower_trail | 7 | no unguarded gu*F before the first branch of appendGfx
footprint | 6 | no unguarded gu*F before the first branch of appendGfx
got_item_outline | 11 | no unguarded gu*F before the first branch of appendGfx
lightning | 13 | no unguarded gu*F before the first branch of appendGfx
lightning_bolt | 6 | no unguarded gu*F before the first branch of appendGfx
lil_oink | 5 | no unguarded gu*F before the first branch of appendGfx
misc_particles | 8 | no unguarded gu*F before the first branch of appendGfx
motion_blur_flame | 6 | no unguarded gu*F before the first branch of appendGfx
partner_buff | 5 | no unguarded gu*F before the first branch of appendGfx
peach_star_beam | 15 | no unguarded gu*F before the first branch of appendGfx
quizmo_answer | 6 | no unguarded gu*F before the first branch of appendGfx
radial_shimmer | 11 | no unguarded gu*F before the first branch of appendGfx
radiating_energy_orb | 20 | no unguarded gu*F before the first branch of appendGfx
recover | 10 | no unguarded gu*F before the first branch of appendGfx
rising_bubble | 9 | no unguarded gu*F before the first branch of appendGfx
shape_spell | 10 | no unguarded gu*F before the first branch of appendGfx
shattering_stones | 7 | no unguarded gu*F before the first branch of appendGfx
shockwave | 12 | no unguarded gu*F before the first branch of appendGfx
smoke_ring | 12 | no unguarded gu*F before the first branch of appendGfx
snowfall | 4 | no unguarded gu*F before the first branch of appendGfx
something_rotating | 10 | no unguarded gu*F before the first branch of appendGfx
spiky_white_aura | 5 | no unguarded gu*F before the first branch of appendGfx
spirit_card | 6 | no unguarded gu*F before the first branch of appendGfx
star_outline | 10 | no unguarded gu*F before the first branch of appendGfx
star_spirits_energy | 12 | no unguarded gu*F before the first branch of appendGfx
stars_burst | 5 | no unguarded gu*F before the first branch of appendGfx
stars_orbiting | 7 | no unguarded gu*F before the first branch of appendGfx
stat_change | 10 | no unguarded gu*F before the first branch of appendGfx
sun | 10 | no unguarded gu*F before the first branch of appendGfx
tattle_window | 5 | no unguarded gu*F before the first branch of appendGfx
underwater | 9 | no unguarded gu*F before the first branch of appendGfx
walking_dust | 10 | no unguarded gu*F before the first branch of appendGfx
water_fountain | 13 | no unguarded gu*F before the first branch of appendGfx
windy_leaves | 4 | no unguarded gu*F before the first branch of appendGfx
