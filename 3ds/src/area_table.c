// Port-side gAreas. Same AREA and MAP layout as src/world/world.c, so the real area and map indices and mapCount hold.
// Only the maps in the 3DS image have MAP() rows (real settings). Every other map is MAP_ID(): id only, null settings.
// load_map_by_IDs would dereference a null settings pointer, so pm_map_guard.c refuses a GotoMap to such a row.
// world.c itself is built with its table cut off (see CMakeLists.txt, WORLD_C_GEN).
#include "common.h"
#include "map.h"

#define AREA(area, jp_name) { ARRAY_COUNT(area##_maps), area##_maps, "area_" #area, jp_name }

// The map code is linked into the image, so no overlay DMA. dmaStart null skips the dma_copy in load_map_by_IDs.
#define MAP(map)     .id = #map,     .settings = &map##_settings

#define MAP_WITH_INIT(map)     MAP(map),     .init = &map##_map_init

#define MAP_ID(map) .id = #map

/// Toad Town
#include "world/area_mac/mac.h"
MapConfig mac_maps[] = {
    { MAP_ID(machi), .bgName = "nok_bg" },
    { MAP(mac_00), .bgName = "nok_bg" },
    { MAP_ID(mac_01), .bgName = "nok_bg" },
    { MAP_ID(mac_02), .bgName = "nok_bg" },
    { MAP_ID(mac_03), .bgName = "nok_bg" },
    { MAP_ID(mac_04), .bgName = "nok_bg" },
    { MAP_ID(mac_05), .bgName = "nok_bg" },
    { MAP_ID(mac_06), .bgName = "nok_bg" },
};

/// Toad Town Tunnels
MapConfig tik_maps[] = {
    { MAP_ID(tik_01), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_02), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_03), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_04), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_05), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_06), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_07), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_08), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_10), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_12), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_14), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_15), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_17), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_18), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_19), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_20), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_21), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_22), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_23), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_24), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(tik_25), .songVariation = 1, .sfxReverb = 2 },
};

/// Inside the Whale
MapConfig kgr_maps[] = {
    { MAP_ID(kgr_01), .sfxReverb = 1 },
    { MAP_ID(kgr_02), .sfxReverb = 1 },
};

/// Goomba Region
#include "world/area_kmr/kmr.h"
MapConfig kmr_maps[] = {
    { MAP_ID(kmr_00), .bgName = "kmr_bg", .sfxReverb = 1 },
    { MAP(kmr_02), .bgName = "kmr_bg" },
    { MAP_ID(kmr_03), .bgName = "kmr_bg" },
    { MAP_ID(kmr_04), .bgName = "kmr_bg" },
    { MAP_ID(kmr_05), .bgName = "kmr_bg" },
    { MAP_ID(kmr_06), .bgName = "kmr_bg" },
    { MAP_ID(kmr_07), .bgName = "kmr_bg" },
    { MAP_ID(kmr_09), .bgName = "kmr_bg" },
    { MAP_ID(kmr_10), .bgName = "kmr_bg" },
    { MAP_ID(kmr_11), .bgName = "kmr_bg" },
    { MAP_ID(kmr_12), .bgName = "kmr_bg" },
    { MAP(kmr_20), .bgName = "kmr_bg" }, // Mario's House
    { MAP_ID(kmr_21) },
    { MAP_ID(kmr_22) },
    { MAP_ID(kmr_23) },
    { MAP_ID(kmr_24) },
    { MAP_ID(kmr_30) },
};

/// Mt. Rugged
MapConfig iwa_maps[] = {
    { MAP_ID(iwa_00), .bgName = "iwa_bg" },
    { MAP_ID(iwa_01), .bgName = "iwa_bg" },
    { MAP_ID(iwa_02), .bgName = "iwa_bg" },
    { MAP_ID(iwa_03), .bgName = "iwa_bg" },
    { MAP_ID(iwa_04), .bgName = "iwa_bg" },
    { MAP_ID(iwa_10), .bgName = "iwa_bg" },
    { MAP_ID(iwa_11), .bgName = "iwa_bg" },
};

