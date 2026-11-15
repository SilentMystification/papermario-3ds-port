#include "common.h"
#include "nd_gtab.h"
#include "hud_element.h"
#include "entity.h"
#include "include_asset.h"

#if !VERSION_PAL
Lights1 level_up_lights = gdSPDefLights1(0, 0, 0, 0, 0, 0, 0, 0, 0);

#if VERSION_JP
#include "level_up/draw_RE.vtx.inc.c"
#include "level_up/draw_BE.vtx.inc.c"
#include "level_up/draw_RU.vtx.inc.c"
#include "level_up/draw_A.vtx.inc.c"
#include "level_up/draw_LTSU.vtx.inc.c"
#include "level_up/draw_PU.vtx.inc.c"
#include "level_up/draw_exclamation_mark.vtx.inc.c"

INCLUDE_IMG("level_up/big_A.png", D_802A1398_7A6C18);
#include "level_up/big_A.gfx.inc.c"
INCLUDE_IMG("level_up/big_BE.png", D_802A2410_7A7C90);
#include "level_up/big_BE.gfx.inc.c"
INCLUDE_IMG("level_up/big_exclamation_mark.png", D_802A3488_7A8D08);
#include "level_up/big_exclamation_mark.gfx.inc.c"
INCLUDE_IMG("level_up/big_PU.png", D_802A3D00_7A9580);
#include "level_up/big_PU.gfx.inc.c"
INCLUDE_IMG("level_up/big_RE.png", D_802A4D78_7AA5F8);
#include "level_up/big_RE.gfx.inc.c"
INCLUDE_IMG("level_up/big_RU.png", D_802A5DF0_7AB670);
#include "level_up/big_RU.gfx.inc.c"
INCLUDE_IMG("level_up/big_LTSU.png", D_802A6E68_7AC6E8);
#include "level_up/big_LTSU.gfx.inc.c"
#else
#include "level_up/draw_second_E.vtx.inc.c"
#include "level_up/draw_V.vtx.inc.c"
#include "level_up/draw_E.vtx.inc.c"
#include "level_up/draw_second_L.vtx.inc.c"
#include "level_up/draw_L.vtx.inc.c"
#include "level_up/draw_P.vtx.inc.c"
#include "level_up/draw_U.vtx.inc.c"
#include "level_up/draw_exclamation_mark.vtx.inc.c"

INCLUDE_IMG("level_up/big_V.png", D_802A1418_79F358);
#include "level_up/big_V.gfx.inc.c"
INCLUDE_IMG("level_up/big_P.png", D_802A2490_7A03D0);
#include "level_up/big_P.gfx.inc.c"
INCLUDE_IMG("level_up/big_exclamation_mark.png", D_802A3508_7A1448);
#include "level_up/big_exclamation_mark.gfx.inc.c"
INCLUDE_IMG("level_up/big_U.png", D_802A4580_7A24C0);
#include "level_up/big_U.gfx.inc.c"
INCLUDE_IMG("level_up/big_L.png", D_802A55F8_7A3538);
#include "level_up/big_L.gfx.inc.c"
INCLUDE_IMG("level_up/big_E.png", D_802A6670_7A45B0);
#include "level_up/big_E.gfx.inc.c"
#endif

NDN_DECL(D_802A76E8_7A5628);

NDN_DECL(D_802A7750_7A5690);

NDN_DECL(D_802A77B8_7A56F8);

NDN_DECL(D_802A7820_7A5760);

NDN_DECL(D_802A7888_7A57C8);

NDN_DECL(D_802A78F0_7A5830);

NDN_DECL(D_802A7958_7A5898);

#if !VERSION_JP
NDN_DECL(D_802A79C0_7A5900);
#endif

NDN_DECL(D_802A7A28_7A5968);

NDN_DECL(D_802A7A88_7A59C8);
#endif