/// Dry Dry Outpost
MapConfig dro_maps[] = {
    { MAP_ID(dro_01), .bgName = "sbk_bg" },
    { MAP_ID(dro_02), .bgName = "sbk_bg" },
};

/// Dry Dry Desert
MapConfig sbk_maps[] = {
    { MAP_ID(sbk_00), .bgName = "sbk_bg" },
    { MAP_ID(sbk_01), .bgName = "sbk_bg" },
    { MAP_ID(sbk_02), .bgName = "sbk_bg" },
    { MAP_ID(sbk_03), .bgName = "sbk_bg" },
    { MAP_ID(sbk_04), .bgName = "sbk_bg" },
    { MAP_ID(sbk_05), .bgName = "sbk_bg" },
    { MAP_ID(sbk_06), .bgName = "sbk_bg" },
    { MAP_ID(sbk_10), .bgName = "sbk_bg" },
    { MAP_ID(sbk_11), .bgName = "sbk_bg" },
    { MAP_ID(sbk_12), .bgName = "sbk_bg" },
    { MAP_ID(sbk_13), .bgName = "sbk_bg" },
    { MAP_ID(sbk_14), .bgName = "sbk_bg" },
    { MAP_ID(sbk_15), .bgName = "sbk_bg" },
    { MAP_ID(sbk_16), .bgName = "sbk_bg" },
    { MAP_ID(sbk_20), .bgName = "sbk_bg" },
    { MAP_ID(sbk_21), .bgName = "sbk_bg" },
    { MAP_ID(sbk_22), .bgName = "sbk_bg" },
    { MAP_ID(sbk_23), .bgName = "sbk_bg" },
    { MAP_ID(sbk_24), .bgName = "sbk_bg" },
    { MAP_ID(sbk_25), .bgName = "sbk_bg" },
    { MAP_ID(sbk_26), .bgName = "sbk_bg" },
    { MAP_ID(sbk_30), .bgName = "sbk_bg" },
    { MAP_ID(sbk_31), .bgName = "sbk_bg" },
    { MAP_ID(sbk_32), .bgName = "sbk_bg" },
    { MAP_ID(sbk_33), .bgName = "sbk_bg" },
    { MAP_ID(sbk_34), .bgName = "sbk_bg" },
    { MAP_ID(sbk_35), .bgName = "sbk_bg" },
    { MAP_ID(sbk_36), .bgName = "sbk_bg" },
    { MAP_ID(sbk_40), .bgName = "sbk_bg" },
    { MAP_ID(sbk_41), .bgName = "sbk_bg" },
    { MAP_ID(sbk_42), .bgName = "sbk_bg" },
    { MAP_ID(sbk_43), .bgName = "sbk_bg" },
    { MAP_ID(sbk_44), .bgName = "sbk_bg" },
    { MAP_ID(sbk_45), .bgName = "sbk_bg" },
    { MAP_ID(sbk_46), .bgName = "sbk_bg" },
    { MAP_ID(sbk_50), .bgName = "sbk_bg" },
    { MAP_ID(sbk_51), .bgName = "sbk_bg" },
    { MAP_ID(sbk_52), .bgName = "sbk_bg" },
    { MAP_ID(sbk_53), .bgName = "sbk_bg" },
    { MAP_ID(sbk_54), .bgName = "sbk_bg" },
    { MAP_ID(sbk_55), .bgName = "sbk_bg" },
    { MAP_ID(sbk_56), .bgName = "sbk_bg" },
    { MAP_ID(sbk_60), .bgName = "sbk_bg" },
    { MAP_ID(sbk_61), .bgName = "sbk_bg" },
    { MAP_ID(sbk_62), .bgName = "sbk_bg" },
    { MAP_ID(sbk_63), .bgName = "sbk_bg" },
    { MAP_ID(sbk_64), .bgName = "sbk_bg" },
    { MAP_ID(sbk_65), .bgName = "sbk_bg" },
    { MAP_ID(sbk_66), .bgName = "sbk_bg" },
    { MAP_ID(sbk_99), .bgName = "sbk_bg" },
};