INCLUDE_IMG("level_up/heart.png", level_up_heart_png);
INCLUDE_PAL("level_up/heart.pal", level_up_heart_pal);
INCLUDE_IMG("level_up/flower.png", level_up_flower_png);
INCLUDE_PAL("level_up/flower.pal", level_up_flower_pal);
INCLUDE_IMG("level_up/leaves.png", level_up_leaves_png);
INCLUDE_PAL("level_up/leaves.pal", level_up_leaves_pal);
INCLUDE_IMG("level_up/badge.png", level_up_badge_png);
INCLUDE_PAL("level_up/badge.pal", level_up_badge_pal);
INCLUDE_IMG("level_up/FP.png", level_up_FP_png);
INCLUDE_PAL("level_up/FP.pal", level_up_FP_pal);
INCLUDE_IMG("level_up/BP.png", level_up_BP_png);
INCLUDE_PAL("level_up/BP.pal", level_up_BP_pal);
INCLUDE_IMG("level_up/HP.png", level_up_HP_png);
INCLUDE_PAL("level_up/HP.pal", level_up_HP_pal);

#if VERSION_PAL
INCLUDE_IMG("de/level_up/FP.png", level_up_FP_de_png);
INCLUDE_PAL("de/level_up/FP.pal", level_up_FP_de_pal);
INCLUDE_IMG("de/level_up/BP.png", level_up_BP_de_png);
INCLUDE_PAL("de/level_up/BP.pal", level_up_BP_de_pal);
INCLUDE_IMG("de/level_up/HP.png", level_up_HP_de_png);
INCLUDE_PAL("de/level_up/HP.pal", level_up_HP_de_pal);
INCLUDE_IMG("fr/level_up/FP.png", level_up_FP_fr_png);
INCLUDE_PAL("fr/level_up/FP.pal", level_up_FP_fr_pal);
INCLUDE_IMG("fr/level_up/BP.png", level_up_BP_fr_png);
INCLUDE_PAL("fr/level_up/BP.pal", level_up_BP_fr_pal);
INCLUDE_IMG("fr/level_up/HP.png", level_up_HP_fr_png);
INCLUDE_PAL("fr/level_up/HP.pal", level_up_HP_fr_pal);
INCLUDE_IMG("es/level_up/FP.png", level_up_FP_es_png);
INCLUDE_PAL("es/level_up/FP.pal", level_up_FP_es_pal);
INCLUDE_IMG("es/level_up/BP.png", level_up_BP_es_png);
INCLUDE_PAL("es/level_up/BP.pal", level_up_BP_es_pal);
INCLUDE_IMG("es/level_up/HP.png", level_up_HP_es_png);
INCLUDE_PAL("es/level_up/HP.pal", level_up_HP_es_pal);
#endif

INCLUDE_IMG("level_up/digit_0.png", level_up_digit_0_png);
INCLUDE_PAL("level_up/digit_0.pal", level_up_digit_0_pal);
INCLUDE_IMG("level_up/digit_1.png", level_up_digit_1_png);
INCLUDE_PAL("level_up/digit_1.pal", level_up_digit_1_pal);
INCLUDE_IMG("level_up/digit_2.png", level_up_digit_2_png);
INCLUDE_PAL("level_up/digit_2.pal", level_up_digit_2_pal);
INCLUDE_IMG("level_up/digit_3.png", level_up_digit_3_png);
INCLUDE_PAL("level_up/digit_3.pal", level_up_digit_3_pal);
INCLUDE_IMG("level_up/digit_4.png", level_up_digit_4_png);
INCLUDE_PAL("level_up/digit_4.pal", level_up_digit_4_pal);
INCLUDE_IMG("level_up/digit_5.png", level_up_digit_5_png);
INCLUDE_PAL("level_up/digit_5.pal", level_up_digit_5_pal);
INCLUDE_IMG("level_up/digit_6.png", level_up_digit_6_png);
INCLUDE_PAL("level_up/digit_6.pal", level_up_digit_6_pal);
INCLUDE_IMG("level_up/digit_7.png", level_up_digit_7_png);
INCLUDE_PAL("level_up/digit_7.pal", level_up_digit_7_pal);
INCLUDE_IMG("level_up/digit_8.png", level_up_digit_8_png);
INCLUDE_PAL("level_up/digit_8.pal", level_up_digit_8_pal);
INCLUDE_IMG("level_up/digit_9.png", level_up_digit_9_png);
INCLUDE_PAL("level_up/digit_9.pal", level_up_digit_9_pal);

INCLUDE_IMG("level_up/small_digit_0.png", level_up_small_digit_0_png);
INCLUDE_PAL("level_up/small_digit_0.pal", level_up_small_digit_0_pal);
INCLUDE_IMG("level_up/small_digit_1.png", level_up_small_digit_1_png);
INCLUDE_PAL("level_up/small_digit_1.pal", level_up_small_digit_1_pal);
INCLUDE_IMG("level_up/small_digit_2.png", level_up_small_digit_2_png);
INCLUDE_PAL("level_up/small_digit_2.pal", level_up_small_digit_2_pal);
INCLUDE_IMG("level_up/small_digit_3.png", level_up_small_digit_3_png);
INCLUDE_PAL("level_up/small_digit_3.pal", level_up_small_digit_3_pal);
INCLUDE_IMG("level_up/small_digit_4.png", level_up_small_digit_4_png);
INCLUDE_PAL("level_up/small_digit_4.pal", level_up_small_digit_4_pal);
INCLUDE_IMG("level_up/small_digit_5.png", level_up_small_digit_5_png);
INCLUDE_PAL("level_up/small_digit_5.pal", level_up_small_digit_5_pal);
INCLUDE_IMG("level_up/small_digit_6.png", level_up_small_digit_6_png);
INCLUDE_PAL("level_up/small_digit_6.pal", level_up_small_digit_6_pal);
INCLUDE_IMG("level_up/small_digit_7.png", level_up_small_digit_7_png);
INCLUDE_PAL("level_up/small_digit_7.pal", level_up_small_digit_7_pal);
INCLUDE_IMG("level_up/small_digit_8.png", level_up_small_digit_8_png);
INCLUDE_PAL("level_up/small_digit_8.pal", level_up_small_digit_8_pal);
INCLUDE_IMG("level_up/small_digit_9.png", level_up_small_digit_9_png);
INCLUDE_PAL("level_up/small_digit_9.pal", level_up_small_digit_9_pal);
INCLUDE_IMG("level_up/small_arrow.png", level_up_small_arrow_png);
INCLUDE_PAL("level_up/small_arrow.pal", level_up_small_arrow_pal);

#define level_up_digit_0_red_png level_up_digit_0_png
INCLUDE_PAL("level_up/digit_0.red.pal", level_up_digit_0_red_pal);
#define level_up_digit_1_red_png level_up_digit_1_png
INCLUDE_PAL("level_up/digit_1.red.pal", level_up_digit_1_red_pal);
#define level_up_digit_2_red_png level_up_digit_2_png
INCLUDE_PAL("level_up/digit_2.red.pal", level_up_digit_2_red_pal);
#define level_up_digit_3_red_png level_up_digit_3_png
INCLUDE_PAL("level_up/digit_3.red.pal", level_up_digit_3_red_pal);
#define level_up_digit_4_red_png level_up_digit_4_png
INCLUDE_PAL("level_up/digit_4.red.pal", level_up_digit_4_red_pal);
#define level_up_digit_5_red_png level_up_digit_5_png
INCLUDE_PAL("level_up/digit_5.red.pal", level_up_digit_5_red_pal);
#define level_up_digit_6_red_png level_up_digit_6_png
INCLUDE_PAL("level_up/digit_6.red.pal", level_up_digit_6_red_pal);
#define level_up_digit_7_red_png level_up_digit_7_png
INCLUDE_PAL("level_up/digit_7.red.pal", level_up_digit_7_red_pal);
#define level_up_digit_8_red_png level_up_digit_8_png
INCLUDE_PAL("level_up/digit_8.red.pal", level_up_digit_8_red_pal);
#define level_up_digit_9_red_png level_up_digit_9_png
INCLUDE_PAL("level_up/digit_9.red.pal", level_up_digit_9_red_pal);