/// Dry Dry Ruins
MapConfig isk_maps[] = {
    { MAP_ID(isk_01), .bgName = "sbk3_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_02), .bgName = "sbk3_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_03), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_04), .bgName = "sbk3_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_05), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_06), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_07), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_08), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_10), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_11), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_12), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_13), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_14), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_16), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_18), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(isk_19), .songVariation = 1, .sfxReverb = 2 },
};

/// Koopa Bros. Fortress
MapConfig trd_maps[] = {
    { MAP_ID(trd_00), .bgName = "nok_bg" },
    { MAP_ID(trd_01), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_02), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_03), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_04), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_05), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_06), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_07), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_08), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(trd_09), .bgName = "nok_bg" },
    { MAP_ID(trd_10), .songVariation = 1, .sfxReverb = 3 },
};

/// Koopa Region
MapConfig nok_maps[] = {
    { MAP_ID(nok_01), .bgName = "nok_bg" },
    { MAP_ID(nok_02), .bgName = "nok_bg" },
    { MAP_ID(nok_03), .bgName = "nok_bg" },
    { MAP_ID(nok_04), .bgName = "nok_bg" },
    { MAP_ID(nok_11), .bgName = "nok_bg" },
    { MAP_ID(nok_12), .bgName = "nok_bg" },
    { MAP_ID(nok_13), .bgName = "nok_bg" },
    { MAP_ID(nok_14), .bgName = "nok_bg" },
    { MAP_ID(nok_15), .bgName = "nok_bg" },
};

/// Star Region
#include "world/area_hos/hos.h"
MapConfig hos_maps[] = {
    { MAP_ID(hos_00), .bgName = "nok_bg" },
    { MAP_ID(hos_01), .bgName = "hos_bg" },
    { MAP_ID(hos_02), .bgName = "hos_bg" },
    { MAP_ID(hos_03), .bgName = "hos_bg" },
    { MAP_ID(hos_04), .bgName = "hos_bg" },
    { MAP(hos_05), .bgName = "hos_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(hos_06), .bgName = "hos_bg" },
    { MAP_ID(hos_10), .bgName = "hos_bg" },
    { MAP_ID(hos_20), .bgName = "hos_bg" },
};

/// Bowser's Castle
MapConfig kpa_maps[] = {
    { MAP_ID(kpa_01), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_03), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_04), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_08), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_10), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_11), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_12), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_13), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_14), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_15), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_16), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_17), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_32), .bgName = "kpa_bg", .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_33), .bgName = "kpa_bg", .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_40), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_41), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_50), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_51), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_52), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_53), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_60), .bgName = "kpa_bg" },
    { MAP_ID(kpa_61), .bgName = "kpa_bg" },
    { MAP_ID(kpa_62), .bgName = "kpa_bg" },
    { MAP_ID(kpa_63), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_70), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_81), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_82), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_83), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_90), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_91), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_94), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_95), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_96), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(kpa_100), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(kpa_101), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(kpa_102), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_111), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_112), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_113), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_114), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_115), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_116), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_117), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(kpa_118), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_119), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(kpa_121), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kpa_130), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_133), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kpa_134), .songVariation = 1, .sfxReverb = 3 },
};

/// Peach's Castle Grounds
#include "world/area_osr/osr.h"
MapConfig osr_maps[] = {
    { MAP(osr_00), .bgName = "nok_bg" },
    { MAP_ID(osr_01), .bgName = "nok_bg" },
    { MAP_ID(osr_02), .bgName = "kpa_bg" },
    { MAP_ID(osr_03), .bgName = "kpa_bg" },
    { MAP_ID(osr_04), .bgName = "nok_bg" },
};