#define level_up_small_digit_0_red_png level_up_small_digit_0_png
INCLUDE_PAL("level_up/small_digit_0.red.pal", level_up_small_digit_0_red_pal);
#define level_up_small_digit_1_red_png level_up_small_digit_1_png
INCLUDE_PAL("level_up/small_digit_1.red.pal", level_up_small_digit_1_red_pal);
#define level_up_small_digit_2_red_png level_up_small_digit_2_png
INCLUDE_PAL("level_up/small_digit_2.red.pal", level_up_small_digit_2_red_pal);
#define level_up_small_digit_3_red_png level_up_small_digit_3_png
INCLUDE_PAL("level_up/small_digit_3.red.pal", level_up_small_digit_3_red_pal);
#define level_up_small_digit_4_red_png level_up_small_digit_4_png
INCLUDE_PAL("level_up/small_digit_4.red.pal", level_up_small_digit_4_red_pal);
#define level_up_small_digit_5_red_png level_up_small_digit_5_png
INCLUDE_PAL("level_up/small_digit_5.red.pal", level_up_small_digit_5_red_pal);
#define level_up_small_digit_6_red_png level_up_small_digit_6_png
INCLUDE_PAL("level_up/small_digit_6.red.pal", level_up_small_digit_6_red_pal);
#define level_up_small_digit_7_red_png level_up_small_digit_7_png
INCLUDE_PAL("level_up/small_digit_7.red.pal", level_up_small_digit_7_red_pal);
#define level_up_small_digit_8_red_png level_up_small_digit_8_png
INCLUDE_PAL("level_up/small_digit_8.red.pal", level_up_small_digit_8_red_pal);
#define level_up_small_digit_9_red_png level_up_small_digit_9_png
INCLUDE_PAL("level_up/small_digit_9.red.pal", level_up_small_digit_9_red_pal);
#define level_up_small_arrow_red_png level_up_small_arrow_png
INCLUDE_PAL("level_up/small_arrow.red.pal", level_up_small_arrow_red_pal);

#define level_up_digit_0_blue_png level_up_digit_0_png
INCLUDE_PAL("level_up/digit_0.blue.pal", level_up_digit_0_blue_pal);
#define level_up_digit_1_blue_png level_up_digit_1_png
INCLUDE_PAL("level_up/digit_1.blue.pal", level_up_digit_1_blue_pal);
#define level_up_digit_2_blue_png level_up_digit_2_png
INCLUDE_PAL("level_up/digit_2.blue.pal", level_up_digit_2_blue_pal);
#define level_up_digit_3_blue_png level_up_digit_3_png
INCLUDE_PAL("level_up/digit_3.blue.pal", level_up_digit_3_blue_pal);
#define level_up_digit_4_blue_png level_up_digit_4_png
INCLUDE_PAL("level_up/digit_4.blue.pal", level_up_digit_4_blue_pal);
#define level_up_digit_5_blue_png level_up_digit_5_png
INCLUDE_PAL("level_up/digit_5.blue.pal", level_up_digit_5_blue_pal);
#define level_up_digit_6_blue_png level_up_digit_6_png
INCLUDE_PAL("level_up/digit_6.blue.pal", level_up_digit_6_blue_pal);
#define level_up_digit_7_blue_png level_up_digit_7_png
INCLUDE_PAL("level_up/digit_7.blue.pal", level_up_digit_7_blue_pal);
#define level_up_digit_8_blue_png level_up_digit_8_png
INCLUDE_PAL("level_up/digit_8.blue.pal", level_up_digit_8_blue_pal);
#define level_up_digit_9_blue_png level_up_digit_9_png
INCLUDE_PAL("level_up/digit_9.blue.pal", level_up_digit_9_blue_pal);

#define level_up_small_digit_0_blue_png level_up_small_digit_0_png
INCLUDE_PAL("level_up/small_digit_0.blue.pal", level_up_small_digit_0_blue_pal);
#define level_up_small_digit_1_blue_png level_up_small_digit_1_png
INCLUDE_PAL("level_up/small_digit_1.blue.pal", level_up_small_digit_1_blue_pal);
#define level_up_small_digit_2_blue_png level_up_small_digit_2_png
INCLUDE_PAL("level_up/small_digit_2.blue.pal", level_up_small_digit_2_blue_pal);
#define level_up_small_digit_3_blue_png level_up_small_digit_3_png
INCLUDE_PAL("level_up/small_digit_3.blue.pal", level_up_small_digit_3_blue_pal);
#define level_up_small_digit_4_blue_png level_up_small_digit_4_png
INCLUDE_PAL("level_up/small_digit_4.blue.pal", level_up_small_digit_4_blue_pal);
#define level_up_small_digit_5_blue_png level_up_small_digit_5_png
INCLUDE_PAL("level_up/small_digit_5.blue.pal", level_up_small_digit_5_blue_pal);
#define level_up_small_digit_6_blue_png level_up_small_digit_6_png
INCLUDE_PAL("level_up/small_digit_6.blue.pal", level_up_small_digit_6_blue_pal);
#define level_up_small_digit_7_blue_png level_up_small_digit_7_png
INCLUDE_PAL("level_up/small_digit_7.blue.pal", level_up_small_digit_7_blue_pal);
#define level_up_small_digit_8_blue_png level_up_small_digit_8_png
INCLUDE_PAL("level_up/small_digit_8.blue.pal", level_up_small_digit_8_blue_pal);
#define level_up_small_digit_9_blue_png level_up_small_digit_9_png
INCLUDE_PAL("level_up/small_digit_9.blue.pal", level_up_small_digit_9_blue_pal);
#define level_up_small_arrow_blue_png level_up_small_arrow_png
INCLUDE_PAL("level_up/small_arrow.blue.pal", level_up_small_arrow_blue_pal);