/// Peach's Castle
/// @bug There are two entries for kkj_26; the latter is unreachable.
#include "world/area_kkj/kkj.h"
MapConfig kkj_maps[] = {
    { MAP_WITH_INIT(kkj_00), .bgName = "nok_bg", .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kkj_01), .bgName = "nok_bg", .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kkj_02), .bgName = "nok_bg", .sfxReverb = 2 },
    { MAP_ID(kkj_03), .bgName = "nok_bg", .sfxReverb = 3 },
    { MAP_ID(kkj_10), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kkj_11), .songVariation = 1, .sfxReverb = 3 },
    { MAP_ID(kkj_12), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kkj_13), .bgName = "kpa_bg",  .songVariation = 1, .sfxReverb = 3},
    { MAP_ID(kkj_14), .bgName = "kpa_bg" },
    { MAP_ID(kkj_15) },
    { MAP_ID(kkj_16), .sfxReverb = 1 },
    { MAP_ID(kkj_17) },
    { MAP_ID(kkj_18), .bgName = "kpa_bg" },
    { MAP_ID(kkj_19) },
    { MAP_ID(kkj_20) },
    { MAP_ID(kkj_21), .sfxReverb = 1 },
    { MAP_ID(kkj_22), .bgName = "kpa_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kkj_23), .bgName = "kpa_bg" },
    { MAP_ID(kkj_24), .bgName = "kpa_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kkj_25), .bgName = "kpa_bg" },
    { MAP_ID(kkj_26), .bgName = "kpa_bg" },
    { MAP_ID(kkj_26), .sfxReverb = 2 },
    { MAP_ID(kkj_27), .sfxReverb = 1 },
    { MAP_ID(kkj_28), .sfxReverb = 1 },
    { MAP_ID(kkj_29), .sfxReverb = 1 },
};

/// Jade Jungle
MapConfig jan_maps[] = {
    { MAP_ID(jan_00), .bgName = "yos_bg" },
    { MAP_ID(jan_01), .bgName = "yos_bg" },
    { MAP_ID(jan_02), .bgName = "yos_bg" },
    { MAP_ID(jan_03), .bgName = "yos_bg" },
    { MAP_ID(jan_04), .bgName = "yos_bg" },
    { MAP_ID(jan_05), .bgName = "yos_bg" },
    { MAP_ID(jan_06), .bgName = "jan_bg" },
    { MAP_ID(jan_07), .bgName = "yos_bg" },
    { MAP_ID(jan_08), .bgName = "yos_bg" },
    { MAP_ID(jan_09), .bgName = "yos_bg" },
    { MAP_ID(jan_10), .bgName = "yos_bg" },
    { MAP_ID(jan_11), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(jan_12), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_13), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_14), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_15), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_16), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_17), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(jan_18), .bgName = "yos_bg" },
    { MAP_ID(jan_19), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(jan_22), .bgName = "jan_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(jan_23), .bgName = "yos_bg" },
};

/// Forever Forest
MapConfig mim_maps[] = {
    { MAP_ID(mim_01), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_02), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_03), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_04), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_05), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_06), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_07), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_08), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_09), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_10), .bgName = "nok_bg" },
    { MAP_ID(mim_11), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(mim_12), .bgName = "arn_bg", .songVariation = 1, .sfxReverb = 2 },
};

/// Boo's Mansion
MapConfig obk_maps[] = {
    { MAP_ID(obk_01), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(obk_02), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_03), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_04), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_05), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_06), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_07), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_08), .bgName = "obk_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(obk_09), .songVariation = 1, .sfxReverb = 2 },
};

/// Gusty Gulch
MapConfig arn_maps[] = {
    { MAP_ID(arn_02), .bgName = "arn_bg" },
    { MAP_ID(arn_03), .bgName = "arn_bg" },
    { MAP_ID(arn_04), .bgName = "arn_bg" },
    { MAP_ID(arn_05), .bgName = "arn_bg" },
    { MAP_ID(arn_07), .bgName = "arn_bg" },
    { MAP_ID(arn_08), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(arn_09), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(arn_10), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(arn_11), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(arn_12), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(arn_13), .songVariation = 1, .sfxReverb = 1 },
};