INCLUDE_IMG("level_up/select_one_to_upgrade.png", level_up_select_one_to_upgrade_png);
INCLUDE_PAL("level_up/select_one_to_upgrade.pal", level_up_select_one_to_upgrade_pal);

#if VERSION_PAL
INCLUDE_IMG("de/level_up/select_one_to_upgrade.png", level_up_select_one_to_upgrade_de_png);
INCLUDE_PAL("de/level_up/select_one_to_upgrade.pal", level_up_select_one_to_upgrade_de_pal);
INCLUDE_IMG("fr/level_up/select_one_to_upgrade.png", level_up_select_one_to_upgrade_fr_png);
INCLUDE_PAL("fr/level_up/select_one_to_upgrade.pal", level_up_select_one_to_upgrade_fr_pal);
INCLUDE_IMG("es/level_up/select_one_to_upgrade.png", level_up_select_one_to_upgrade_es_png);
INCLUDE_PAL("es/level_up/select_one_to_upgrade.pal", level_up_select_one_to_upgrade_es_pal);
#endif

#if !VERSION_PAL
EntityModelScript EMS_level_up = STANDARD_ENTITY_MODEL_SCRIPT(NDN(D_802A7A88_7A59C8), RENDER_MODE_CLOUD_NO_ZCMP);
#endif

HudScript HES_level_up_heart = HES_TEMPLATE_CI_ENUM_SIZE(level_up_heart, 40, 40);
HudScript HES_level_up_heart_copy = HES_TEMPLATE_CI_ENUM_SIZE(level_up_heart, 40, 40);
HudScript HES_level_up_flower = HES_TEMPLATE_CI_ENUM_SIZE(level_up_flower, 40, 40);
HudScript HES_level_up_leaves = HES_TEMPLATE_CI_ENUM_SIZE(level_up_leaves, 40, 40);
HudScript HES_level_up_badge = HES_TEMPLATE_CI_ENUM_SIZE(level_up_badge, 40, 40);

HudScript HES_level_up_FP = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_FP, 32, 24);
#if VERSION_PAL
HudScript HES_level_up_FP_de = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_FP_de, 32, 24);
HudScript HES_level_up_FP_fr = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_FP_fr, 32, 24);
HudScript HES_level_up_FP_es = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_FP_es, 32, 24);
#endif
HudScript HES_level_up_green_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_0, 8, 16);
HudScript HES_level_up_green_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_1, 8, 16);
HudScript HES_level_up_green_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_2, 8, 16);
HudScript HES_level_up_green_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_3, 8, 16);
HudScript HES_level_up_green_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_4, 8, 16);
HudScript HES_level_up_green_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_5, 8, 16);
HudScript HES_level_up_green_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_6, 8, 16);
HudScript HES_level_up_green_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_7, 8, 16);
HudScript HES_level_up_green_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_8, 8, 16);
HudScript HES_level_up_green_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_9, 8, 16);
HudScript HES_level_up_small_green_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_0, 8, 8);
HudScript HES_level_up_small_green_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_1, 8, 8);
HudScript HES_level_up_small_green_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_2, 8, 8);
HudScript HES_level_up_small_green_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_3, 8, 8);
HudScript HES_level_up_small_green_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_4, 8, 8);
HudScript HES_level_up_small_green_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_5, 8, 8);
HudScript HES_level_up_small_green_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_6, 8, 8);
HudScript HES_level_up_small_green_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_7, 8, 8);
HudScript HES_level_up_small_green_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_8, 8, 8);
HudScript HES_level_up_small_green_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_9, 8, 8);
HudScript HES_level_up_small_green_arrow = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_arrow, 8, 8);

HudScript HES_level_up_HP = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_HP, 32, 24);
#if VERSION_PAL
HudScript HES_level_up_HP_de = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_HP_de, 32, 24);
HudScript HES_level_up_HP_fr = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_HP_fr, 32, 24);
HudScript HES_level_up_HP_es = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_HP_es, 32, 24);
#endif
HudScript HES_level_up_red_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_0_red, 8, 16);
HudScript HES_level_up_red_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_1_red, 8, 16);
HudScript HES_level_up_red_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_2_red, 8, 16);
HudScript HES_level_up_red_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_3_red, 8, 16);
HudScript HES_level_up_red_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_4_red, 8, 16);
HudScript HES_level_up_red_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_5_red, 8, 16);
HudScript HES_level_up_red_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_6_red, 8, 16);
HudScript HES_level_up_red_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_7_red, 8, 16);
HudScript HES_level_up_red_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_8_red, 8, 16);
HudScript HES_level_up_red_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_9_red, 8, 16);
HudScript HES_level_up_small_red_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_0_red, 8, 8);
HudScript HES_level_up_small_red_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_1_red, 8, 8);
HudScript HES_level_up_small_red_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_2_red, 8, 8);
HudScript HES_level_up_small_red_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_3_red, 8, 8);
HudScript HES_level_up_small_red_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_4_red, 8, 8);
HudScript HES_level_up_small_red_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_5_red, 8, 8);
HudScript HES_level_up_small_red_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_6_red, 8, 8);
HudScript HES_level_up_small_red_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_7_red, 8, 8);
HudScript HES_level_up_small_red_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_8_red, 8, 8);
HudScript HES_level_up_small_red_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_9_red, 8, 8);
HudScript HES_level_up_small_red_arrow = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_arrow_red, 8, 8);

HudScript HES_level_up_BP = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_BP, 32, 24);
#if VERSION_PAL
HudScript HES_level_up_BP_de = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_BP_de, 32, 24);
HudScript HES_level_up_BP_fr = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_BP_fr, 32, 24);
HudScript HES_level_up_BP_es = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_BP_es, 32, 24);
#endif
HudScript HES_level_up_blue_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_0_blue, 8, 16);
HudScript HES_level_up_blue_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_1_blue, 8, 16);
HudScript HES_level_up_blue_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_2_blue, 8, 16);
HudScript HES_level_up_blue_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_3_blue, 8, 16);
HudScript HES_level_up_blue_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_4_blue, 8, 16);
HudScript HES_level_up_blue_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_5_blue, 8, 16);
HudScript HES_level_up_blue_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_6_blue, 8, 16);
HudScript HES_level_up_blue_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_7_blue, 8, 16);
HudScript HES_level_up_blue_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_8_blue, 8, 16);
HudScript HES_level_up_blue_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_digit_9_blue, 8, 16);
HudScript HES_level_up_small_blue_digit_0 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_0_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_1 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_1_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_2 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_2_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_3 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_3_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_4 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_4_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_5 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_5_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_6 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_6_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_7 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_7_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_8 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_8_blue, 8, 8);
HudScript HES_level_up_small_blue_digit_9 = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_digit_9_blue, 8, 8);
HudScript HES_level_up_small_blue_arrow = HES_TEMPLATE_CI_ENUM_SIZE(level_up_small_arrow_blue, 8, 8);

HudScript HES_level_up_select_one_to_upgrade = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_select_one_to_upgrade, 208, 16);
#if VERSION_PAL
HudScript HES_level_up_select_one_to_upgrade_de = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_select_one_to_upgrade_de, 208, 16);
HudScript HES_level_up_select_one_to_upgrade_fr = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_select_one_to_upgrade_fr, 208, 16);
HudScript HES_level_up_select_one_to_upgrade_es = HES_TEMPLATE_CI_CUSTOM_SIZE(level_up_select_one_to_upgrade_es, 208, 16);
#endif