/// Tubba Blubba's Castle
MapConfig dgb_maps[] = {
    { MAP_ID(dgb_00), .bgName = "arn_bg" },
    { MAP_ID(dgb_01), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_02), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_03), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_04), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_05), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_06), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_07), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_08), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_10), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_11), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_12), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_13), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_14), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_15), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_16), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_17), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(dgb_18), .songVariation = 1, .sfxReverb = 2 },
};

/// Mt. Lavalava
MapConfig kzn_maps[] = {
    { MAP_ID(kzn_01), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_02), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_03), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_04), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_05), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_06), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_07), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_08), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_10), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_11), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_17), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_18), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_19), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_20), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_22), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(kzn_23), .bgName = "yos_bg", .songVariation = 1, .sfxReverb = 2 },
};

/// Flower Fields
MapConfig flo_maps[] = {
    { MAP_ID(flo_00), .bgName = "fla_bg" },
    { MAP_ID(flo_03), .bgName = "fla_bg" },
    { MAP_ID(flo_07), .bgName = "fla_bg" },
    { MAP_ID(flo_08), .bgName = "fla_bg" },
    { MAP_ID(flo_09), .bgName = "fla_bg" },
    { MAP_ID(flo_10), .bgName = "fla_bg" },
    { MAP_ID(flo_11), .bgName = "fla_bg" },
    { MAP_ID(flo_12), .bgName = "fla_bg" },
    { MAP_ID(flo_13), .bgName = "fla_bg" },
    { MAP_ID(flo_14), .bgName = "fla_bg" },
    { MAP_ID(flo_15), .bgName = "fla_bg" },
    { MAP_ID(flo_16), .bgName = "fla_bg" },
    { MAP_ID(flo_17), .bgName = "fla_bg" },
    { MAP_ID(flo_18), .bgName = "fla_bg" },
    { MAP_ID(flo_19), .bgName = "sra_bg" },
    { MAP_ID(flo_21), .bgName = "sra_bg" },
    { MAP_ID(flo_22), .bgName = "fla_bg" },
    { MAP_ID(flo_23), .bgName = "fla_bg" },
    { MAP_ID(flo_24), .bgName = "fla_bg" },
    { MAP_ID(flo_25), .bgName = "fla_bg" },
};

/// Shiver Region
MapConfig sam_maps[] = {
    { MAP_ID(sam_01), .bgName = "yki_bg" },
    { MAP_ID(sam_02), .bgName = "yki_bg" },
    { MAP_ID(sam_03), .bgName = "yki_bg" },
    { MAP_ID(sam_04), .bgName = "yki_bg" },
    { MAP_ID(sam_05), .bgName = "sam_bg" },
    { MAP_ID(sam_06), .bgName = "sam_bg" },
    { MAP_ID(sam_07), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(sam_08), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(sam_09), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(sam_10), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(sam_11), .bgName = "yki_bg" },
    { MAP_ID(sam_12), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
};

/// Crystal Palace
MapConfig pra_maps[] = {
    { MAP_ID(pra_01), .bgName = "yki_bg", .sfxReverb = 1 },
    { MAP_ID(pra_02), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_03), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_04), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_05), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_06), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_09), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_10), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_11), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_12), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_13), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_14), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_15), .bgName = "yki_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_16), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_18), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_19), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_20), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_21), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_22), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_27), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_28), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_29), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_31), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_32), .bgName = "sam_bg", .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_33), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_34), .songVariation = 1, .sfxReverb = 1 },
    { MAP_ID(pra_35), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_36), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_37), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_38), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_39), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(pra_40), .songVariation = 1, .sfxReverb = 1 },
};

/// Shy Guy's Toy Box
MapConfig omo_maps[] = {
    { MAP_ID(omo_01), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_02), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_03), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_04), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_05), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_06), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_07), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_08), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_09), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_10), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_11), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_12), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_13), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_14), .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_15), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_16), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
    { MAP_ID(omo_17), .bgName = "omo_bg", .songVariation = 1, .sfxReverb = 2 },
};

/// Debug
MapConfig tst_maps[] = {
    { MAP_ID(tst_01), .bgName = "nok_bg" },
    { MAP_ID(tst_02), .bgName = "nok_bg" },
    { MAP_ID(tst_03), .bgName = "nok_bg" },
    { MAP_ID(tst_04), .bgName = "nok_bg" },
    { MAP_ID(tst_10), .bgName = "nok_bg" },
    { MAP_ID(tst_11), .bgName = "nok_bg" },
    { MAP_ID(tst_12), .bgName = "nok_bg" },
    { MAP_ID(tst_13), .bgName = "nok_bg" },
    { MAP_ID(tst_20), .bgName = "nok_bg" },
};

/// Credits
MapConfig end_maps[] = {
    { MAP_ID(end_00) },
    { MAP_ID(end_01) },
};

/// Toad Town Playroom
MapConfig mgm_maps[] = {
    { MAP_ID(mgm_00) },
    { MAP_ID(mgm_01) },
    { MAP_ID(mgm_02) },
    { MAP_ID(mgm_03) },
};

/// Game Over
MapConfig gv_maps[] = {
    { MAP_ID(gv_01) },
};

AreaConfig gAreas[] = {
    AREA(kmr, "クリむら"),  // kuri mura [kuribou village, Goomba Village]
    AREA(mac, "まち"),  // machi [town, Toad Town]
    AREA(tik, "まちのちか"),  // machi no chika [under the town, Toad Town Tunnels]
    AREA(kgr, "くじらのなか"),  // kujira no naka [inside the whale]
    AREA(kkj, "きのこ城"),  // kinoko shiro [mushroom castle, Princess Peach's Castle]
    AREA(hos, "星ふる丘"),  // hoshi furu oka [star fall hill, Shooting Star Summit]
    AREA(nok, "ノコノコむら"),  // nokonoko mura [nokonoko village, Koopa Village]
    AREA(trd, "とりで"),  // toride [fortress, Koopa Bros. Fortress]
    AREA(iwa, "岩山"),  // iwayama [rock mountain, Mt. Rugged]
    AREA(dro, "カラカラタウン"),  // karakara taun [dried up town, Dry Dry Outpost]
    AREA(sbk, "カラカラ砂漠"),  // karakara sabaku [dried up desert, Dry Dry Desert]
    AREA(isk, "カラカラいせき"),  // karakara iseki [dried up ruins, Dry Dry Ruins]
    AREA(mim, "迷いの森"),  // mayoi no mori [lost forest, Forever Forest]
    AREA(obk, "テレサハウス"),  // teresa hausu [teresa house, Boo's Mansion]
    AREA(arn, "あれの"),  // areno [wasteland, Gusty Gulch]
    AREA(dgb, "ドガボンの城"),  // dogabon no shiro [dogabon's castle, Tubba Blubba's Castle]
    AREA(omo, "ヘイホーのおもちゃばこ"),  // heiho no omochabako [heiho's toy box, Shy Guy's Toy Box]
    AREA(jan, "ジャングル"),  // janguru [jungle, Jade Jungle]
    AREA(kzn, "火山"),  // kazan [volcano, Mt. Lavalava]
    AREA(flo, "フラワーランド"),  // furawaa rando [flower land, Flower Fields]
    AREA(sam, "さむいさむい村"),  // samui samui mura [cold cold village, Shiver City]
    AREA(pra, "パラレルきゅうでん"),  // parareru kyuuden [parallel palace, Crystal Palace]
    AREA(kpa, "クッパ城"),  // kuppa shiro [koopa's castle, Bowser's Castle]
    AREA(osr, "きのこ城そと"),  // kinoko shiro soto [outside mushroom castle, Outside Peach's Castle]
    AREA(end, "エンディング"),  // endingu [Ending]
    AREA(mgm, "ミニゲーム"),  // minige-mu [Minigames]
    AREA(gv, "ゲームオーバー"),  // ge-mu o-ba- [Game Over]
    AREA(tst, "テストマップ"),  // tesuto mappu [Test map]
    {},
};
