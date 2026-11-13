/* Slice W01 (class E, port-side debug tool): `warp=<map>:<entry>`, `story=<name|number>`, `preset=<name>`.
 * No decomp function is replaced. When the game leaves GAME_MODE_STARTUP (its real set_game_mode(GAME_MODE_LOGOS)
 * call, see __wrap_set_game_mode in pm_map.c) the hook does what the real new-file menu does (clear_player_data,
 * clear_saved_variables, area/map/entry, GB_StoryProgress through evt_set_variable, fio_serialize_state) and then
 * enters the real GAME_MODE_ENTER_WORLD, which loads the map through the real state machine.
 * So logos, intro, title and file select are skipped. Without the switches nothing changes. */
#include "common.h"
#include <malloc.h>
#include "functions.h"
#include "game_modes.h"
#include "fio.h"
#include "battle/battle.h"
#include "battle/action_cmd.h"
#include "message_ids.h"
extern bool fio_fetch_saved_file_info(void);
#include "map.h"
#include "pm_port.h"
#include "audio/public.h" /* AMBTITLE: snd_ambient_is_stopped and the AU result names */
#include <stdio.h>
#include <string.h>
float strtof(const char* s, char** end); /* POSWARP: the decomp headers do not declare it (stdlib.h conflicts) */

void fio_serialize_state(void);
unsigned pm_uptime_ms(void);
bool fio_validate_file_checksum(SaveData* saveData); /* fio.c */
void fio_deserialize_state(void);
extern SaveData gCurrentSaveFile;

typedef struct {
    const char* name;
    s32 story;
    const char* map;
    s32 entry;
    s32 flag; /* a game flag (GF_*) to set, 0 = none */
} WarpPreset;

/* Tested rows only. See HANDOFF.md "Warp" for the untested ones and the reason. */
static const WarpPreset presets[] = {
    { "intro",    STORY_INTRO,                    "kmr_20", 0, 0 }, /* new file: bed scene of the real intro */
    { "yard",     STORY_CH0_ARRIVED_AT_TOAD_TOWN, "kmr_20", 4, GF_KMR20_ReunitedWithLuigi }, /* Mario's yard, enters from the pipe, controllable */
    { "toad",     STORY_CH0_ARRIVED_AT_TOAD_TOWN, "mac_00", 4, 0 }, /* Toad Town, enters from the pipe, controllable */
    { "toad_east", STORY_CH0_ARRIVED_AT_TOAD_TOWN, "mac_00", 1, 0 }, /* Toad Town east side: walk right into the loading zone to mac_01 */
    { "toad_west", STORY_CH0_ARRIVED_AT_TOAD_TOWN, "mac_00", 0, 0 }, /* Toad Town west side: kmr_10 zone */
    { "osr",      STORY_CH0_ARRIVED_AT_TOAD_TOWN, "osr_00", 1, 0 },
    { "kmr02",    STORY_CH0_WAKE_UP,              "kmr_02", 0, 0 },
    { "credits",  STORY_EPILOGUE,                 "end_00", 0, 0 }, /* PT63: the parade and the credits (kkj_00 scene_ending goes here with GotoMapSpecial end_00 entry 0) */
};

typedef struct { const char* name; s32 value; } StoryName;
static const StoryName stories[] = {
    { "intro", STORY_INTRO },
    { "ch0_wake_up", STORY_CH0_WAKE_UP },
    { "ch0_hammer", STORY_CH0_FOUND_HAMMER },
    { "ch0_goombario", STORY_CH0_GOOMBARIO_JOINED_PARTY },
    { "ch0_toad_town", STORY_CH0_ARRIVED_AT_TOAD_TOWN },
    { "ch1_start", STORY_CH1_SPOKE_WITH_MERLIN },
    { "ch1_kooper", STORY_CH1_KOOPER_JOINED_PARTY },
    { "ch2_start", STORY_CH2_ARRIVED_AT_MT_RUGGED },
    { "epilogue", STORY_EPILOGUE },
};
/* STORYMENU: the Story progress folder (Debug tab of the Select menu, pm_options.c). Every story point of include/enums.h except the UNUSED
 * values and NEVER, in enum (story) order, with its chapter: 0 Prologue, 1 to 8 Chapter 1 to 8, 9 Epilogue. The labels are the enum names read
 * as text. A point is set as the story= switch sets it (evt_set_variable GB_StoryProgress). Refused in the battle and pause modes. */
typedef struct { int value, chapter; const char* label; } StoryPoint;
static const StoryPoint sp_tab[] = {
    { -128, 0, "CH0 Intro" }, /* STORY_INTRO */
    { -127, 0, "CH0 Wake up" }, /* STORY_CH0_WAKE_UP */
    { -126, 0, "CH0 Met innkeeper" }, /* STORY_CH0_MET_INNKEEPER */
    { -124, 0, "CH0 Met goompa" }, /* STORY_CH0_MET_GOOMPA */
    { -123, 0, "CH0 Gate crushed" }, /* STORY_CH0_GATE_CRUSHED */
    { -122, 0, "CH0 Fell off cliff" }, /* STORY_CH0_FELL_OFF_CLIFF */
    { -121, 0, "CH0 Goompa joined party" }, /* STORY_CH0_GOOMPA_JOINED_PARTY */
    { -120, 0, "CH0 Looking for hammer" }, /* STORY_CH0_LOOKING_FOR_HAMMER */
    { -119, 0, "CH0 Found hammer" }, /* STORY_CH0_FOUND_HAMMER */
    { -118, 0, "CH0 Defeated jr troopa" }, /* STORY_CH0_DEFEATED_JR_TROOPA */
    { -117, 0, "CH0 Left the playground" }, /* STORY_CH0_LEFT_THE_PLAYGROUND */
    { -115, 0, "CH0 Goombario joined party" }, /* STORY_CH0_GOOMBARIO_JOINED_PARTY */
    { -114, 0, "CH0 Smashed gate block" }, /* STORY_CH0_SMASHED_GATE_BLOCK */
    { -113, 0, "CH0 Defeated goomba bros" }, /* STORY_CH0_DEFEATED_GOOMBA_BROS */
    { -112, 0, "CH0 Defeated goomba king" }, /* STORY_CH0_DEFEATED_GOOMBA_KING */
    { -111, 0, "CH0 Found gatehouse switch" }, /* STORY_CH0_FOUND_GATEHOUSE_SWITCH */
    { -110, 0, "CH0 Hit gatehouse switch" }, /* STORY_CH0_HIT_GATEHOUSE_SWITCH */
    { -109, 0, "CH0 Opened bridge to toad town" }, /* STORY_CH0_OPENED_BRIDGE_TO_TOAD_TOWN */
    { -108, 0, "CH0 Kammy returned to bowser" }, /* STORY_CH0_KAMMY_RETURNED_TO_BOWSER */
    { -107, 0, "CH0 Arrived at toad town" }, /* STORY_CH0_ARRIVED_AT_TOAD_TOWN */
    { -106, 0, "CH0 Met star spirits" }, /* STORY_CH0_MET_STAR_SPIRITS */
    { -105, 0, "CH0 Began peach mission" }, /* STORY_CH0_BEGAN_PEACH_MISSION */
    { -104, 0, "CH0 Twink gave lucky star" }, /* STORY_CH0_TWINK_GAVE_LUCKY_STAR */
    { -103, 0, "CH0 Returned to toad town" }, /* STORY_CH0_RETURNED_TO_TOAD_TOWN */
    { -102, 1, "CH1 Spoke with merlin" }, /* STORY_CH1_SPOKE_WITH_MERLIN */
    { -101, 1, "CH1 Told merlin about dark toads" }, /* STORY_CH1_TOLD_MERLIN_ABOUT_DARK_TOADS */
    { -100, 1, "CH1 Merlin revealed koopa bros" }, /* STORY_CH1_MERLIN_REVEALED_KOOPA_BROS */
    { -99, 1, "CH1 Knocked switch from tree" }, /* STORY_CH1_KNOCKED_SWITCH_FROM_TREE */
    { -98, 1, "CH1 Made first bridge" }, /* STORY_CH1_MADE_FIRST_BRIDGE */
    { -97, 1, "CH1 Arrived at koopa village" }, /* STORY_CH1_ARRIVED_AT_KOOPA_VILLAGE */
    { -96, 1, "CH1 Promised to help kooper" }, /* STORY_CH1_PROMISED_TO_HELP_KOOPER */
    { -95, 1, "CH1 Fuzzy thief left town" }, /* STORY_CH1_FUZZY_THIEF_LEFT_TOWN */
    { -94, 1, "CH1 Fuzzy thief ran away" }, /* STORY_CH1_FUZZY_THIEF_RAN_AWAY */
    { -93, 1, "CH1 Fuzzy thief hid in tree" }, /* STORY_CH1_FUZZY_THIEF_HID_IN_TREE */
    { -92, 1, "CH1 Kooper joined party" }, /* STORY_CH1_KOOPER_JOINED_PARTY */
    { -91, 1, "CH1 Made second bridge" }, /* STORY_CH1_MADE_SECOND_BRIDGE */
    { -90, 1, "CH1 Arrived at koopa fortress" }, /* STORY_CH1_ARRIVED_AT_KOOPA_FORTRESS */
    { -89, 1, "CH1 Spotted by koopa bros" }, /* STORY_CH1_SPOTTED_BY_KOOPA_BROS */
    { -88, 1, "CH1 Koopa bros hid key" }, /* STORY_CH1_KOOPA_BROS_HID_KEY */
    { -87, 1, "CH1 Defeated basement guard" }, /* STORY_CH1_DEFEATED_BASEMENT_GUARD */
    { -86, 1, "CH1 Lowered first stairs" }, /* STORY_CH1_LOWERED_FIRST_STAIRS */
    { -85, 1, "CH1 Koopa bros set trap" }, /* STORY_CH1_KOOPA_BROS_SET_TRAP */
    { -84, 1, "CH1 Mario activated trap" }, /* STORY_CH1_MARIO_ACTIVATED_TRAP */
    { -83, 1, "CH1 Bombette joined party" }, /* STORY_CH1_BOMBETTE_JOINED_PARTY */
    { -82, 1, "CH1 Defeated dungeon guards" }, /* STORY_CH1_DEFEATED_DUNGEON_GUARDS */
    { -81, 1, "CH1 Lowered second stairs" }, /* STORY_CH1_LOWERED_SECOND_STAIRS */
    { -80, 1, "CH1 Raised submerged stairs" }, /* STORY_CH1_RAISED_SUBMERGED_STAIRS */
    { -79, 1, "CH1 Koopa bros firing blasters" }, /* STORY_CH1_KOOPA_BROS_FIRING_BLASTERS */
    { -78, 1, "CH1 Defeated koopa bros" }, /* STORY_CH1_DEFEATED_KOOPA_BROS */
    { -77, 1, "CH1 Star spirit rescued" }, /* STORY_CH1_STAR_SPIRIT_RESCUED */
    { -76, 1, "CH1 Began peach mission" }, /* STORY_CH1_BEGAN_PEACH_MISSION */
    { -75, 1, "CH1 Star spirit departed" }, /* STORY_CH1_STAR_SPRIT_DEPARTED */
    { -74, 1, "CH1 Defeated jr troopa" }, /* STORY_CH1_DEFEATED_JR_TROOPA */
    { -73, 1, "CH1 Returned to toad town" }, /* STORY_CH1_RETURNED_TO_TOAD_TOWN */
    { -72, 2, "CH2 Arrived at mt rugged" }, /* STORY_CH2_ARRIVED_AT_MT_RUGGED */
    { -71, 2, "CH2 Spoke with parakarry" }, /* STORY_CH2_SPOKE_WITH_PARAKARRY */
    { -70, 2, "CH2 Parakarry joined party" }, /* STORY_CH2_PARAKARRY_JOINED_PARTY */
    { -69, 2, "CH2 Arrived at dry dry desert" }, /* STORY_CH2_ARRIVED_AT_DRY_DRY_DESERT */
    { -68, 2, "CH2 Arrived at dry dry outpost" }, /* STORY_CH2_ARRIVED_AT_DRY_DRY_OUTPOST */
    { -67, 2, "CH2 Shady mouse left shop" }, /* STORY_CH2_SHADY_MOUSE_LEFT_SHOP */
    { -66, 2, "CH2 Spoke with sheek" }, /* STORY_CH2_SPOKE_WITH_SHEEK */
    { -65, 2, "CH2 Shady mouse entered shop" }, /* STORY_CH2_SHADY_MOUSE_ENTERED_SHOP */
    { -64, 2, "CH2 Bought secret items" }, /* STORY_CH2_BOUGHT_SECRET_ITEMS */
    { -63, 2, "CH2 Got pulse stone" }, /* STORY_CH2_GOT_PULSE_STONE */
    { -62, 2, "CH2 Uncovered dry dry ruins" }, /* STORY_CH2_UNCOVERED_DRY_DRY_RUINS */
    { -61, 2, "CH2 Drained first sand room" }, /* STORY_CH2_DRAINED_FIRST_SAND_ROOM */
    { -60, 2, "CH2 Drained second sand room" }, /* STORY_CH2_DRAINED_SECOND_SAND_ROOM */
    { -59, 2, "CH2 Got super hammer" }, /* STORY_CH2_GOT_SUPER_HAMMER */
    { -58, 2, "CH2 Drained third sand room" }, /* STORY_CH2_DRAINED_THIRD_SAND_ROOM */
    { -57, 2, "CH2 Solved artifact puzzle" }, /* STORY_CH2_SOLVED_ARTIFACT_PUZZLE */
    { -56, 2, "CH2 Defeated tutankoopa" }, /* STORY_CH2_DEFEATED_TUTANKOOPA */
    { -55, 2, "CH2 Star spirit rescued" }, /* STORY_CH2_STAR_SPIRIT_RESCUED */
    { -54, 2, "CH2 Began peach mission" }, /* STORY_CH2_BEGAN_PEACH_MISSION */
    { -53, 2, "CH2 Star spirit departed" }, /* STORY_CH2_STAR_SPRIT_DEPARTED */
    { -51, 3, "CH3 Saw boo enter forest" }, /* STORY_CH3_SAW_BOO_ENTER_FOREST */
    { -50, 3, "CH3 Invited to Boo's mansion" }, /* STORY_CH3_INVITED_TO_BOOS_MANSION */
    { -49, 3, "CH3 Allowed into forever forest" }, /* STORY_CH3_ALLOWED_INTO_FOREVER_FOREST */
    { -48, 3, "CH3 Arrived at Boo's mansion" }, /* STORY_CH3_ARRIVED_AT_BOOS_MANSION */
    { -47, 3, "CH3 Opened Boo's mansion gate" }, /* STORY_CH3_OPENED_BOOS_MANSION_GATE */
    { -46, 3, "CH3 Entered Boo's mansion" }, /* STORY_CH3_ENTERED_BOOS_MANSION */
    { -45, 3, "CH3 Triggered door jump scare" }, /* STORY_CH3_TRIGGERED_DOOR_JUMP_SCARE */
    { -44, 3, "CH3 Got record" }, /* STORY_CH3_GOT_RECORD */
    { -43, 3, "CH3 Played the record" }, /* STORY_CH3_PLAYED_THE_RECORD */
    { -42, 3, "CH3 Got weight" }, /* STORY_CH3_GOT_WEIGHT */
    { -41, 3, "CH3 Weighed down chandelier" }, /* STORY_CH3_WEIGHED_DOWN_CHANDELIER */
    { -40, 3, "CH3 Got super boots" }, /* STORY_CH3_GOT_SUPER_BOOTS */
    { -39, 3, "CH3 Hit huge blue switch" }, /* STORY_CH3_HIT_HUGE_BLUE_SWITCH */
    { -38, 3, "CH3 Got boo portrait" }, /* STORY_CH3_GOT_BOO_PORTRAIT */
    { -37, 3, "CH3 Restored boo portrait" }, /* STORY_CH3_RESTORED_BOO_PORTRAIT */
    { -36, 3, "CH3 Bow joined party" }, /* STORY_CH3_BOW_JOINED_PARTY */
    { -35, 3, "CH3 Unlocked gusty gulch" }, /* STORY_CH3_UNLOCKED_GUSTY_GULCH */
    { -34, 3, "CH3 Arrived at ghost town" }, /* STORY_CH3_ARRIVED_AT_GHOST_TOWN */
    { -33, 3, "CH3 Saw tubba eat boo" }, /* STORY_CH3_SAW_TUBBA_EAT_BOO */
    { -32, 3, "CH3 Arrived at Tubba's manor" }, /* STORY_CH3_ARRIVED_AT_TUBBAS_MANOR */
    { -30, 3, "CH3 Tubba began napping" }, /* STORY_CH3_TUBBA_BEGAN_NAPPING */
    { -29, 3, "CH3 Tubba woke up" }, /* STORY_CH3_TUBBA_WOKE_UP */
    { -28, 3, "CH3 Tubba smashed the bridges" }, /* STORY_CH3_TUBBA_SMASHED_THE_BRIDGES */
    { -27, 3, "CH3 Tubba chased Mario in hall" }, /* STORY_CH3_TUBBA_CHASED_MARIO_IN_HALL */
    { -26, 3, "CH3 Tubba chased Mario in foyer" }, /* STORY_CH3_TUBBA_CHASED_MARIO_IN_FOYER */
    { -25, 3, "CH3 Escaped Tubba's manor" }, /* STORY_CH3_ESCAPED_TUBBAS_MANOR */
    { -24, 3, "CH3 Unlocked windy mill" }, /* STORY_CH3_UNLOCKED_WINDY_MILL */
    { -23, 3, "CH3 Went down the well" }, /* STORY_CH3_WENT_DOWN_THE_WELL */
    { -22, 3, "CH3 Heart fled first tunnel" }, /* STORY_CH3_HEART_FLED_FIRST_TUNNEL */
    { -19, 3, "CH3 Heart fled second tunnel" }, /* STORY_CH3_HEART_FLED_SECOND_TUNNEL */
    { -18, 3, "CH3 Heart escaped well" }, /* STORY_CH3_HEART_ESCAPED_WELL */
    { -17, 3, "CH3 Heart escaped windy mill" }, /* STORY_CH3_HEART_ESCAPED_WINDY_MILL */
    { -16, 3, "CH3 Defeated tubba blubba" }, /* STORY_CH3_DEFEATED_TUBBA_BLUBBA */
    { -15, 3, "CH3 Star spirit rescued" }, /* STORY_CH3_STAR_SPIRIT_RESCUED */
    { -14, 3, "CH3 Began peach mission" }, /* STORY_CH3_BEGAN_PEACH_MISSION */
    { -13, 3, "CH3 Star spirit departed" }, /* STORY_CH3_STAR_SPRIT_DEPARTED */
    { -12, 4, "CH4 Frying pan stolen" }, /* STORY_CH4_FRYING_PAN_STOLEN */
    { -11, 4, "CH4 Met with twink" }, /* STORY_CH4_MET_WITH_TWINK */
    { -10, 4, "CH4 Found hidden door" }, /* STORY_CH4_FOUND_HIDDEN_DOOR */
    { -9, 4, "CH4 Entered the toy box" }, /* STORY_CH4_ENTERED_THE_TOY_BOX */
    { -8, 4, "CH4 Got storeroom key" }, /* STORY_CH4_GOT_STOREROOM_KEY */
    { -7, 4, "CH4 Returned storeroom key" }, /* STORY_CH4_RETURNED_STOREROOM_KEY */
    { -6, 4, "CH4 Got toy train" }, /* STORY_CH4_GOT_TOY_TRAIN */
    { -5, 4, "CH4 Returned toy train" }, /* STORY_CH4_RETURNED_TOY_TRAIN */
    { -4, 4, "CH4 Got frying pan" }, /* STORY_CH4_GOT_FRYING_PAN */
    { -3, 4, "CH4 Got tayce ts cake" }, /* STORY_CH4_GOT_TAYCE_TS_CAKE */
    { -2, 4, "CH4 Gave cake to gourmet guy" }, /* STORY_CH4_GAVE_CAKE_TO_GOURMET_GUY */
    { -1, 4, "CH4 Pulled switch switch" }, /* STORY_CH4_PULLED_SWITCH_SWITCH */
    { 0, 4, "CH4 Solved color puzzle" }, /* STORY_CH4_SOLVED_COLOR_PUZZLE */
    { 1, 4, "CH4 Defeated lantern ghost" }, /* STORY_CH4_DEFEATED_LANTERN_GHOST */
    { 2, 4, "CH4 Watt joined party" }, /* STORY_CH4_WATT_JOINED_PARTY */
    { 3, 4, "CH4 Opened general guy room" }, /* STORY_CH4_OPENED_GENERAL_GUY_ROOM */
    { 4, 4, "CH4 Defeated general guy" }, /* STORY_CH4_DEFEATED_GENERAL_GUY */
    { 5, 4, "CH4 Star spirit rescued" }, /* STORY_CH4_STAR_SPIRIT_RESCUED */
    { 6, 4, "CH4 Began peach mission" }, /* STORY_CH4_BEGAN_PEACH_MISSION */
    { 7, 4, "CH4 Star spirit departed" }, /* STORY_CH4_STAR_SPRIT_DEPARTED */
    { 8, 5, "CH5 Whale mouth open" }, /* STORY_CH5_WHALE_MOUTH_OPEN */
    { 9, 5, "CH5 Entered whale" }, /* STORY_CH5_ENTERED_WHALE */
    { 10, 5, "CH5 Defeated fuzzipede" }, /* STORY_CH5_DEFEATED_FUZZIPEDE */
    { 11, 5, "CH5 Reached lava lava island" }, /* STORY_CH5_REACHED_LAVA_LAVA_ISLAND */
    { 12, 5, "CH5 Kolorado escaped fuzzies" }, /* STORY_CH5_KOLORADO_ESCAPED_FUZZIES */
    { 13, 5, "CH5 Kolorado escaped spear guys" }, /* STORY_CH5_KOLORADO_ESCAPED_SPEAR_GUYS */
    { 14, 5, "CH5 Yoshi children are missing" }, /* STORY_CH5_YOSHI_CHILDREN_ARE_MISSING */
    { 15, 5, "CH5 Sushie joined party" }, /* STORY_CH5_SUSHIE_JOINED_PARTY */
    { 16, 5, "CH5 All yoshi children rescued" }, /* STORY_CH5_ALL_YOSHI_CHILDREN_RESCUED */
    { 17, 5, "CH5 Got jade raven" }, /* STORY_CH5_GOT_JADE_RAVEN */
    { 18, 5, "CH5 Moved raven statue" }, /* STORY_CH5_MOVED_RAVEN_STATUE */
    { 19, 5, "CH5 Defeated piranhas miniboss" }, /* STORY_CH5_DEFEATED_PIRANHAS_MINIBOSS */
    { 20, 5, "CH5 Reached raphaels tree" }, /* STORY_CH5_REACHED_RAPHAELS_TREE */
    { 21, 5, "CH5 Raphael left nest" }, /* STORY_CH5_RAPHAEL_LEFT_NEST */
    { 22, 5, "CH5 Raphael moved root" }, /* STORY_CH5_RAPHAEL_MOVED_ROOT */
    { 23, 5, "CH5 Raphael waiting for Mario" }, /* STORY_CH5_RAPHAEL_WAITING_FOR_MARIO */
    { 24, 5, "CH5 Zip line ready" }, /* STORY_CH5_ZIP_LINE_READY */
    { 25, 5, "CH5 Entered mt lava lava" }, /* STORY_CH5_ENTERED_MT_LAVA_LAVA */
    { 26, 5, "CH5 Kolorado fell in lava" }, /* STORY_CH5_KOLORADO_FELL_IN_LAVA */
    { 27, 5, "CH5 Lava stream blocked" }, /* STORY_CH5_LAVA_STREAM_BLOCKED */
    { 28, 5, "CH5 Got ultra hammer" }, /* STORY_CH5_GOT_ULTRA_HAMMER */
    { 29, 5, "CH5 Smashed ultra block" }, /* STORY_CH5_SMASHED_ULTRA_BLOCK */
    { 30, 5, "CH5 Kolorado fell in pit" }, /* STORY_CH5_KOLORADO_FELL_IN_PIT */
    { 31, 5, "CH5 Kolorado at dead end" }, /* STORY_CH5_KOLORADO_AT_DEAD_END */
    { 32, 5, "CH5 Hidden passage open" }, /* STORY_CH5_HIDDEN_PASSAGE_OPEN */
    { 33, 5, "CH5 Kolorado ran ahead" }, /* STORY_CH5_KOLORADO_RAN_AHEAD */
    { 34, 5, "CH5 Kolorado in treasure room" }, /* STORY_CH5_KOLORADO_IN_TREASURE_ROOM */
    { 35, 5, "CH5 Defeated lava piranha" }, /* STORY_CH5_DEFEATED_LAVA_PIRANHA */
    { 36, 5, "CH5 Mt lava lava erupting" }, /* STORY_CH5_MT_LAVA_LAVA_ERUPTING */
    { 37, 5, "CH5 Opened escape route" }, /* STORY_CH5_OPENED_ESCAPE_ROUTE */
    { 38, 5, "CH5 Began peach mission" }, /* STORY_CH5_BEGAN_PEACH_MISSION */
    { 39, 5, "CH5 Star spirit departed" }, /* STORY_CH5_STAR_SPRIT_DEPARTED */
    { 40, 5, "CH5 Traded vase for seed" }, /* STORY_CH5_TRADED_VASE_FOR_SEED */
    { 41, 5, "CH5 Returned to toad town" }, /* STORY_CH5_RETURNED_TO_TOAD_TOWN */
    { 42, 6, "CH6 Flower gate open" }, /* STORY_CH6_FLOWER_GATE_OPEN */
    { 43, 6, "CH6 Arrived at flower fields" }, /* STORY_CH6_ARRIVED_AT_FLOWER_FIELDS */
    { 44, 6, "CH6 Asked to defeat monty moles" }, /* STORY_CH6_ASKED_TO_DEFEAT_MONTY_MOLES */
    { 45, 6, "CH6 Got magical bean" }, /* STORY_CH6_GOT_MAGICAL_BEAN */
    { 46, 6, "CH6 Got fertile soil" }, /* STORY_CH6_GOT_FERTILE_SOIL */
    { 47, 6, "CH6 Got crystal berry" }, /* STORY_CH6_GOT_CRYSTAL_BERRY */
    { 48, 6, "CH6 Got water stone" }, /* STORY_CH6_GOT_WATER_STONE */
    { 49, 6, "CH6 Filled spring with water" }, /* STORY_CH6_FILLED_SPRING_WITH_WATER */
    { 50, 6, "CH6 Spoke with the sun" }, /* STORY_CH6_SPOKE_WITH_THE_SUN */
    { 51, 6, "CH6 Lakilester joined party" }, /* STORY_CH6_LAKILESTER_JOINED_PARTY */
    { 52, 6, "CH6 Defeated puff puff guards" }, /* STORY_CH6_DEFEATED_PUFF_PUFF_GUARDS */
    { 53, 6, "CH6 Destroyed puff puff machine" }, /* STORY_CH6_DESTROYED_PUFF_PUFF_MACHINE */
    { 54, 6, "CH6 Wisterwood gave hint" }, /* STORY_CH6_WISTERWOOD_GAVE_HINT */
    { 55, 6, "CH6 Grew magic beanstalk" }, /* STORY_CH6_GREW_MAGIC_BEANSTALK */
    { 56, 6, "CH6 Defeated huff n puff" }, /* STORY_CH6_DEFEATED_HUFF_N_PUFF */
    { 57, 6, "CH6 Star spirit rescued" }, /* STORY_CH6_STAR_SPIRIT_RESCUED */
    { 58, 6, "CH6 Began peach mission" }, /* STORY_CH6_BEGAN_PEACH_MISSION */
    { 59, 6, "CH6 Star spirit departed" }, /* STORY_CH6_STAR_SPRIT_DEPARTED */
    { 60, 6, "CH6 Returned to toad town" }, /* STORY_CH6_RETURNED_TO_TOAD_TOWN */
    { 61, 7, "CH7 Invited to starborn valley" }, /* STORY_CH7_INVITED_TO_STARBORN_VALLEY */
    { 62, 7, "CH7 Arrived at shiver city" }, /* STORY_CH7_ARRIVED_AT_SHIVER_CITY */
    { 63, 7, "CH7 Mayor murder mystery" }, /* STORY_CH7_MAYOR_MURDER_MYSTERY */
    { 64, 7, "CH7 Shattered frozen pond" }, /* STORY_CH7_SHATTERED_FROZEN_POND */
    { 65, 7, "CH7 Spoke with herringway" }, /* STORY_CH7_SPOKE_WITH_HERRINGWAY */
    { 66, 7, "CH7 Herringway at mayors house" }, /* STORY_CH7_HERRINGWAY_AT_MAYORS_HOUSE */
    { 67, 7, "CH7 Mayor murder solved" }, /* STORY_CH7_MAYOR_MURDER_SOLVED */
    { 68, 7, "CH7 Defeated jr troopa" }, /* STORY_CH7_DEFEATED_JR_TROOPA */
    { 69, 7, "CH7 Defeated monstar" }, /* STORY_CH7_DEFEATED_MONSTAR */
    { 70, 7, "CH7 Arrived at starborn valley" }, /* STORY_CH7_ARRIVED_AT_STARBORN_VALLEY */
    { 71, 7, "CH7 Merle apologized" }, /* STORY_CH7_MERLE_APOLOGIZED */
    { 72, 7, "CH7 Got snowman scarf" }, /* STORY_CH7_GOT_SNOWMAN_SCARF */
    { 73, 7, "CH7 Got snowman bucket" }, /* STORY_CH7_GOT_SNOWMAN_BUCKET */
    { 74, 7, "CH7 Unlocked shiver mountain" }, /* STORY_CH7_UNLOCKED_SHIVER_MOUNTAIN */
    { 75, 7, "CH7 Defeated first duplighost" }, /* STORY_CH7_DEFEATED_FIRST_DUPLIGHOST */
    { 76, 7, "CH7 Got star stone" }, /* STORY_CH7_GOT_STAR_STONE */
    { 77, 7, "CH7 Raised frozen stairs" }, /* STORY_CH7_RAISED_FROZEN_STAIRS */
    { 78, 7, "CH7 Arrived at crystal palace" }, /* STORY_CH7_ARRIVED_AT_CRYSTAL_PALACE */
    { 79, 7, "CH7 Defeated mirror duplighosts" }, /* STORY_CH7_DEFEATED_MIRROR_DUPLIGHOSTS */
    { 80, 7, "CH7 Defeated bombette duplighosts" }, /* STORY_CH7_DEFEATED_BOMBETTE_DUPLIGHOSTS */
    { 81, 7, "CH7 Defeated clubbas" }, /* STORY_CH7_DEFEATED_CLUBBAS */
    { 82, 7, "CH7 Defeated kooper duplighosts" }, /* STORY_CH7_DEFEATED_KOOPER_DUPLIGHOSTS */
    { 83, 7, "CH7 Extended palace bridge" }, /* STORY_CH7_EXTENDED_PALACE_BRIDGE */
    { 84, 7, "CH7 Found hidden room under statue" }, /* STORY_CH7_FOUND_HIDDEN_ROOM_UNDER_STATUE */
    { 85, 7, "CH7 Solved albino dino puzzle" }, /* STORY_CH7_SOLVED_ALBINO_DINO_PUZZLE */
    { 86, 7, "CH7 Defeated crystal king" }, /* STORY_CH7_DEFEATED_CRYSTAL_KING */
    { 87, 7, "CH7 Star spirit rescued" }, /* STORY_CH7_STAR_SPIRIT_RESCUED */
    { 88, 7, "CH7 Began peach mission" }, /* STORY_CH7_BEGAN_PEACH_MISSION */
    { 89, 7, "CH7 Star spirit departed" }, /* STORY_CH7_STAR_SPRIT_DEPARTED */
    { 90, 8, "CH8 Opened path to star way" }, /* STORY_CH8_OPENED_PATH_TO_STAR_WAY */
    { 91, 8, "CH8 Reached star haven" }, /* STORY_CH8_REACHED_STAR_HAVEN */
    { 92, 8, "CH8 Star ship activated" }, /* STORY_CH8_STAR_SHIP_ACTIVATED */
    { 94, 8, "CH8 Reached Bowser's castle" }, /* STORY_CH8_REACHED_BOWSERS_CASTLE */
    { 95, 8, "CH8 Reached Peach's castle" }, /* STORY_CH8_REACHED_PEACHS_CASTLE */
    { 96, 9, "Epilogue" }, /* STORY_EPILOGUE */
};
#define SP_N ((int)(sizeof sp_tab / sizeof sp_tab[0]))
static const char* const sp_chapter[10] = { "Prologue", "Chapter 1", "Chapter 2", "Chapter 3", "Chapter 4", "Chapter 5", "Chapter 6", "Chapter 7", "Chapter 8", "Epilogue" };
void pm_menu_msg(const char* fmt, ...);
const char* pm_sp_chapter_name(int c) { return sp_chapter[c]; }
int pm_sp_count(int c) {
    int k, n = 0;
    for (k = 0; k < SP_N; k++) if (sp_tab[k].chapter == c) n++;
    return n;
}
/* the index in sp_tab of point i of chapter c, -1 when there is none */
static int sp_index(int c, int i) {
    int k, n = 0;
    for (k = 0; k < SP_N; k++) if (sp_tab[k].chapter == c && n++ == i) return k;
    return -1;
}
const char* pm_sp_label(int c, int i) {
    int k = sp_index(c, i);
    return k >= 0 ? sp_tab[k].label : "";
}
static int sp_cur(void) { return (int)evt_get_variable(NULL, GB_StoryProgress); }
int pm_sp_is_current(int c, int i) {
    int k = sp_index(c, i);
    return k >= 0 && sp_tab[k].value == sp_cur();
}
const char* pm_sp_current_label(void) {
    int k, cur = sp_cur();
    for (k = 0; k < SP_N; k++) if (sp_tab[k].value == cur) return sp_tab[k].label;
    return "(not in the list)";
}
/* the chapter of the current story progress, -1 when it is not a listed point */
int pm_sp_current_chapter(void) {
    int k, cur = sp_cur();
    for (k = 0; k < SP_N; k++) if (sp_tab[k].value == cur) return sp_tab[k].chapter;
    return -1;
}
static int sp_set_index(int k) {
    if (get_game_mode() == GAME_MODE_BATTLE || get_game_mode() == GAME_MODE_PAUSE) {
        pm_menu_msg("Story progress is not set in battle");
        return 0;
    }
    evt_set_variable(NULL, GB_StoryProgress, sp_tab[k].value);
    pm_log("story: set %d %s", sp_tab[k].value, sp_tab[k].label);
    return 1;
}
int pm_sp_set(int c, int i) {
    int k = sp_index(c, i);
    return k >= 0 && sp_set_index(k);
}
/* dir -1 or +1: the point before or after the current progress in story order, clamped at both ends. Returns 1 when it was set. */
int pm_sp_step(int dir) {
    int cur = sp_cur(), k = 0, j;
    while (k < SP_N && sp_tab[k].value < cur) k++;
    if (k < SP_N && sp_tab[k].value == cur) j = k + dir;
    else j = dir > 0 ? k : k - 1;
    if (j < 0) j = 0;
    if (j >= SP_N) j = SP_N - 1;
    if (k < SP_N && sp_tab[k].value == cur && j == k) return 0;
    return sp_set_index(j);
}

/* value of "key=value" in the switch string, 0 when absent */
int pm_debug_value(const char* key, char* out, unsigned n) {
    const char* sw = pm_debug_text();
    size_t kl = strlen(key);
    const char* p = sw;
    out[0] = 0;
    while ((p = strstr(p, key)) != NULL) {
        int edge = p == sw || p[-1] == ' ' || p[-1] == '\n' || p[-1] == '\t';
        if (edge && p[kl] == '=') {
            p += kl + 1;
            unsigned i = 0;
            while (*p && *p != ' ' && *p != '\n' && *p != '\t' && *p != '\r' && i + 1 < n) out[i++] = *p++;
            out[i] = 0;
            return 1;
        }
        p += kl;
    }
    return 0;
}

static int smoke_frames = 300, w_loadslot = -1;
static int warp_on, warp_done, scene_f0 = -1, smoke_on;
static int warp_nofile; /* CHEATSAVE: the warp being set up has no save file (a new-file warp); taken by the next entry into the world */
/* S30, TOUR1: map tour. tour=<map>:<entry>[@x:y:z],... or tour=@file (map:entry list in the SD app folder), or tourlist=all (every map of gAreas
 * without tst_, the spot of the map in pm_tour_spots (compiled from 3ds/tours/spots.txt) when it has the map, else entry 0; tourlist=curated
 * is the compiled curated list). After warp= (or preset=) has loaded its first map,
 * the real GotoMap path (set_map_transition_effect + set_game_mode(GAME_MODE_CHANGE_MAP), as in src/evt/map_api.c goto_map) visits every map.
 * Per item: WARP (gRandSeed = TOUR_SEED, map change) -> SETTLE (TOUR_SETTLE frames after the map is in, no window) -> MEASURE (prof window,
 * tourtime=<s> seconds, default 4) -> CLOSE (window line `prof f<n> w<n> tour <i>/<n> <map>:<entry>`) -> next WARP. A map or mode change
 * inside MEASURE closes the window early, so no window holds two scenes. End line `tour: done items <n>`. Debug tool only (class E). */
#define TOUR_SETTLE 60 /* frames: the placed position (POS_DELAY + POS_READBACK = 40) and the texture loads of the first view are done */
#define TOUR_SEED 0x2A5F3C11u
enum { TS_START, TS_WAIT, TS_SETTLE, TS_MEASURE };
static char* tour;
static const char* tour_next;
static unsigned tour_f0, tour_ts, tour_len = 120;
static int tour_n, tour_total, tour_st, tour_entry, tour_wa, tour_wm;
static char tour_name[16];
static s32 w_story = STORY_INTRO;
static int w_story_set; /* WARPFIX: story= or preset= given. Without it a loaded save keeps its story progress. */
/* WARPFIX2 Boss tab in the world: the values of the loaded save that the fight changes, restored after it */
static int boss_keep_on, boss_fight, boss_seen_battle, boss_wf, boss_bf;
static s32 bk_story, bk_hmhp, bk_cmhp, bk_chp, bk_hmfp, bk_cmfp, bk_cfp, bk_pen, bk_cur, bk_ac;
/* CHEATSAVE: 1 once after a warp setup with no save file (pm_cheat_enter_world, pm_map.c): that entry gets no cheat file */
int pm_warp_take_nofile(void) { int v = warp_nofile; warp_nofile = 0; return v; }
/* savefile=<path>: a SaveData file as fio_save_game writes it (sdmc:/3ds/PaperMario/). Loaded like loadslot, then the warp overrides map and position. */
static char w_savefile[128];
static int sf_ok;
static int pm_load_savefile(const char* path) {
    FILE* f = fopen(path, "rb");
    size_t n;
    if (!f) return 0;
    n = fread(&gCurrentSaveFile, 1, sizeof gCurrentSaveFile, f);
    fclose(f);
    if (n != sizeof gCurrentSaveFile || !fio_validate_file_checksum(&gCurrentSaveFile)) return 0;
    if (gGameStatusPtr->saveCount < gCurrentSaveFile.saveCount) gGameStatusPtr->saveCount = gCurrentSaveFile.saveCount;
    fio_deserialize_state();
    return 1;
}
static char w_map[16] = "kmr_20";
static s32 w_entry, w_flag, w_partner;

/* POSWARP: switch pos=<x>:<y>:<z>[:<angle>] puts Mario at a point of the map (floats, negatives allowed). Once per map load: POS_DELAY scene
 * frames after the world entry (the entry code has placed Mario by then), then `poswarp: placed Mario at x y z on map M yaw a (readback)` POS_READBACK
 * frames later, with the values read back from gPlayerStatus. Padscript token pos_<x>:<y>:<z>[:<angle>] places Mario at its own step (pm_input.c).
 * The write is the one reset_player_status makes (src/7B440.c): position, yaw, speed 0, time in air 0, no floor, idle state, camera target.
 * The camera then follows by its normal update (the target is set, the camera moves to it over the next frames, no hard cut). */
#define POS_DELAY 30
#define POS_READBACK 10
static float pos_v[4];
static int pos_n, pos_on, pos_state, pos_area = -1, pos_map = -1;
/* POSWARP: a pending position. A warp that starts a load arms it for that map only (pend_arm); pm_warp_world_mode applies it on that map and clears it. */
static int pend_on, pend_n, pend_area = -1, pend_map = -1, pend_tour, pos_tour;
static float pend_v[4];
static void pend_arm(int areaID, int mapID, const float* v, int n) { pend_on = 1; pend_n = n; pend_area = areaID; pend_map = mapID; memcpy(pend_v, v, sizeof pend_v); }
static unsigned pos_f0, pos_t;

int pm_pos_parse(const char* s, float* out) {
    int n = 0;
    while (n < 4) {
        char* e;
        float f = strtof(s, &e);
        if (e == s) break;
        out[n++] = f;
        if (*e != ':') break;
        s = e + 1;
    }
    return n == 3 || n == 4 ? n : 0;
}

/* WARPY: the floor under x z. The game's own collider ray (test_ray_colliders, the call of pos_probe) from y + 1000 down, length 2000.
 * *found is 0 when no collider is hit (the caller keeps the given y). */
static float pos_floor(float x, float y, float z, float up, int* found) {
    f32 hx, hy, hz, hd = 2000.0f, nx, ny, nz;
    s32 cid = test_ray_colliders(COLLIDER_FLAG_IGNORE_PLAYER, x, y + up, z, 0.0f, -1.0f, 0.0f, &hx, &hy, &hz, &hd, &nx, &ny, &nz);
    *found = cid > NO_COLLIDER;
    return *found ? hy : y;
}

/* FLOORSCAN (debug switch floorscan, WARPY2 diagnosis, placement unchanged): at scene frame 5 of a pos= run, logs every collider hit of the
 * column at the pos= x z (poswarp column: lines, the walkable flag is normal y above 0.5), then the walkable floor on a grid of 50 units
 * over the collider triangles of the map (bounds from the triangle vertices). The edge is the largest z (the side toward the default
 * camera): the 10 standable points with the largest z are logged as floorscan: edge n x y z. */
#define FS_STEP 50.0f
#define FS_MAX 200
static void pos_column(float x, float z) {
    f32 top = pos_v[1] + 1000.0f;
    int i;
    for (i = 0; i < 8; i++) {
        f32 hx, hy, hz, hd = 2000.0f, nx, ny, nz;
        s32 cid = test_ray_colliders(COLLIDER_FLAG_IGNORE_PLAYER, x, top, z, 0.0f, -1.0f, 0.0f, &hx, &hy, &hz, &hd, &nx, &ny, &nz);
        if (cid <= NO_COLLIDER) break;
        pm_log("poswarp column: x %.3f z %.3f hit y %.3f collider %d normal y %.3f walkable %d", x, z, hy, (int)cid, ny, ny > 0.5f);
        top = hy - 1.0f;
    }
}
static float pos_grid_floor(float x, float z, float top, int* found) {
    int i;
    *found = 0;
    for (i = 0; i < 8; i++) {
        f32 hx, hy, hz, hd = 4000.0f, nx, ny, nz;
        s32 cid = test_ray_colliders(COLLIDER_FLAG_IGNORE_PLAYER, x, top, z, 0.0f, -1.0f, 0.0f, &hx, &hy, &hz, &hd, &nx, &ny, &nz);
        if (cid <= NO_COLLIDER) break;
        if (ny > 0.5f) { *found = 1; return hy; }
        top = hy - 1.0f;
    }
    return 0.0f;
}
static void pos_floorscan(void) {
    int c, t, i, j, n = 0, ns = 0, found, ne = 0;
    float minx = 1e30f, maxx = -1e30f, minz = 1e30f, maxz = -1e30f, miny = 1e30f, maxy = -1e30f;
    float ex[10], ey[10], ez[10];
    pos_column(pos_v[0], pos_v[2]);
    for (c = 0; c < gCollisionData.numColliders; c++) {
        Collider* col = &gCollisionData.colliderList[c];
        if (col->numTriangles <= 0 || col->triangleTable == NULL) continue;
        for (t = 0; t < col->numTriangles; t++) {
            Vec3f* vv[3] = { col->triangleTable[t].v1, col->triangleTable[t].v2, col->triangleTable[t].v3 };
            for (j = 0; j < 3; j++) {
                if (vv[j] == NULL) continue;
                if (vv[j]->x < minx) minx = vv[j]->x;
                if (vv[j]->x > maxx) maxx = vv[j]->x;
                if (vv[j]->z < minz) minz = vv[j]->z;
                if (vv[j]->z > maxz) maxz = vv[j]->z;
                if (vv[j]->y < miny) miny = vv[j]->y;
                if (vv[j]->y > maxy) maxy = vv[j]->y;
            }
        }
    }
    if (minx > maxx) { pm_log("floorscan: no triangles"); return; }
    for (i = 0; i * FS_STEP <= maxx - minx && i < FS_MAX; i++) {
        float x = minx + i * FS_STEP;
        for (j = 0; j * FS_STEP <= maxz - minz && j < FS_MAX; j++) {
            float z = minz + j * FS_STEP;
            float y;
            y = pos_grid_floor(x, z, maxy + 1000.0f, &found);
            n++;
            if (!found) continue;
            ns++;
            if (ne < 10 || z > ez[ne - 1]) {
                int k = ne < 10 ? ne++ : ne - 1;
                while (k > 0 && ez[k - 1] < z) { ex[k] = ex[k - 1]; ey[k] = ey[k - 1]; ez[k] = ez[k - 1]; k--; }
                ex[k] = x; ey[k] = y; ez[k] = z;
            }
        }
    }
    pm_log("floorscan: bounds x %.3f..%.3f z %.3f..%.3f y %.3f..%.3f step %.0f samples %d standable %d", minx, maxx, minz, maxz, miny, maxy, FS_STEP, n, ns);
    for (i = 0; i < ne; i++) pm_log("floorscan: edge %d x %.3f y %.3f z %.3f", i, ex[i], ey[i], ez[i]);
}

/* up: the ray starts at y + up. pos= and the padscript use 1000; a tour spot uses 200 (TOUR1), below a roof over the spot. */
static void pos_place(const float* v, int n, float up) {
    PlayerStatus* ps = &gPlayerStatus;
    int found;
    float fy = pos_floor(v[0], v[1], v[2], up, &found); /* WARPY: a warp puts Mario on the floor, not at the given y */
    if (found) pm_log("poswarp: floor y %.3f -> %.3f", v[1], fy);
    else pm_log_w("poswarp: floor y %.3f -> no floor under x %.3f z %.3f, y kept", v[1], v[0], v[2]);
    ps->pos.x = v[0];
    ps->pos.y = found ? fy : v[1];
    ps->pos.z = v[2];
    if (n == 4) ps->curYaw = ps->targetYaw = v[3];
    ps->curSpeed = 0.0f;
    ps->timeInAir = 0;
    gCollisionStatus.curFloor = NO_COLLIDER;
    set_action_state(ACTION_STATE_IDLE);
    gCameras[CAM_DEFAULT].targetPos.x = ps->pos.x;
    gCameras[CAM_DEFAULT].targetPos.y = ps->pos.y;
    gCameras[CAM_DEFAULT].targetPos.z = ps->pos.z;
}
void pm_pos_set(const float* v, int n) { pos_place(v, n, 1000.0f); }

/* set: the requested position. The settled position is gPlayerStatus now (the ground logic may have moved Mario, e.g. a fall). */
void pm_ph_scene_moved(void); /* pm_phase.c */
void pm_pos_log(const float* set, const char* how) {
    PlayerStatus* ps = &gPlayerStatus;
    pm_ph_scene_moved(); /* PROF: the prof window restarts at the placed position */
    pm_log("poswarp: placed Mario at %.3f %.3f %.3f on map %s settled %.3f %.3f %.3f yaw %.2f (%s)", set[0], set[1], set[2],
           gAreas[gGameStatusPtr->areaID].maps[gGameStatusPtr->mapID].id, ps->pos.x, ps->pos.y, ps->pos.z, ps->curYaw, how);
}

/* POSWARP probe (diagnosis): the floor under the spot at x z, from the ray down from y 400 (test_ray_colliders, the call of the NPC
 * floor query) and the player ground check (player_check_collision_below), plus the collider count. Logged at the scene frames 0, 30, 60, 120, 180. */
static void pos_probe(unsigned f) {
    f32 hx, hy, hz, hd, nx, ny, nz, gy;
    s32 cid = NO_COLLIDER, pid = NO_COLLIDER;
    PlayerStatus* ps = &gPlayerStatus;
    s32 hit;
    hd = 1000.0f; /* the maximum ray length goes in as hitDepth (player_raycast_down does the same) */
    hit = test_ray_colliders(COLLIDER_FLAG_IGNORE_PLAYER, pos_v[0], 400.0f, pos_v[2], 0.0f, -1.0f, 0.0f, &hx, &hy, &hz, &hd, &nx, &ny, &nz);
    if (hit > NO_COLLIDER) { cid = hit; gy = hy; } else gy = -9999.0f;
    {
        f32 py = player_check_collision_below(0.0f, &pid);
        pm_log("poswarp probe f%u mario %.3f %.3f %.3f ray y %.3f id %d ground y %.3f id %d colliders %d", f, ps->pos.x, ps->pos.y, ps->pos.z, gy, (int)cid, py, (int)pid, (int)gCollisionData.numColliders);
    }
    { /* MENU3 row 1 (entry test): what the entry leaves behind. alpha 0 = Mario not drawn, input off = the script or a cutscene holds the player, msg 1 = a text box is open, cam = the camera target, floor = curFloor */
        extern s32 pm_msg_active(void);
        pm_log("poswarp state f%u entry %d alpha %u input %d cutscene %d lock %d flags %08x msg %d overrides %08x cam %.1f %.1f %.1f eye %.1f %.1f %.1f mario %.1f %.1f %.1f floor %d",
               f, gGameStatusPtr->entryID, (unsigned)ps->curAlpha, (int)ps->inputDisabledCount, (ps->flags & PS_FLAG_CUTSCENE_MOVEMENT) != 0,
               (ps->flags & PS_FLAG_INPUT_DISABLED) != 0, (unsigned)ps->flags, (int)pm_msg_active(), (unsigned)gOverrideFlags,
               gCameras[CAM_DEFAULT].targetPos.x, gCameras[CAM_DEFAULT].targetPos.y, gCameras[CAM_DEFAULT].targetPos.z,
               gCameras[CAM_DEFAULT].lookAt_eye.x, gCameras[CAM_DEFAULT].lookAt_eye.y, gCameras[CAM_DEFAULT].lookAt_eye.z,
               ps->pos.x, ps->pos.y, ps->pos.z, (int)gCollisionStatus.curFloor);
    }
}

static void pos_tick(unsigned frame) {
    if (!pos_on || get_game_mode() != GAME_MODE_WORLD) return;
    {
        unsigned pf = frame - pos_f0;
        if (pf == 0) { /* MENU3: posshot=<k> takes one shot (6 frames) k scene frames after the pos= run starts (shotwin counts absolute frames) */
            char pv[12];
            extern void pm_gpu_shot_window(unsigned from, unsigned n);
            if (pm_debug_value("posshot", pv, sizeof pv)) pm_gpu_shot_window(frame + (unsigned)atoi(pv), 6);
        }
        if (pf % 30 == 0 && pf <= 300) pos_probe(pf); /* MENU3: every 30 frames to 300 (entry test) */
        if (pf == 5 && pm_debug_has("floorscan")) pos_floorscan();
    }
    if (pos_state == 0 && frame - pos_f0 >= POS_DELAY) {
        if (pos_tour) {
            pos_place(pos_v, pos_n, 200.0f);
            pm_log_evt("tour: at %s y %.3f", gAreas[gGameStatusPtr->areaID].maps[gGameStatusPtr->mapID].id, gPlayerStatus.pos.y);
        } else pm_pos_set(pos_v, pos_n);
        gCameras[CAM_DEFAULT].needsReinit = true; /* MENU3 camera: the zone camera re-inits at the placed spot (cam_mode_zone_interp.c needsReinit block) */
        pos_state = 1;
        pos_t = frame;
    } else if (pos_state == 1 && frame - pos_t == 1) {
        pm_log("poswarp: camera reinit eye %.1f %.1f %.1f mario %.1f %.1f %.1f", gCameras[CAM_DEFAULT].lookAt_eye.x, gCameras[CAM_DEFAULT].lookAt_eye.y, gCameras[CAM_DEFAULT].lookAt_eye.z, gPlayerStatus.pos.x, gPlayerStatus.pos.y, gPlayerStatus.pos.z);
    } else if (pos_state == 1 && frame - pos_t >= POS_READBACK) {
        pm_pos_log(pos_v, "readback");
        pos_state = 2;
    }
}

/* TOUR1: a whole file of the SD app folder, NUL-terminated (malloc, once per run). NULL when it does not open. */
static char* tour_file(const char* fn) {
    FILE* f = fopen(fn, "rb");
    long n;
    char* b;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    b = n >= 0 ? malloc((size_t)n + 1) : NULL;
    if (b) { n = (long)fread(b, 1, (size_t)n, f); b[n] = 0; }
    fclose(f);
    return b;
}
/* TOUR1: the spot of map id in the spot table sp (pm_tour_spots: lines map:entry@x:y:z, one per map): its first token into o, else id:0. */
static void tour_spot(const char* sp, const char* id, char* o, unsigned on) {
    size_t k = strlen(id);
    const char* l = sp;
    while (l && *l) {
        if (strncmp(l, id, k) == 0 && l[k] == ':') {
            unsigned j = 0;
            while (l[j] && l[j] != ' ' && l[j] != '\t' && l[j] != '\r' && l[j] != '\n' && j + 1 < on) { o[j] = l[j]; j++; }
            o[j] = 0;
            return;
        }
        l = strchr(l, '\n');
        if (l) l++;
    }
    snprintf(o, on, "%s:0", id);
}
#include "pm_tour_lists.h" /* CUR1: pm_tour_spots and pm_tour_curated, generated by tools/gen_tour_lists.py from 3ds/tours/spots.txt and curated.txt */
/* TOUR1: the tour switches (tour=, tour=@file, tourlist=all|curated|@file, tourtime=<s>). Items are separated by commas in the tour buffer. */
static void tour_parse(void) {
    char v[256];
    if (pm_debug_value("tourlist", v, sizeof v) && strcmp(v, "curated") == 0) {
        tour = strdup(pm_tour_curated); /* CUR1: the compiled list only (the spots are in it: no SD file at run time) */
        pm_log("tour: list curated, %d maps (pm_tour_lists.h)", PM_TOUR_CURATED_N);
    } else if (pm_debug_value("tourlist", v, sizeof v) && strcmp(v, "all") == 0) {
        int i, j, ns = 0;
        size_t cap = 40u * (size_t)pm_warp_map_count() + 1, len = 0;
        tour = malloc(cap);
        if (tour) {
            tour[0] = 0;
            for (i = 0; gAreas[i].maps != NULL; i++)
                for (j = 0; j < gAreas[i].mapCount; j++) {
                    const char* id = gAreas[i].maps[j].id;
                    char it[40];
                    if (pm_warp_is_test_map(id)) continue;
                    tour_spot(pm_tour_spots, id, it, sizeof it);
                    ns += strchr(it, '@') != NULL;
                    len += (size_t)snprintf(tour + len, cap - len, "%s,", it);
                }
        }
        pm_log("tour: list all, %d maps with a spot (pm_tour_spots, %d lines)", ns, PM_TOUR_SPOTS_N);
    } else if (pm_debug_value("tourlist", v, sizeof v) || pm_debug_value("tour", v, sizeof v)) {
        if (v[0] == '@') { /* @file: the switch string is only 256 bytes */
            char* c;
            tour = tour_file(v + 1);
            if (!tour) pm_log_w("tour: file %s not found", v + 1);
            for (c = tour; c && *c; c++) if (*c == '\n' || *c == '\r' || *c == ' ') *c = ',';
        } else tour = strdup(v);
    }
    if (!tour) return;
    tour_next = tour;
    {
        const char* c = tour;
        tour_total = 0;
        while (*c) { /* items: runs of characters between commas */
            while (*c == ',') c++;
            if (*c) tour_total++;
            while (*c && *c != ',') c++;
        }
    }
    if (pm_debug_value("tourtime", v, sizeof v)) { float t = strtof(v, NULL); if (t > 0.f) tour_len = (unsigned)(t * 30.f); }
    tour_st = TS_START;
    { extern void pm_ph_tour_mode(int on); pm_ph_tour_mode(1); }
    pm_log("tour: %d items, settle %d frames, window %u frames, seed %08x", tour_total, TOUR_SETTLE, tour_len, TOUR_SEED);
}

/* RANDWARP: the spot table line at l ends at its newline (or the string end). */
static const char* rw_line_next(const char* l) {
    const char* n = strchr(l, '\n');
    return n ? n + 1 : l + strlen(l);
}
/* RANDWARP: 1 when the spot table line at l has a spot (map:entry@x:y:z), 0 for a map without one (tik_17:0). */
static int rw_line_spot(const char* l) {
    for (; *l && *l != '\n'; l++) if (*l == '@') return 1;
    return 0;
}
/* RANDWARP: warp=random[:<seed>]. One spot of pm_tour_spots (the lines with a point) picked by seed % count. The seed is the given value,
 * else the low 32 bits of the system tick. The log line names the seed and the spot, so the run repeats with warp=random:<seed>.
 * Sets the boot warp's map, entry and position (pos_v) and the tour story. Returns 1 when a spot was picked. */
static int rw_warp_random(const char* arg) {
    unsigned seed, cnt = 0, pick = 0, k = 0;
    const char *l, *hit = NULL, *at;
    char pv[40];
    unsigned j = 0;
    seed = arg[0] == ':' ? (unsigned)strtoul(arg + 1, NULL, 0) : pm_ticks(); /* low 32 bits of the system tick */
    for (l = pm_tour_spots; *l; l = rw_line_next(l)) cnt += (unsigned)rw_line_spot(l);
    if (cnt == 0) { pm_log_w("warp: random, no spot in pm_tour_spots"); return 0; }
    pick = seed % cnt;
    for (l = pm_tour_spots; *l && !hit; l = rw_line_next(l)) {
        if (!rw_line_spot(l)) continue;
        if (k == pick) hit = l;
        k++;
    }
    /* the line: map:entry@x:y:z */
    w_map[0] = 0;
    for (j = 0; hit[j] != ':' && j + 1 < sizeof w_map; j++) w_map[j] = hit[j];
    w_map[j] = 0;
    w_entry = atoi(hit + j + 1);
    at = strchr(hit, '@');
    for (j = 0; at[1 + j] && at[1 + j] != '\n' && at[1 + j] != '\r' && at[1 + j] != ' ' && j + 1 < sizeof pv; j++) pv[j] = at[1 + j];
    pv[j] = 0;
    pos_n = pm_pos_parse(pv, pos_v);
    pos_on = pos_n != 0;
    w_story = STORY_CH0_ARRIVED_AT_TOAD_TOWN; /* the tour's story value; story= after it wins */
    pm_log("warp: random seed %u -> %s:%d@%s", seed, w_map, (int)w_entry, pv);
    return 1;
}
static void bt_parse(void);
int pm_warp_active(void) {
    char v[40];
    int has_preset = pm_debug_value("preset", v, sizeof v);
    /* PT01b: the Warp area choice of the options is not used at boot. Only the switches preset, warp and story warp at boot. */
    if (has_preset) {
        int i, found = 0;
        for (i = 0; i < (int)ARRAY_COUNT(presets); i++) {
            if (strcasecmp(v, presets[i].name) == 0) {
                w_story = presets[i].story;
                strncpy(w_map, presets[i].map, sizeof w_map - 1);
                w_entry = presets[i].entry;
                w_flag = presets[i].flag;
                found = 1;
            }
        }
        if (!found) pm_log_w("warp: unknown preset '%s'", v);
    }
    int opt_map = 0;
    int has_warp = pm_debug_value("warp", v, sizeof v);
    if (has_warp && strncmp(v, "random", 6) == 0 && (v[6] == 0 || v[6] == ':')) { /* RANDWARP */
        has_warp = rw_warp_random(v + 6);
    } else if (has_warp) {
        char* c = strchr(v, ':');
        w_entry = 0;
        if (c) {
            *c = 0;
            w_entry = atoi(c + 1);
        }
        w_map[0] = 0;
        strncat(w_map, v, sizeof w_map - 1);
        if (pm_warp_is_test_map(w_map)) { /* PT01b */
            pm_log_w("warp: map %s is a test map, ignored", w_map);
            w_map[0] = 0; has_warp = 0;
        }
    }
    int has_story = pm_debug_value("story", v, sizeof v);
    if (has_preset || has_story) w_story_set = 1; /* WARPFIX */
    if (has_story) {
        int i, found = 0;
        for (i = 0; i < (int)ARRAY_COUNT(stories); i++) {
            if (strcasecmp(v, stories[i].name) == 0) {
                w_story = stories[i].value;
                found = 1;
            }
        }
        if (!found) w_story = atoi(v);
    }
    if (pm_debug_value("partner", v, sizeof v)) w_partner = atoi(v); /* partner=<n>: PARTNER_* id, 1 = Goombario */
    tour_parse();
    bt_parse(); /* BT1 */
    /* F01: a tour visits maps at story 0 with a new file (no partner). The real game has a partner at trd_06 and later maps (trd_06 InitializeFallingPartner, partner_walking_enable need one), so the tour uses Goombario unless partner= says otherwise. */
    if (tour_next != NULL && w_partner == 0) w_partner = 1;
    /* H06e: a warp past the prologue also needs a partner. tik_01_CheckDripCollisionWithNPC reads the partner npc, the real game has one in the sewers. */
    if ((has_warp || opt_map) && w_partner == 0 && strncmp(w_map, "kmr_", 4) != 0) w_partner = 1;
    smoke_on = pm_debug_has("smoke");
    if (pm_debug_value("loadslot", v, sizeof v)) w_loadslot = atoi(v); /* E02: loadslot=<n> loads save slot n first */
    pm_debug_value("savefile", w_savefile, sizeof w_savefile); /* savefile=<path>: empty when not given */
    if (pm_debug_value("smokeframes", v, sizeof v)) smoke_frames = atoi(v); /* E02: smoke length, default 300 */
    if (pm_debug_value("pos", v, sizeof v)) { /* POSWARP: pos=<x>:<y>:<z>[:<angle>] */
        pos_n = pm_pos_parse(v, pos_v);
        pos_on = pos_n != 0;
        if (!pos_on) pm_log_w("poswarp: bad value '%s', need x:y:z or x:y:z:angle", v);
    }
    return has_preset || has_warp || has_story || opt_map;
}

static void warp_setup(s16 areaID, s16 mapID);
static void pm_warp_stat(const char* tag);

/* Called from the set_game_mode wrapper. Returns the mode to enter (the caller then calls the real set_game_mode). */
s16 pm_warp_hook(s16 mode) {
    s16 areaID, mapID;
    if (mode == GAME_MODE_LOGOS && pm_debug_has("fileselect")) { /* debug switch fileselect: the boot goes straight to the file select (no title, no pad script) */
        pm_log("fileselect: direct entry from the logos");
        return GAME_MODE_FILE_SELECT;
    }
    if (warp_done || mode != GAME_MODE_LOGOS) return mode;
    warp_done = 1;
    if (!pm_warp_active()) return mode;
    if (!get_map_IDs_by_name(w_map, &areaID, &mapID)) {
        pm_log_w("warp: map %s not found, normal start", w_map);
        return mode;
    }
    warp_setup(areaID, mapID);
    if (pos_on) pend_arm(areaID, mapID, pos_v, pos_n); /* POSWARP: pos= is the boot warp's position */
    return GAME_MODE_ENTER_WORLD;
}

static void warp_setup(s16 areaID, s16 mapID) {
    if (w_loadslot >= 0) { /* E02: load the file as the file select does (name, save count), then override map and story */
        fio_fetch_saved_file_info();
        pm_log("warp: loadslot %d %s", w_loadslot, fio_load_game(w_loadslot) ? "loaded" : "FAILED");
        pm_warp_stat("warpstat loaded");
    }
    if (w_savefile[0]) {
        sf_ok = pm_load_savefile(w_savefile);
        pm_log("warp: savefile %s %s", w_savefile, sf_ok ? "loaded" : "FAILED");
        pm_warp_stat("warpstat savefile");
    }
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)w_entry;
    warp_nofile = w_loadslot < 0; /* CHEATSAVE: a new-file state has no save slot file */
    if (w_loadslot < 0 && !sf_ok) clear_player_data();
    if (w_loadslot < 0 && !sf_ok) clear_saved_variables();
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)w_entry;
    if ((w_loadslot < 0 && !sf_ok) || w_story_set) evt_set_variable(NULL, GB_StoryProgress, w_story); /* WARPFIX: a loaded save keeps its story */
    if (w_flag && w_loadslot < 0 && !sf_ok) evt_set_variable(NULL, w_flag, 1);
    if (w_partner > 0 && w_partner < 12 && w_loadslot < 0 && !sf_ok) { /* slice F02: a map script that calls EnablePartnerAI needs a partner (WARPFIX: a loaded save keeps its partners) */
        gPlayerData.partners[w_partner].enabled = 1;
        gPlayerData.curPartner = w_partner;
    }
    char vs[16];
    if (pm_debug_has("allpartners")) { void pm_dbgg_give(int g); pm_dbgg_give(0); } /* PAUSEFLK: all 11 partners (the pause Party wheel with the user's partner count) */
    if (pm_debug_value("spmax", vs, sizeof vs)) gPlayerData.maxStarPower = (s8)atoi(vs); /* PAUSEFLK: spmax=<n> also at the warp (rescued Star Spirits in the pause) */
    if (pm_debug_value("sp", vs, sizeof vs)) gPlayerData.starPoints = (s8)atoi(vs); /* slice S19d: sp=<n> star points, 99 + one win = level up */
    if (pm_debug_value("hp", vs, sizeof vs)) gPlayerData.curHP = (s8)atoi(vs); /* slice S19d: hp=<n> Mario HP, 1 = lose the battle fast */
    if (pm_debug_value("coins", vs, sizeof vs)) gPlayerData.coins = (s16)atoi(vs); /* HUDTRAIL: coins=<n> coin count at the warp (999 = the 3-digit status bar number) */
    fio_serialize_state();
    warp_on = 1;
    pm_log("warp: %s entry %d (area %d map %d) story %d flag %x", w_map, w_entry, areaID, mapID, w_story, w_flag);
}

int pm_warp_preset_count(void) { return (int)ARRAY_COUNT(presets); }

/* PT01b: test maps (area_tst, ids tst_*) are cut content. tst_12 fills the 96 effect slots and crashed the N3DS (docs/plans/n3crash.md, Q-F10 option B).
 * The list of maps for the Warp-to option and the options.ini reader do not offer them, and a saved or switch value is ignored. */
int pm_warp_is_test_map(const char* id) { return id && strncmp(id, "tst_", 4) == 0; }

/* PT01: every map of gAreas as one flat list (area order, map order), without the test maps. */
int pm_warp_map_count(void) {
    int i, j, n = 0;
    for (i = 0; gAreas[i].maps != NULL; i++)
        for (j = 0; j < gAreas[i].mapCount; j++) if (!pm_warp_is_test_map(gAreas[i].maps[j].id)) n++;
    return n;
}
const char* pm_warp_map_name(int k) {
    int i, j;
    if (k < 0) return NULL;
    for (i = 0; gAreas[i].maps != NULL; i++)
        for (j = 0; j < gAreas[i].mapCount; j++) {
            if (pm_warp_is_test_map(gAreas[i].maps[j].id)) continue;
            if (k-- == 0) return gAreas[i].maps[j].id;
        }
    return NULL;
}
int pm_warp_map_index(const char* name) {
    int i, j, n = 0;
    for (i = 0; gAreas[i].maps != NULL; i++) {
        for (j = 0; j < gAreas[i].mapCount; j++) {
            if (pm_warp_is_test_map(gAreas[i].maps[j].id)) continue;
            if (strcmp(gAreas[i].maps[j].id, name) == 0) return n;
            n++;
        }
    }
    return -1;
}
/* A warp choice: 1 to P = preset i+1, P+1 and up = map k = choice - P - 1 (pm_warp_map_name). */
const char* pm_warp_preset_name(int i) { return presets[i].name; }
const char* pm_warp_preset_map(int i) { return presets[i].map; }

/* MN6 rest: the Warp area folder. An area is a gAreas entry with at least one non-test map (gAreas order). The row name is the English
 * area name (area_name, same order as gAreas: world.c comments). Its maps are listed in map order with their flat map index (pm_warp_map_name).
 * Built on the first call, after the switches are read (warptest). Ceilings: 32 areas, WG_MAPS maps. */
static const char* const area_name[] = {
    "Goomba Village", "Toad Town", "Toad Town Tunnels", "Inside the Whale", "Peach's Castle", "Shooting Star Summit", "Koopa Village",
    "Koopa Bros. Fortress", "Mt. Rugged", "Dry Dry Outpost", "Dry Dry Desert", "Dry Dry Ruins", "Forever Forest", "Boo's Mansion",
    "Gusty Gulch", "Tubba Blubba's Castle", "Shy Guy's Toy Box", "Jade Jungle", "Mt. Lavalava", "Flower Fields", "Shiver City",
    "Crystal Palace", "Bowser's Castle", "Outside Peach's Castle", "Ending", "Minigames", "Game Over", "Test map",
};
#define WG_MAPS 512 /* ceiling of the flat map list (area folder, entry memory) */
#define WA_MAX 32
#define WA_GA 40 /* ceiling of gAreas entries */
/* STORYMENU: the area rows in story order (gAreas indices, the same index as area_name). The areas not listed here follow in gAreas order
 * (Ending, Minigames, Game Over, Test map). The presets sit between the story areas and the other areas (pm_warp_top_*). */
static const int story_area[] = { 0, 5, 1, 2, 23, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 3, 17, 18, 19, 20, 21, 22 };
static int wa_n = -1, wa_s, wa_area[WA_MAX], wa_off[WA_MAX], wa_size[WA_MAX], wa_k[WG_MAPS];
/* adds gAreas entry i with flat map indices from base (test maps skipped) */
static void wa_add(int i, int base, int* n, int* cnt) {
    int j, c0 = *cnt, k = base;
    for (j = 0; j < gAreas[i].mapCount; j++) {
        if (pm_warp_is_test_map(gAreas[i].maps[j].id)) continue;
        if (*cnt < WG_MAPS) wa_k[(*cnt)++] = k;
        else pm_log_w("warp: map list full, %s has no row", gAreas[i].maps[j].id);
        k++;
    }
    if (*cnt == c0) return;
    if (*n == WA_MAX) { pm_log_w("warp: area list full, area %d has no row", i); return; }
    wa_area[*n] = i; wa_off[*n] = c0; wa_size[*n] = *cnt - c0; (*n)++;
}
static void wa_build(void) {
    int ga_base[WA_GA], placed[WA_GA] = { 0 }, ng = 0, flat = 0, i, j, s, n = 0, cnt = 0;
    for (ng = 0; gAreas[ng].maps != NULL && ng < WA_GA; ng++) {
        ga_base[ng] = flat;
        for (j = 0; j < gAreas[ng].mapCount; j++) if (!pm_warp_is_test_map(gAreas[ng].maps[j].id)) flat++;
    }
    for (s = 0; s < (int)ARRAY_COUNT(story_area); s++) {
        i = story_area[s];
        if (i < ng && !placed[i]) { placed[i] = 1; wa_add(i, ga_base[i], &n, &cnt); }
    }
    wa_s = n;
    for (i = 0; i < ng; i++) if (!placed[i]) wa_add(i, ga_base[i], &n, &cnt);
    wa_n = n;
}
static void wa_ready(void) { if (wa_n < 0) wa_build(); }
int pm_warp_area_count(void) { wa_ready(); return wa_n; }
const char* pm_warp_area_name(int a) { wa_ready(); return wa_area[a] < (int)ARRAY_COUNT(area_name) ? area_name[wa_area[a]] : ""; }
int pm_warp_area_size(int a) { wa_ready(); return wa_size[a]; }
/* the flat map index k of map j of area a (choice = preset count + 1 + k) */
int pm_warp_area_map(int a, int j) { wa_ready(); return wa_k[wa_off[a] + j]; }
/* STORYMENU: the top rows of the Warp area folder, in order: the story areas (rows 0 to wa_s - 1), the presets, then the other areas. */
int pm_warp_top_count(void) { wa_ready(); return wa_n + (int)ARRAY_COUNT(presets); }
/* the preset index of top row r, -1 when r is not a preset row */
int pm_warp_top_preset(int r) {
    int P = (int)ARRAY_COUNT(presets);
    wa_ready();
    return r >= wa_s && r < wa_s + P ? r - wa_s : -1;
}
/* the area index of top row r, -1 when r is a preset row or out of range */
int pm_warp_top_area(int r) {
    int P = (int)ARRAY_COUNT(presets);
    if (r < 0 || r >= pm_warp_top_count()) return -1;
    if (r < wa_s) return r;
    if (r < wa_s + P) return -1;
    return r - P;
}
/* the top row of area a */
int pm_warp_top_row_area(int a) { wa_ready(); return a < wa_s ? a : a + (int)ARRAY_COUNT(presets); }

/* the MapConfig of flat map index k (test maps skipped), NULL when k is out of range */
static const MapConfig* wa_cfg(int k) {
    int i, j, n = 0;
    for (i = 0; gAreas[i].maps != NULL; i++)
        for (j = 0; j < gAreas[i].mapCount; j++) {
            if (pm_warp_is_test_map(gAreas[i].maps[j].id)) continue;
            if (n++ == k) return &gAreas[i].maps[j];
        }
    return NULL;
}
/* the entry count of map k (MapSettings.entryCount, static data). At least 1: a map without settings warps to entry 0. */
int pm_warp_map_entries(int k) {
    const MapConfig* m = wa_cfg(k);
    if (!m || !m->settings || m->settings->entryCount < 1) return 1;
    return m->settings->entryCount;
}
/* MN6 rest: the entry chosen on the area page for map k, kept in RAM (not saved). */
static unsigned char wm_ent[WG_MAPS];
int pm_warp_entry_get(int k) { return k >= 0 && k < WG_MAPS ? wm_ent[k] : 0; }
void pm_warp_entry_set(int k, int e) { if (k >= 0 && k < WG_MAPS) wm_ent[k] = (unsigned char)e; }
/* UI1a: Warp now from the Options panel of the file select. The state is set as a file load does it, the caller leaves the file select
 * through the normal exit (pm_warp_pending makes filemenu_get_exit_mode return 1: curtain, cleanup, ENTER_WORLD). */
int pm_warp_pending;
void pm_warp_now(int c, int entry) {
    s16 areaID, mapID;
    int P = (int)ARRAY_COUNT(presets);
    if (c >= 1 && c <= P) {
        const WarpPreset* pr = &presets[c - 1];
        w_story = pr->story;
        w_map[0] = 0;
        strncat(w_map, pr->map, sizeof w_map - 1);
        w_entry = pr->entry;
        w_flag = pr->flag;
    } else {
        const char* nm = pm_warp_map_name(c - 1 - P);
        if (!nm) return;
        w_story = STORY_CH0_ARRIVED_AT_TOAD_TOWN;
        w_map[0] = 0;
        strncat(w_map, nm, sizeof w_map - 1);
        w_entry = entry;
        w_flag = 0;
    }
    w_loadslot = -1;
    if (strncmp(w_map, "kmr_", 4) != 0 && w_partner == 0) w_partner = 1;
    if (!get_map_IDs_by_name(w_map, &areaID, &mapID)) {
        pm_log_w("warp: map %s not found", w_map);
        pm_menu_msg("Warp failed: map %s not found", w_map);
        return;
    }
    pm_warp_stat("warpstat before fs warp");
    warp_setup(areaID, mapID);
    pm_warp_stat("warpstat after fs warp");
    pm_warp_pending = 1;
}

/* PT63: warp from the Select menu in any game mode that has a world to leave. A map change from the world, ENTER_WORLD from the title or the intro
 * (as pm_warp_hook does at boot). The state is set as a preset sets it (new player data at the story value). Nothing is written to disk. */
/* PT74: a warp or a boss launch from the Select menu while a battle runs. The battle is ended through the real BATTLE_STATE_END_BATTLE (the way
 * battlelist ends one), the game returns to the world, then the action runs. The state does not need to be exact. 0 none, 1 warp, 2 boss.
 * MENUWARP: the start returns 1 (started), so the caller closes the Select menu as a normal warp or launch does (pm_optui_act result 5). */
static int bypass_kind, bypass_a, bypass_b;
static unsigned bypass_n;
static int bypass_begin(int kind, int a, int b) {
    if (bypass_kind) { pm_menu_msg("A battle end is already running"); return 0; }
    bypass_kind = kind; bypass_a = a; bypass_b = b; bypass_n = 0;
    if (get_game_mode() == GAME_MODE_PAUSE) set_game_mode(GAME_MODE_UNPAUSE); /* PT81: the pause menu closes the way its own close key does, then the action runs in the world */
    else btl_set_state(BATTLE_STATE_END_BATTLE);
    pm_menu_msg("Ending the battle, then %s", kind == 1 ? "warp" : "boss");
    pm_log("bypass: battle end for kind %d (%d, %d)", kind, a, b);
    return 1;
}
extern void pm_menu_set(int on);
static int pm_boss_launch_now(int i);
static int pm_warp_live_now(int c, int entry);
/* PT80: the intro (stage curtain, subtitle callback, screen overlay, message boxes) stays alive when a warp leaves it through ENTER_WORLD. End it as the
 * tour does between maps: curtain open and its draw callback off, overlays and message printers cleared. */
static void warp_end_intro(int mode) {
    /* PT81: stop the song and the ambient sounds of the scene that is left. _bgm_set_song ignores every request while demoState is set (the title demo
     * keeps it), so the destination song never started after a warp from the demo. The new map starts its own song with SetMusic. */
    bgm_set_song(0, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL);
    bgm_set_song(1, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL);
    sfx_stop_env_sounds();
    play_ambient_sounds(-1, 0); /* AMBWARP: the MSEQ ambient of the left map (a warp may land on a map that does not clear it) */
    pm_log("warp: songs, loop sounds and ambient stopped, demoState was %d, game mode %d", (int)gGameStatusPtr->demoState, mode);
    gGameStatusPtr->demoState = DEMO_STATE_NONE;
    if (mode != GAME_MODE_INTRO && mode != GAME_MODE_TITLE_SCREEN) return;
    set_curtain_scale_goal(2.0f);
    set_curtain_draw_callback(NULL);
    clear_screen_overlays();
    clear_printers();
    gGameStatusPtr->introPart = INTRO_PART_NONE;
    pm_log("warp: intro or title state ended (curtain, overlays, message printers) from mode %d", mode);
}
static void bypass_tick(void) {
    if (!bypass_kind) return;
    if (get_game_mode() == GAME_MODE_WORLD) {
        int k = bypass_kind, a = bypass_a, b = bypass_b, ok;
        bypass_kind = 0;
        ok = k == 1 ? pm_warp_live_now(a, b) : pm_boss_launch_now(a);
        if (ok) pm_menu_set(0);
    } else if (++bypass_n > 900) {
        bypass_kind = 0;
        pm_menu_msg("The battle did not end, mode %d", (int)get_game_mode());
    }
}
/* WARPQ: a warp or a boss launch asked for during a map change (game mode 5), an entry (7), a battle end (9) or an unpause (11) is held in one
 * slot. A newer request replaces it. wq_tick runs it in the world (game mode 4) with the function the menu calls. The start returns 1, so the
 * Select menu closes at once (pm_optui_act result 5). Other modes keep the refusal. */
enum { WQ_NONE, WQ_WARP, WQ_BOSS };
static int wq_kind, wq_a, wq_b;
static unsigned wq_n;
static int wq_queueable(int mode) {
    return mode == GAME_MODE_CHANGE_MAP || mode == GAME_MODE_ENTER_WORLD || mode == GAME_MODE_END_BATTLE || mode == GAME_MODE_UNPAUSE;
}
static int wq_begin(int kind, int a, int b, int mode) {
    wq_kind = kind; wq_a = a; wq_b = b; wq_n = 0;
    pm_log("warp: queued in game mode %d (kind %d, %d, %d)", mode, kind, a, b);
    pm_menu_msg("Warp queued until the map change ends");
    return 1;
}
static void wq_tick(void) {
    int mode = get_game_mode();
    if (!wq_kind) return;
    if (mode == GAME_MODE_WORLD) {
        int k = wq_kind, a = wq_a, b = wq_b;
        wq_kind = WQ_NONE;
        pm_log("warp: queued warp runs (kind %d, %d, %d)", k, a, b);
        if (k == WQ_WARP) pm_warp_live_now(a, b);
        else pm_boss_launch_now(a);
    } else if (!wq_queueable(mode) || ++wq_n > 900) {
        wq_kind = WQ_NONE;
        pm_log_w("warp: queued warp dropped in game mode %d after %u frames", mode, wq_n);
        pm_menu_msg("Queued warp dropped in game mode %d", mode);
    }
}
/* WARPFIX: the map and entry of a choice (preset or map list) for a live warp. Sets gGameStatusPtr area, map and entry only. */
static int warp_live_move(int c, int entry) {
    int P = (int)ARRAY_COUNT(presets);
    const char* nm;
    s16 areaID, mapID;
    if (c >= 1 && c <= P) {
        nm = presets[c - 1].map;
        entry = presets[c - 1].entry;
    } else {
        nm = pm_warp_map_name(c - 1 - P);
    }
    if (!nm || !get_map_IDs_by_name(nm, &areaID, &mapID)) {
        pm_log_w("warp: live map not found (choice %d)", c);
        pm_menu_msg("Warp failed: map not found");
        return 0;
    }
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)entry;
    return 1;
}

/* WARPFIX: one log line with the player data that a warp must keep (save slot, items, badges, coins, story progress, map). */
static void pm_warp_stat(const char* tag) {
    PlayerData* pd = &gPlayerData;
    int i, items = 0, badges = 0;
    for (i = 0; i < ARRAY_COUNT(pd->invItems); i++) if (pd->invItems[i] != ITEM_NONE) items++;
    for (i = 0; i < ARRAY_COUNT(pd->badges); i++) if (pd->badges[i] != ITEM_NONE) badges++;
    pm_log("%s: slot %d map %d:%d entry %d items %d badges %d coins %d story %d", tag, (int)gGameStatusPtr->saveSlot,
           (int)gGameStatusPtr->areaID, (int)gGameStatusPtr->mapID, (int)gGameStatusPtr->entryID, items, badges, (int)pd->coins,
           (int)evt_get_variable(NULL, GB_StoryProgress));
    pm_log("%s hp: HP %d/%d FP %d/%d BP max %d level %d", tag, (int)pd->curHP, (int)pd->curMaxHP, (int)pd->curFP, (int)pd->curMaxFP,
           (int)pd->maxBP, (int)pd->level);
}

void pm_save_prompt(void) {
    pm_log("save prompt: saving enabled"); /* SAVEFIX: saving is never blocked */
}

int pm_warp_live(int c, int entry) {
    if (get_game_mode() == GAME_MODE_BATTLE || get_game_mode() == GAME_MODE_PAUSE) return bypass_begin(1, c, entry);
    return pm_warp_live_now(c, entry);
}
static int pm_warp_live_now(int c, int entry) {
    int mode = get_game_mode();
    int ok = mode == GAME_MODE_WORLD || mode == GAME_MODE_TITLE_SCREEN || mode == GAME_MODE_INTRO || mode == GAME_MODE_DEMO;
    if (!ok && wq_queueable(mode)) return wq_begin(WQ_WARP, c, entry, mode);
    if (!ok) {
        pm_log_w("warp: not possible in game mode %d", mode);
        pm_menu_msg("Warp not possible in game mode %d", mode);
        return 0;
    }
    pm_warp_pending = 0;
    if (mode == GAME_MODE_WORLD) { /* WARPFIX: in the world a warp changes the map and the entry only (no player data, no story, no save slot) */
        pm_warp_stat("warpstat before live");
        if (!warp_live_move(c, entry)) return 0;
        pm_warp_stat("warpstat after live");
        pm_warp_pending = 1;
    } else {
        pm_warp_now(c, entry);
    }
    if (!pm_warp_pending) return 0;
    warp_end_intro(mode);
    pm_warp_pending = 0;
    if (mode == GAME_MODE_WORLD) {
        set_map_transition_effect(TRANSITION_STANDARD);
        set_game_mode(GAME_MODE_CHANGE_MAP);
    } else {
        set_game_mode(GAME_MODE_ENTER_WORLD);
    }
    pm_log("warp: live to choice %d entry %d from mode %d", c, entry, mode);
    return 1;
}

/* Boss tab of the Select menu: a manual launcher (the player fights). Warp to mac_00 entry 4 with the story value of the area, 30 world frames
 * later start the battle as the debug switch `battle=` does. Mario gets 75 HP and 30 FP so the fight can be tried. Nothing is written to disk. */
static s32 battle_name_value(const char* s, unsigned n, s32 dflt);
static const struct { const char* label; const char* battle; s32 story; } boss_tab[] = {
#include "pm_boss_table.h"
};
static int boss_pending = -1;
static unsigned boss_frames;
/* BT1: the damage popup slots of the battle (src/battle/190B20.c) outlive a battle that a debug path ends early (bossend, battletour, the
 * Select menu bypass): BATTLE_STATE_END_BATTLE frees every effect (remove_all_effects), but a popup younger than its 40 frames keeps its slot.
 * update_damage_popups of the next battle then writes timeLeft = 5 through the freed effect (a wild write: Azahar unmapped Write32 5 in
 * btl_update, tour_battle3 run 2). The real game ends a battle only after the popups expired. Cleared before a debug start. */
extern struct EffectInstance* gDamageCountEffects[24];
extern s32 gDamageCountTimers[24];
static void battle_popups_clear(void) {
    memset(gDamageCountEffects, 0, sizeof(gDamageCountEffects[0]) * 24);
    memset(gDamageCountTimers, 0, sizeof(s32) * 24);
}
int pm_boss_count(void) { return (int)ARRAY_COUNT(boss_tab); }
const char* pm_boss_label(int i) { return i >= 0 && i < pm_boss_count() ? boss_tab[i].label : ""; }
int pm_boss_launch(int i) {
    if ((get_game_mode() == GAME_MODE_BATTLE || get_game_mode() == GAME_MODE_PAUSE) && i >= 0 && i < pm_boss_count()) return bypass_begin(2, i, 0);
    return pm_boss_launch_now(i);
}
static int pm_boss_launch_now(int i) {
    s16 areaID, mapID;
    int mode = get_game_mode();
    if (i < 0 || i >= pm_boss_count()) return 0;
    if (mode != GAME_MODE_WORLD && mode != GAME_MODE_TITLE_SCREEN && mode != GAME_MODE_INTRO && mode != GAME_MODE_DEMO && wq_queueable(mode)) return wq_begin(WQ_BOSS, i, 0, mode);
    if (mode != GAME_MODE_WORLD && mode != GAME_MODE_TITLE_SCREEN && mode != GAME_MODE_INTRO && mode != GAME_MODE_DEMO) {
        pm_menu_msg("Boss launch not possible in game mode %d", mode);
        return 0;
    }
    if (mode == GAME_MODE_WORLD) { /* WARPFIX2: in the world the loaded save stays. Map and entry move, the fight needs story, partner and full HP/FP: they are set here and restored after the fight. */
        if (boss_keep_on || !get_map_IDs_by_name("mac_00", &areaID, &mapID)) { pm_menu_msg("Boss launch not possible now"); return 0; }
        boss_keep_on = 1;
        bk_story = evt_get_variable(NULL, GB_StoryProgress);
        bk_hmhp = gPlayerData.hardMaxHP; bk_cmhp = gPlayerData.curMaxHP; bk_chp = gPlayerData.curHP;
        bk_hmfp = gPlayerData.hardMaxFP; bk_cmfp = gPlayerData.curMaxFP; bk_cfp = gPlayerData.curFP;
        bk_pen = gPlayerData.partners[1].enabled; bk_cur = gPlayerData.curPartner; bk_ac = gPlayerData.hasActionCommands;
        pm_warp_stat("warpstat before boss");
        gGameStatusPtr->areaID = areaID;
        gGameStatusPtr->mapID = mapID;
        gGameStatusPtr->entryID = 4;
        evt_set_variable(NULL, GB_StoryProgress, boss_tab[i].story);
        gPlayerData.partners[1].enabled = 1;
        gPlayerData.curPartner = 1;
        gPlayerData.hardMaxHP = gPlayerData.curMaxHP = gPlayerData.curHP = 75;
        gPlayerData.hardMaxFP = gPlayerData.curMaxFP = gPlayerData.curFP = 30;
        gPlayerData.hasActionCommands = true; /* BT1: every boss comes after the Goompa tutorial; without it the action commands and guards are off (AC_MODE_NOT_LEARNED) */
        warp_end_intro(mode);
        boss_pending = i;
        boss_frames = 0;
        set_map_transition_effect(TRANSITION_STANDARD);
        set_game_mode(GAME_MODE_CHANGE_MAP);
        pm_log("boss: %s (%s) in the world, the loaded save is kept and restored after the fight", boss_tab[i].label, boss_tab[i].battle);
        return 1;
    }
    w_story = boss_tab[i].story;
    w_map[0] = 0;
    strncat(w_map, "mac_00", sizeof w_map - 1);
    w_entry = 4;
    w_flag = 0;
    w_loadslot = -1;
    if (w_partner == 0) w_partner = 1;
    if (!get_map_IDs_by_name(w_map, &areaID, &mapID)) { pm_menu_msg("Boss launch failed: map mac_00 not found"); return 0; }
    warp_setup(areaID, mapID);
    gPlayerData.hardMaxHP = gPlayerData.curMaxHP = gPlayerData.curHP = 75;
    gPlayerData.hardMaxFP = gPlayerData.curMaxFP = gPlayerData.curFP = 30;
    gPlayerData.hasActionCommands = true; /* BT1: as in the world launch */
    fio_serialize_state();
    warp_end_intro(mode);
    boss_pending = i;
    boss_frames = 0;
    if (mode == GAME_MODE_WORLD) {
        set_map_transition_effect(TRANSITION_STANDARD);
        set_game_mode(GAME_MODE_CHANGE_MAP);
    } else {
        set_game_mode(GAME_MODE_ENTER_WORLD);
    }
    pm_log("boss: %s (%s) story %d", boss_tab[i].label, boss_tab[i].battle, (int)boss_tab[i].story);
    return 1;
}
static void boss_tick(void) {
    char v[96];
    const char* c;
    s32 formation, stage;
    s16 areaID, mapID;
    if (boss_pending < 0) return;
    if (get_game_mode() != GAME_MODE_WORLD || !get_map_IDs_by_name("mac_00", &areaID, &mapID) || gGameStatusPtr->mapID != mapID || gGameStatusPtr->areaID != areaID) { boss_frames = 0; return; }
    if (gEncounterState != ENCOUNTER_STATE_NEUTRAL || (gPlayerStatus.flags & PS_FLAG_INPUT_DISABLED)) { boss_frames = 0; return; } /* BTLDBG: wait while a talk, an encounter or a script holds the world */
    if (++boss_frames < 30) return;
    v[0] = 0;
    strncat(v, boss_tab[boss_pending].battle, sizeof v - 1);
    c = strchr(v, ':');
    formation = battle_name_value(v, c ? (unsigned)(c - v) : (unsigned)strlen(v), BTL_KMR_1_FORMATION_00);
    stage = c ? battle_name_value(c + 1, (unsigned)strlen(c + 1), BTL_KMR_1_STAGE_01) : -1; /* BT1: a row without a stage part: the battle's own stage (load_battle_section, gCurrentStagePtr NULL) */
    pm_log("boss: start battle formation %d stage %d", (int)formation, (int)stage);
    boss_pending = -1;
    battle_popups_clear();
    gEncounterState = ENCOUNTER_STATE_NONE;
    set_battle_stage(stage);
    load_battle(formation);
    if (boss_keep_on) { boss_fight = 1; boss_seen_battle = 0; boss_bf = 0; boss_wf = 0; }
}

/* BT2: the Battles folder (Boss and Battles > Battles > area > formation). Every formation of pm_battle_table.h (BT1). A start in the world
 * is the battle switch of the debug switch battle= (set_battle_stage, load_battle), taken 30 world frames after the menu closes. No warp,
 * no story change. Other modes: a message (named gap). */
static const struct { const char* label; const char* battle; const char* area; } battle_tab[] = {
#include "pm_battle_table.h"
};
static int bt_pending = -1;
static unsigned bt_frames;
int pm_battle_count(void) { return (int)ARRAY_COUNT(battle_tab); }
const char* pm_battle_label(int k) { return k >= 0 && k < pm_battle_count() ? battle_tab[k].label : ""; }
/* the areas, in table order: the distinct area names */
int pm_battle_area_count(void) {
    int k, n = 0;
    for (k = 0; k < pm_battle_count(); k++) if (k == 0 || strcmp(battle_tab[k].area, battle_tab[k - 1].area) != 0) n++;
    return n;
}
static int battle_area_start(int a) {
    int k, n = -1;
    for (k = 0; k < pm_battle_count(); k++) if (k == 0 || strcmp(battle_tab[k].area, battle_tab[k - 1].area) != 0) { if (++n == a) return k; }
    return -1;
}
const char* pm_battle_area_name(int a) {
    static char nb[24];
    int k = battle_area_start(a), i;
    if (k < 0) return "";
    snprintf(nb, sizeof nb, "%s", battle_tab[k].area);
    for (i = 0; nb[i]; i++) if (nb[i] >= 'a' && nb[i] <= 'z') nb[i] = (char)(nb[i] - 32);
    return nb;
}
/* formations of area a: the count, and the table index of row r */
int pm_battle_area_rows(int a) {
    int k = battle_area_start(a), n = 0;
    if (k < 0) return 0;
    for (; k < pm_battle_count() && strcmp(battle_tab[k].area, battle_tab[battle_area_start(a)].area) == 0; k++) n++;
    return n;
}
int pm_battle_area_item(int a, int r) {
    int k = battle_area_start(a);
    return k < 0 ? -1 : k + r;
}
/* the start: in the world, 30 frames later (battle_tick). Returns 1 when it is queued. */
int pm_battle_launch(int k) {
    if (get_game_mode() != GAME_MODE_WORLD) { pm_menu_msg("Battle start needs the world"); return 0; }
    if (k < 0 || k >= pm_battle_count()) return 0;
    bt_pending = k;
    bt_frames = 0;
    return 1;
}
/* BT3: the HUD line of a battle: "Battle <AREA> F<formation> S<stage> <label>" from the table row of the formation and stage (label and area),
 * or "Battle F.. S.." when the table has no row. */
const char* pm_battle_hud_text(int formation, int stage, char* out, unsigned n) {
    int k;
    for (k = 0; k < pm_battle_count(); k++) {
        char v[64], *c;
        s32 f, st;
        v[0] = 0;
        strncat(v, battle_tab[k].battle, sizeof v - 1);
        c = strchr(v, ':');
        f = battle_name_value(v, c ? (unsigned)(c - v) : (unsigned)strlen(v), -1);
        st = battle_name_value(c ? c + 1 : "", c ? (unsigned)strlen(c + 1) : 0, -1);
        if (f == formation && st == stage) {
            char ar[24];
            int i;
            snprintf(ar, sizeof ar, "%s", battle_tab[k].area);
            for (i = 0; ar[i]; i++) if (ar[i] >= 'a' && ar[i] <= 'z') ar[i] = (char)(ar[i] - 32);
            snprintf(out, n, "Battle %s F%02X S%02X %s", ar, (unsigned)(formation & 0xFF), (unsigned)(stage & 0xFF), battle_tab[k].label);
            return out;
        }
    }
    snprintf(out, n, "Battle F%02X S%02X", (unsigned)(formation & 0xFF), (unsigned)(stage & 0xFF));
    return out;
}
static void battle_tick(void) {
    s32 formation, stage;
    char v[64], *c;
    if (bt_pending < 0) return;
    if (get_game_mode() != GAME_MODE_WORLD) { bt_pending = -1; return; }
    if (gEncounterState != ENCOUNTER_STATE_NEUTRAL || (gPlayerStatus.flags & PS_FLAG_INPUT_DISABLED)) { bt_frames = 0; return; } /* BTLDBG: wait while a talk, an encounter or a script holds the world */
    if (++bt_frames < 30) return;
    v[0] = 0;
    strncat(v, battle_tab[bt_pending].battle, sizeof v - 1);
    c = strchr(v, ':');
    formation = battle_name_value(v, c ? (unsigned)(c - v) : (unsigned)strlen(v), BTL_KMR_1_FORMATION_00);
    stage = c ? battle_name_value(c + 1, (unsigned)strlen(c + 1), BTL_KMR_1_STAGE_01) : -1; /* BT1: no stage part: the battle's own stage */
    pm_log("battle: start formation %d stage %d", (int)formation, (int)stage);
    bt_pending = -1;
    battle_popups_clear();
    gEncounterState = ENCOUNTER_STATE_NONE;
    set_battle_stage(stage);
    if (pm_debug_has("acmd")) gPlayerData.hasActionCommands = true; /* test: action command prompts without the Lucky Star */
    load_battle(formation);
}

/* WARPFIX2: the fight is over when the world is back after the battle. Then the saved story, HP/FP and partner come back. Debug switch
 * bossend=<n>: the battle ends by the real BATTLE_STATE_END_BATTLE after n battle frames (test only; a player ends the fight in the battle). */
/* BT1: the end of a boss fight by the real BATTLE_STATE_END_BATTLE (bossend switch and the battle tour). */
static void boss_end(const char* why) {
    int w;
    /* The battle menu windows (moves, strategies, description, popup) close by their own menu code before a real battle end. A forced end
     * can come with one open: the end's set_windows_visible(WINDOW_GROUP_ALL) then shows it in the world, and its draw (btl_menu_moves_draw_content)
     * reads menu hud elements that are gone (tour_battle run 32696: unmapped null reads and writes). Clear them as clear_windows does. */
    for (w = WIN_BTL_MOVES_MENU; w <= WIN_BTL_POPUP; w++) gWindows[w].flags = 0;
    btl_set_state(BATTLE_STATE_END_BATTLE);
    pm_log("boss: %s, the battle ends now", why);
}
/* The saved story, HP/FP, partner and action commands of a world launch come back. */
static void boss_restore(void) {
    pm_warp_stat("warpstat before boss restore");
    evt_set_variable(NULL, GB_StoryProgress, bk_story);
    gPlayerData.hardMaxHP = bk_hmhp; gPlayerData.curMaxHP = bk_cmhp; gPlayerData.curHP = bk_chp;
    gPlayerData.hardMaxFP = bk_hmfp; gPlayerData.curMaxFP = bk_cmfp; gPlayerData.curFP = bk_cfp;
    gPlayerData.partners[1].enabled = bk_pen; gPlayerData.curPartner = bk_cur; gPlayerData.hasActionCommands = bk_ac;
    boss_fight = 0;
    boss_keep_on = 0;
    pm_warp_stat("warpstat after boss restore");
}
static void boss_restore_tick(void) {
    char bv[12];
    if (!boss_fight) return;
    if (get_game_mode() == GAME_MODE_BATTLE) {
        boss_seen_battle = 1;
        if (pm_debug_value("bossend", bv, sizeof bv) && (unsigned)atoi(bv) == ++boss_bf) boss_end("bossend switch");
        return;
    }
    if (!boss_seen_battle || get_game_mode() != GAME_MODE_WORLD || ++boss_wf < 10) return;
    boss_restore();
}

/* PT66: Return to Title Screen. The game own soft reset: the Mario overlay fades in, the game sounds stop, then GAME_MODE_STARTUP runs
 * the engine setup again and the logos lead to the title. The flash is flushed and the audio drained first, as for the HOME menu. */
extern s16 SoftResetOverlayAlpha, SoftResetState;
void pm_title_return(void) {
    void pm_lg_quiesce(void); /* pm_logic.c */
    pm_lg_quiesce(); /* SAVESAFE: the logic writes the save pages, so the join runs after its step boundary */
    pm_flash_slots_line("title return before flush"); /* SAVETITLE log */
    pm_flash_flush("return to title");
    pm_flash_slots_line("title return after flush"); /* SAVETITLE log */
    pm_audio_drain(3);
    SoftResetOverlayAlpha = 0;
    SoftResetState = 0;
    gOverrideFlags |= GLOBAL_OVERRIDES_SOFT_RESET;
    pm_log("title: soft reset requested");
}

void pm_padscript_start(void);
extern unsigned pm_pad_scans;

/* MN7: the position log line of the L+R+Y+B combo (pm_input.c): map id, area, map, entry, x y z, yaw of Mario. Level warn, so the console shows it. */
void pm_poslog_line(void) {
    PlayerStatus* ps = &gPlayerStatus;
    int a = gGameStatusPtr->areaID, m = gGameStatusPtr->mapID;
    pm_log_w("pos: %s area %d map %d entry %d x %.3f y %.3f z %.3f yaw %.1f", gAreas[a].maps[m].id, a, m, (int)gGameStatusPtr->entryID, ps->pos.x, ps->pos.y, ps->pos.z, ps->curYaw);
}

/* The game reaches GAME_MODE_WORLD: first frame of the chosen scene. */
void pm_warp_world_mode(unsigned frame) {
    static int ac = -1; /* AMBWARP: debug switch ambchk, read once: logs the ambient stop state on the map the warp lands on */
    if (ac < 0) ac = pm_debug_has("ambchk");
    if (ac) {
        AuResult rc = snd_ambient_is_stopped(0); /* AMBWARP: no ambient player at all (numActivePlayers 0) gives AU_AMBIENCE_ERROR_3 and is silent too */
        pm_log_evt("ambchk f%u map %s stopped %d", frame, gAreas[gGameStatusPtr->areaID].maps[gGameStatusPtr->mapID].id, rc == AU_RESULT_OK || rc == AU_AMBIENCE_ERROR_3);
    }
    if (pend_on && gGameStatusPtr->areaID == pend_area && gGameStatusPtr->mapID == pend_map) { /* POSWARP: the load a warp armed, on its map only (a door or pipe does not arm it) */
        pend_on = 0;
        pos_n = pend_n; memcpy(pos_v, pend_v, sizeof pos_v); pos_on = 1;
        pos_area = pend_area; pos_map = pend_map;
        pos_tour = pend_tour; pend_tour = 0;
        pos_f0 = frame;
        pos_state = 0;
    }
    if (tour_next && warp_on && tour_st != TS_MEASURE) {
        tour_f0 = frame; /* a map of the tour is in: SETTLE (again, when a script changed the map during SETTLE) */
        tour_st = TS_SETTLE;
        { /* NDT3: tourshot=<k> takes one shot k frames after each tour map is in: one start, one shot per scene */
            static int tsk = -2; char tsv[16];
            extern void pm_gpu_shot_window(unsigned from, unsigned n);
            if (tsk == -2) tsk = pm_debug_value("tourshot", tsv, sizeof tsv) ? atoi(tsv) : -1;
            if (tsk >= 0) pm_gpu_shot_window(frame + (unsigned)tsk, 1);
        }
        struct mallinfo mi = mallinfo(); /* game heap at the scene start: used KB, and free KB = heap total minus used (free-in-arena alone misses the sbrk part) */
        unsigned heap_used = (unsigned)mi.uordblks, heap_total = pm_heap_bytes();
        pm_log_d("tour: in map area %d map %d entry %d frame %u t=%u ms heap used %u KB free %u KB", gGameStatusPtr->areaID, gGameStatusPtr->mapID, gGameStatusPtr->entryID, frame, pm_uptime_ms(), heap_used >> 10, (heap_total - heap_used) >> 10);
    }
    if (!warp_on || scene_f0 >= 0) return;
    scene_f0 = (int)frame;
    pm_log_d("warp: scene frame0 %u scan %u t=%u ms", frame, pm_pad_scans, pm_uptime_ms());
    pm_padscript_start();
}

static void tour_step(void) {
    char name[16];
    s16 areaID, mapID;
    int entry = 0, tpn = 0;
    float tpos[4];
    unsigned i = 0;
    while (*tour_next == ',') tour_next++;
    if (!*tour_next) {
        { extern void pm_gpu_ndn_window_end(void); pm_gpu_ndn_window_end(); } /* MDL-5e debug: the fallback window of the tour */
        { extern void pm_ph_tour_mode(int on); pm_ph_tour_mode(0); }
        pm_log_evt("tour: done items %d", tour_n); /* test-event line: passes the gate in prof mode */
        tour_next = NULL;
        return;
    }
    while (*tour_next && *tour_next != ':' && *tour_next != ',' && i + 1 < sizeof name) name[i++] = *tour_next++;
    name[i] = 0;
    if (*tour_next == ':') { entry = atoi(++tour_next); while (*tour_next == '-' || (*tour_next >= '0' && *tour_next <= '9')) tour_next++; } /* TOUR1: past the number, so an @x:y:z after it is read */
    if (*tour_next == '@') { /* POSWARP: map:entry@x:y:z puts Mario at that point on this map */
        char pv[40]; unsigned j = 0;
        tour_next++;
        while (*tour_next && *tour_next != ',' && j + 1 < sizeof pv) pv[j++] = *tour_next++;
        pv[j] = 0;
        tpn = pm_pos_parse(pv, tpos);
    }
    while (*tour_next && *tour_next != ',') tour_next++;
    if (*tour_next == ',') tour_next++;
    tour_n++;
    if (!get_map_IDs_by_name(name, &areaID, &mapID)) {
        pm_log_w("tour: map %s not in gAreas", name);
        tour_step(); /* the next item */
        return;
    }
    pm_log("tour: next %s entry %d (area %d map %d) t=%u ms", name, entry, areaID, mapID, pm_uptime_ms());
    if (tpn) { pend_arm(areaID, mapID, tpos, tpn); pend_tour = 1; } /* POSWARP: this step's position, on its map; TOUR1: y snapped from y + 200 */
    memcpy(tour_name, name, sizeof tour_name);
    tour_entry = entry;
    gRandSeed = TOUR_SEED; /* TOUR1: every warp starts from one seed, so the map init draws the same random numbers each run */
    /* F01: the tour skips the exit script of the map (kmr_23 N(func_80240010_907A40)). Do what it does, else the curtain callback runs on the next map. */
    set_curtain_scale_goal(2.0f);
    set_curtain_draw_callback(NULL);
    /* F01 nok_01: the tour skips the scripts that stop BGM player B (trd_05 plays SONG_KOOPA_BROS_INTERLUDE on it). nok_01 loads the radio MSEQ files
       into dataMSEQ[1], the same buffer as dataBGM[1] (decomp layout, engine.c), so a song left on B reads overwritten data. */
    bgm_set_song(1, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL);
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)entry;
    set_map_transition_effect(TRANSITION_STANDARD);
    set_game_mode(GAME_MODE_CHANGE_MAP);
    tour_st = TS_WAIT;
}

/* TOUR1: the tour state machine, once per step. */
static void tour_tick(unsigned frame) {
    extern void pm_ph_window_open(const char* label);
    extern void pm_ph_window_close(void);
    if (tour_st == TS_WAIT) {
        if (frame - tour_ts > 300) { /* the change was refused or never came */
            pm_log_w("tour: no map change within 300 frames, next item");
            tour_ts = frame;
            tour_step();
        }
    } else if (tour_st == TS_SETTLE) {
        if (frame - tour_f0 < TOUR_SETTLE || get_game_mode() != GAME_MODE_WORLD) return;
        if (tour_n == 0) { /* the warp= map is not an item: go to the first item */
            tour_ts = frame;
            tour_step();
        } else {
            char lb[48];
            snprintf(lb, sizeof lb, "%d/%d %s:%d", tour_n, tour_total, tour_name, tour_entry);
            tour_wa = gGameStatusPtr->areaID; tour_wm = gGameStatusPtr->mapID;
            pm_ph_window_open(lb);
            pm_log_evt("tour: window %s open f%u rng %08x", lb, frame, (unsigned)gRandSeed);
            tour_f0 = frame;
            tour_st = TS_MEASURE;
        }
    } else if (tour_st == TS_MEASURE) {
        int left = get_game_mode() != GAME_MODE_WORLD || gGameStatusPtr->areaID != tour_wa || gGameStatusPtr->mapID != tour_wm;
        if (left || frame - tour_f0 >= tour_len) {
            if (left) pm_log_w("tour: scene left after %u frames, window closed early", frame - tour_f0);
            pm_ph_window_close();
            tour_ts = frame;
            tour_step();
        }
    }
}

/* BT1: boss battle tour. battletour=all|i,j,.. (indices of pm_boss_table.h) runs the bosses one after the other in one session, after warp= (or
 * preset=) has loaded its first map. Per boss: LAUNCH (in the world: gRandSeed = TOUR_SEED, pm_boss_launch, the world launch that keeps and
 * restores the player state) -> WAITBATTLE (GAME_MODE_BATTLE, then TOUR_SETTLE frames; no battle within BT_TIMEOUT: `battletour: <label> FAIL`,
 * next) -> PLAY (prof window, tourtime=<s>; the scenario presses A with autoa) -> END (boss_end: the real BATTLE_STATE_END_BATTLE) -> RESTORE
 * (boss_restore_tick back in the world, then 10 frames) -> CLOSE (line `battletour: <i>/<n> battle:<label> closed`) -> next LAUNCH.
 * End line `battletour: done items <n> fail <k>`. Window line `prof f<n> w<n> tour <i>/<n> battle:<label>` (spaces of the label as _). Debug tool only (class E). */
#define BT_TIMEOUT 600 /* frames, 20 s */
enum { BT_OFF, BT_START, BT_LAUNCH, BT_WAITBATTLE, BT_PLAY, BT_END, BT_RESTORE };
static unsigned char bt_idx[64];
static int bt_st, bt_n, bt_total, bt_fail, bt_cur, bt_shot = -1;
static unsigned bt_f0, bt_bf, bt_wf;
static char bt_label[48];
static void bt_parse(void) {
    char v[200];
    const char* c;
    if (!pm_debug_value("battletour", v, sizeof v)) return;
    bt_total = 0;
    if (strcmp(v, "all") == 0) {
        for (; bt_total < pm_boss_count() && bt_total < (int)sizeof bt_idx; bt_total++) bt_idx[bt_total] = (unsigned char)bt_total;
    } else {
        for (c = v; *c && bt_total < (int)sizeof bt_idx;) {
            int i = atoi(c);
            if (i >= 0 && i < pm_boss_count()) bt_idx[bt_total++] = (unsigned char)i;
            else pm_log_w("battletour: boss %d not in the table (%d bosses)", i, pm_boss_count());
            while (*c && *c != ',') c++;
            if (*c == ',') c++;
        }
    }
    if (pm_debug_value("tourtime", v, sizeof v)) { float t = strtof(v, NULL); if (t > 0.f) tour_len = (unsigned)(t * 30.f); }
    if (pm_debug_value("tourshot", v, sizeof v)) bt_shot = atoi(v); /* tourshot=<k>: one shot k frames after each battle starts (k >= TOUR_SETTLE: inside the window) */
    if (!bt_total) return;
    bt_st = BT_START;
    { extern void pm_ph_tour_mode(int on); pm_ph_tour_mode(1); }
    pm_log("battletour: %d bosses, settle %d frames, window %u frames, timeout %d frames, seed %08x", bt_total, TOUR_SETTLE, tour_len, BT_TIMEOUT, TOUR_SEED);
}
static void bt_fail_item(const char* why) {
    bt_fail++;
    pm_log_evt("battletour: %s FAIL (%s, game mode %d)", bt_label, why, (int)get_game_mode());
}
/* the next boss, or the end line */
static void bt_launch(unsigned frame) {
    if (get_game_mode() != GAME_MODE_WORLD) return; /* the launch is a world launch: wait for the world */
    if (bt_n >= bt_total) {
        { extern void pm_ph_tour_mode(int on); pm_ph_tour_mode(0); }
        pm_log_evt("battletour: done items %d fail %d", bt_n, bt_fail);
        bt_st = BT_OFF;
        return;
    }
    bt_cur = bt_idx[bt_n++];
    { /* window label `<i>/<n> battle:<boss label>` (spaces as _): the TOUR2 ingest reads `tour <i>/<n> <label>` and takes kind battle from the prefix */
        char* c;
        snprintf(bt_label, sizeof bt_label, "%d/%d battle:%s", bt_n, bt_total, pm_boss_label(bt_cur));
        for (c = strchr(bt_label, ':'); c && *c; c++) if (*c == ' ') *c = '_';
    }
    gRandSeed = TOUR_SEED; /* every launch starts from one seed */
    bt_f0 = frame;
    bt_bf = 0;
    if (!pm_boss_launch(bt_cur)) { bt_fail_item("launch refused"); return; }
    pm_log_evt("battletour: launch %s (boss %d) f%u t=%u ms", bt_label, bt_cur, frame, pm_uptime_ms());
    bt_st = BT_WAITBATTLE;
}
static void bt_tick(unsigned frame) {
    extern void pm_ph_window_open(const char* label);
    extern void pm_ph_window_close(void);
    int mode = get_game_mode();
    switch (bt_st) {
    case BT_START: /* the warp= map is not an item: TOUR_SETTLE frames in it, then the first boss */
        if (mode == GAME_MODE_WORLD && frame - (unsigned)scene_f0 >= TOUR_SETTLE) bt_st = BT_LAUNCH;
        break;
    case BT_LAUNCH:
        bt_launch(frame);
        break;
    case BT_WAITBATTLE:
        if (mode == GAME_MODE_BATTLE) {
            if (!bt_bf) {
                extern void pm_gpu_shot_window(unsigned from, unsigned n);
                bt_bf = frame;
                pm_log_evt("battletour: %s in battle f%u t=%u ms", bt_label, frame, pm_uptime_ms());
                if (bt_shot >= 0) pm_gpu_shot_window(frame + (unsigned)bt_shot, 1);
            }
            if (frame - bt_bf >= TOUR_SETTLE) {
                pm_ph_window_open(bt_label);
                pm_log_evt("battletour: window %s open f%u rng %08x", bt_label, frame, (unsigned)gRandSeed);
                bt_f0 = frame;
                bt_st = BT_PLAY;
            }
        } else if (frame - bt_f0 > BT_TIMEOUT) {
            bt_fail_item("no battle within 20 s");
            boss_pending = -1;
            if (boss_keep_on && !boss_fight) boss_restore(); /* the launch never reached the battle: its saved player state comes back */
            bt_st = BT_LAUNCH;
        }
        break;
    case BT_PLAY:
        if (mode != GAME_MODE_BATTLE || frame - bt_f0 >= tour_len) {
            if (mode != GAME_MODE_BATTLE) pm_log_w("battletour: battle left after %u frames, window closed early", frame - bt_f0);
            pm_ph_window_close();
            bt_st = BT_END;
        }
        break;
    case BT_END:
        if (mode == GAME_MODE_BATTLE) boss_end("battletour");
        bt_f0 = frame;
        bt_wf = 0;
        bt_st = BT_RESTORE;
        break;
    case BT_RESTORE: /* boss_restore_tick restores the player state 10 world frames after the battle; 10 frames more, then the next boss */
        if (!boss_fight && mode == GAME_MODE_WORLD) {
            if (++bt_wf >= 10) {
                pm_log_evt("battletour: %s closed f%u", bt_label, frame);
                bt_st = BT_LAUNCH;
            }
        } else if (frame - bt_f0 > BT_TIMEOUT) {
            bt_fail_item("no return to the world within 20 s");
            bt_st = BT_OFF;
            pm_log_evt("battletour: done items %d fail %d", bt_n, bt_fail);
        }
        break;
    }
}

/* M03: debug switches `battle` and `demobattle` (class E, no game logic), moved here from the deleted pm_title.c.
 * `battle` or `battle=<formation>:<stage>` starts a real battle through set_battle_stage and load_battle. A part is a name from
 * battle_names.h or stage_names.h (BTL_KMR_1_FORMATION_00, BTL_KMR_1_STAGE_01) or a number (0x0000, 1). The default is
 * BTL_KMR_1_FORMATION_00 and BTL_KMR_1_STAGE_01 (one Goomba on kmr_03). `battlef=<n>` is the scene frame (default 100).
 * `demobattle` enters the real demo state at scene 1 (load_demo_battle(0), dig stage, Fuzzies). Both need warp, preset or story. */
static const struct { const char* name; s32 value; } battle_names[] = {
#include "battle_name_table.h"
};

static s32 battle_name_value(const char* s, unsigned n, s32 dflt) {
    char t[48];
    unsigned i;
    if (n == 0) return dflt;
    if (n >= sizeof t) n = sizeof t - 1;
    memcpy(t, s, n);
    t[n] = 0;
    for (i = 0; i < sizeof battle_names / sizeof battle_names[0]; i++) {
        if (strcmp(t, battle_names[i].name) == 0) return battle_names[i].value;
    }
    return (s32)strtol(t, NULL, 0);
}


/* ENCCRASH: the side effects of a real touch (encounter.c update_encounters_neutral, START_BATTLE, ENCOUNTER_TRIGGER_NONE). Without them
 * the touched enemy's AI and aux scripts keep running into the battle, and the hit scripts never start. Mirror of that case; the game
 * code is not changed. */
static void encwarp_touch_effects(EncounterStatus* es, Enemy* en) {
    Encounter* encounter = es->curEncounter;
    int i;
    if (en->aiScript != NULL) suspend_all_script(en->aiScriptID);
    if (en->auxScript != NULL) suspend_all_script(en->auxScriptID);
    for (i = 0; encounter != NULL && i < encounter->count; i++) {
        Enemy* e = encounter->enemy[i];
        Evt* script;
        if (e == NULL) continue;
        if ((e->flags & ENEMY_FLAG_ENABLE_HIT_SCRIPT) && e != en) continue;
        if (e->flags & ENEMY_FLAG_DISABLE_AI) continue;
        if ((en->flags & ENEMY_FLAG_PROJECTILE) && e != en) continue;
        if (e->hitBytecode != NULL) {
            e->encountered = ENCOUNTER_TRIGGER_NONE;
            script = start_script(e->hitBytecode, EVT_PRIORITY_A, 0);
            e->hitScript = script;
            e->hitScriptID = script->id;
            script->owner1.enemy = e;
            script->owner2.npcID = e->npcID;
            script->groupFlags = e->scriptGroup;
        }
    }
    disable_player_input();
    partner_disable_input();
    if (gPlayerStatus.actionState != ACTION_STATE_TORNADO_JUMP && gPlayerStatus.actionState != ACTION_STATE_TORNADO_POUND &&
        gPlayerStatus.actionState != ACTION_STATE_SPIN_JUMP && gPlayerStatus.actionState != ACTION_STATE_SPIN_POUND) {
        gPlayerStatus.flags |= PS_FLAG_ENTERING_BATTLE;
    }
}

/* encwarp=<scene frame>: enters ENCOUNTER_STATE_PRE_BATTLE for the first world enemy of the map (the fields START_BATTLE sets), so the real
 * pre-battle update and the star transition OVERLAY_START_BATTLE run. Logs the pre-battle start and shoots the 40 frames from it
 * (pm_gpu_shot_window). Nothing else is changed. */
extern void pm_gpu_shot_window(unsigned from, unsigned n);
extern unsigned pm_frame_count(void);
static void encwarp_tick(unsigned f) {
    static int state; /* 0 wait, 1 moved, 2 shooting, 3 done or failed */
    static unsigned t_move, t0;
    char v[16];
    if (state == 0) {
        Enemy* en = NULL;
        EncounterStatus* es = &gCurrentEncounter;
        int i, j;
        if (!pm_debug_value("encwarp", v, sizeof v) || f < (unsigned)atoi(v) || get_game_mode() != GAME_MODE_WORLD) return;
        for (i = 0; i < es->numEncounters && !en; i++) {
            Encounter* e = es->encounterList[i];
            for (j = 0; e && j < e->count && !en; j++) {
                Enemy* ej = e->enemy[j];
                if (ej && !(ej->flags & ENEMY_FLAG_IGNORE_TOUCH) && !(ej->flags & ENEMY_FLAG_DISABLE_AI)) {
                    en = ej;
                    es->curEncounter = e;
                    es->curEnemy = ej;
                }
            }
        }
        if (!en) { pm_log_w("encwarp: no world enemy in the encounter list at scene frame %u", f); state = 3; return; }
        /* The touch test does not start the battle (the enemy npc stays at the sentinel y -1000, run 4). The pre-battle state is set with the
         * fields START_BATTLE sets (firstStrikeType, scriptedBattle, fadeOutAmount, substateDelay, state). Update and draw of the pre-battle
         * state (update_encounters_pre_battle, OVERLAY_START_BATTLE) are the real code. */
        /* ENCCRASH: a standing touch has hitType NONE and firstStrikeType NONE or ENEMY (encounter.c standing contact). FIRST_STRIKE_PLAYER with
         * hitType NONE has no dummy menu move (btl_states_actions.c, switch on hitType), so LoadMoveScript set a NULL move script and
         * ExecWait(NULL) crashed the battle (evt_handle_exec_wait, start_child_script). NONE is the scripted-battle path (battle.c mode 0). */
        es->firstStrikeType = FIRST_STRIKE_NONE;
        es->hitType = ENCOUNTER_TRIGGER_NONE;
        encwarp_touch_effects(es, en);
        es->scriptedBattle = false;
        es->fadeOutAmount = 0;
        es->substateDelay = 0;
        gEncounterState = ENCOUNTER_STATE_PRE_BATTLE;
        EncounterStateChanged = true;
        gEncounterSubState = ENCOUNTER_SUBSTATE_PRE_BATTLE_INIT;
        pm_log("encwarp: pre-battle entered by encwarp at scene frame %u (enemy npc %d)", f, (int)en->npcID);
        t_move = f;
        state = 1;
    } else if (state == 1) {
        if (f - t_move <= 40 && (f - t_move) % 5 == 0) { /* diag: why the touch does not start the battle */
            Enemy* en = NULL;
            Npc* nq = NULL;
            int i, j;
            for (i = 0; i < gCurrentEncounter.numEncounters && !en; i++) {
                Encounter* e = gCurrentEncounter.encounterList[i];
                for (j = 0; e && j < e->count && !en; j++) en = e->enemy[j];
            }
            if (en) nq = get_npc_unsafe(en->npcID);
            if (en && nq) pm_log("encwarp diag at %u: npc %d %d %d flags %x enemy flags %x susp %d cooldown %d hitType %d encState %d actionState %d pflags %x", f, (int)nq->pos.x, (int)nq->pos.y, (int)nq->pos.z, (unsigned)nq->flags, (unsigned)en->flags, (int)en->aiSuspendTime, (int)gCurrentEncounter.battleTriggerCooldown, (int)gCurrentEncounter.hitType, (int)gEncounterState, (int)gPlayerStatus.actionState, (unsigned)gPlayerStatus.flags);
            else pm_log("encwarp diag at %u: no enemy or npc in the list", f);
        }
        if (gEncounterState == ENCOUNTER_STATE_PRE_BATTLE) {
            t0 = pm_frame_count();
            pm_log("encwarp: pre-battle start at frame %u", t0);
            pm_gpu_shot_window(t0, 40);
            state = 2;
        } else if (f - t_move > 600) {
            pm_log_w("encwarp: no pre-battle start in 600 frames, mode %d", (int)get_game_mode());
            state = 3;
        }
    } else if (state == 2 && pm_frame_count() >= t0 + 45) {
        pm_log_evt("encwarp: window end at frame %u", pm_frame_count());
        state = 3;
    }
}

static void battle_hooks(unsigned f) {
    encwarp_tick(f);
    char v[96], vf[16];
    unsigned at = 100;
    if (pm_debug_value("battlef", vf, sizeof vf)) at = (unsigned)atoi(vf);
    if (f == at && (pm_debug_value("battle", v, sizeof v) || pm_debug_has("battle"))) {
        const char* c = strchr(v, ':');
        s32 formation = battle_name_value(v, c ? (unsigned)(c - v) : (unsigned)strlen(v), BTL_KMR_1_FORMATION_00);
        s32 stage = battle_name_value(c ? c + 1 : "", c ? (unsigned)strlen(c + 1) : 0, BTL_KMR_1_STAGE_01);
        pm_log("debug battle: formation %d stage %d at scene frame %u", (int)formation, (int)stage, f);
        /* No world enemy started this battle, so there is no encounter to run: the real update_encounters_neutral reads
         * the enemy list of the world map (get_npc_unsafe assert, unmapped reads). ENCOUNTER_STATE_NONE makes update_encounters do nothing. */
        if (pm_debug_value("spmax", vf, sizeof vf)) gPlayerData.maxStarPower = (s8)atoi(vf); /* WIDE4: spmax=<n> star power bars at the battle start; n > 0 adds the Star Power wheel option */
        if (pm_debug_value("hammer", vf, sizeof vf)) gPlayerData.hammerLevel = (s8)atoi(vf); /* WIDE4: hammer=<n> hammer level at the battle start (0 = Hammer); the Hammer wheel option */
        gEncounterState = ENCOUNTER_STATE_NONE;
        set_battle_stage(stage);
        load_battle(formation);
    }
    if (f == at + 10 && pm_debug_has("battlemenu") && get_game_mode() == GAME_MODE_BATTLE) { /* UI2 debug: battlemenu sets BS_FLAGS1_MENU_OPEN 10 frames after the battle starts, so the enemy HP bar draws (16C8E0.c:905). The bar also needs the enemy tattled (181810.c is_actor_health_bar_visible): save_tattle_flags is the real Tattle path. */
        int bi;
        extern void save_tattle_flags(s32 actorType);
        gBattleStatus.flags1 |= BS_FLAGS1_MENU_OPEN;
        for (bi = 0; bi < ARRAY_COUNT(gBattleStatus.enemyActors); bi++) {
            if (gBattleStatus.enemyActors[bi] != nullptr) save_tattle_flags(gBattleStatus.enemyActors[bi]->actorType);
        }
        pm_log_d("debug battlemenu: menu flag set and enemies tattled f%u", f);
    }
    if (f == at && pm_debug_value("demoscene", v, sizeof v)) { /* demoscene=<n>: any real demo scene (index in DemoScenes of src/state_demo.c; 0 nok_12, 5 jan_04 is unchecked) */
        s32 ds = atoi(v);
        pm_log("debug demo: enter GAME_MODE_DEMO at demo scene %d", (int)ds);
        gGameStatusPtr->demoState = DEMO_STATE_ACTIVE;
        gGameStatusPtr->nextDemoScene = ds;
        set_game_mode(GAME_MODE_DEMO);
    }
}


/* Once per drawn frame (pm_frame_loop). */
static void pm_statehash(unsigned f) {
    { /* MA1 step 0: switch statehash=<n> logs an FNV-1a hash of the game state every n steps (default 10), and a running hash over all samples.
         * Fields only, no pointers, so the value is equal between builds when the game logic is equal. Use it as the replay check for MA1 edits. */
        static unsigned sh_every, sh_run = 2166136261u, sh_n;
        char sv[12];
        if (!sh_every) sh_every = pm_debug_value("statehash", sv, sizeof sv) ? (unsigned)(atoi(sv) > 0 ? atoi(sv) : 10) : 0xFFFFFFFFu;
        if (sh_every != 0xFFFFFFFFu && f % sh_every == 0) {
            unsigned h = 2166136261u, h2 = 2166136261u; /* h2: the same fields without gRandSeed (FS1: a skipped draw does not run the effect draw code that calls the random number generator) */
            #define SHF(v) do { float fv_ = (float)(v); unsigned u_; memcpy(&u_, &fv_, 4); h = (h ^ u_) * 16777619u; h2 = (h2 ^ u_) * 16777619u; } while (0)
            #define SHI(v) do { h = (h ^ (unsigned)(int)(v)) * 16777619u; h2 = (h2 ^ (unsigned)(int)(v)) * 16777619u; } while (0)
            h = (h ^ (unsigned)(int)gRandSeed) * 16777619u;
            SHF(gPlayerStatus.pos.x); SHF(gPlayerStatus.pos.y); SHF(gPlayerStatus.pos.z);
            SHF(gPlayerStatus.targetYaw); SHF(gPlayerStatus.curYaw);
            SHI(gPlayerStatus.actionState); SHI(gPlayerStatus.flags);
            SHI(gPlayerData.curHP); SHI(gPlayerData.curFP); SHI(gPlayerData.coins); SHI(gPlayerData.starPoints); SHI(gPlayerData.curPartner);
            SHI(gGameStatusPtr->demoState); SHI(gGameStatusPtr->mapID); SHI(gGameStatusPtr->areaID);
            for (int i = 0; i < MAX_NPCS; i++) { /* slot order, no search */
                Npc* np = get_npc_by_index(i);
                if (!np || !np->flags) { SHI(-1); continue; }
                SHF(np->pos.x); SHF(np->pos.y); SHF(np->pos.z); SHF(np->yaw); SHI(np->flags); SHI(np->curAnim);
            }
            sh_run = (sh_run ^ h) * 16777619u;
            sh_n++;
            pm_log("statehash f%u %08x run %08x n %u norand %08x", f, h, sh_run, sh_n, h2);
        }
    }
}

void pm_sweep_tick(unsigned f);
#include "model.h"
/* Debug switch modellog=<hex model id>: the id whose draws the model code logs (pm_model_log_id) */
int pm_model_log_id(void) {
    static int id = -2;
    char v[16];
    if (id == -2) id = pm_debug_value("modellog", v, sizeof v) ? (int)strtol(v, NULL, 16) : -1;
    return id;
}

/* WARPQ test switch modewarp=<game mode>:<map>:<entry>: in the first scene frame that the game is in that mode, the warp of the Select
 * menu (pm_warp_live, the call of the Warp now row) goes to the map. Debug tool only, no game logic. */
static void modewarp_hook(unsigned f) {
    static int done;
    char v[48], name[16];
    char *c1, *c2;
    int mode, idx, entry = 0;
    if (done || !pm_debug_value("modewarp", v, sizeof v)) return;
    mode = atoi(v);
    if (get_game_mode() != mode) return;
    done = 1;
    c1 = strchr(v, ':');
    if (!c1) return;
    c2 = strchr(c1 + 1, ':');
    if (c2) entry = atoi(c2 + 1);
    {
        size_t n = c2 ? (size_t)(c2 - (c1 + 1)) : strlen(c1 + 1);
        if (n >= sizeof name) n = sizeof name - 1;
        memcpy(name, c1 + 1, n);
        name[n] = 0;
    }
    idx = pm_warp_map_index(name);
    if (idx < 0) { pm_log_w("modewarp: map %s not found", name); return; }
    pm_log("modewarp: game mode %d at scene frame %u, warp to %s entry %d", mode, f, name, entry);
    pm_warp_live((int)ARRAY_COUNT(presets) + 1 + idx, entry);
}
/* WARPFIX test switch livewarp=<scene frame>:<map>:<entry>: at that frame of the scene, the warp of the Select menu (pm_warp_live, the call of
 * the Warp now row) goes to the map. Debug tool only, no game logic. */
static void livewarp_hook(unsigned f) {
    char v[48], name[16];
    char *c1, *c2;
    int idx, entry = 0;
    if (!pm_debug_value("livewarp", v, sizeof v)) return;
    if (f != (unsigned)atoi(v)) return;
    c1 = strchr(v, ':');
    if (!c1) return;
    c2 = strchr(c1 + 1, ':');
    if (c2) entry = atoi(c2 + 1);
    {
        size_t n = c2 ? (size_t)(c2 - (c1 + 1)) : strlen(c1 + 1);
        if (n >= sizeof name) n = sizeof name - 1;
        memcpy(name, c1 + 1, n);
        name[n] = 0;
    }
    idx = pm_warp_map_index(name);
    if (idx < 0) { pm_log_w("livewarp: map %s not found", name); return; }
    pm_log("livewarp: scene frame %u to %s entry %d", f, name, entry);
    if (pm_debug_has("warpitems")) { /* WARPFIX test only: two mushrooms and 50 coins, so the live warp has items to keep (the test save holds none) */
        gPlayerData.invItems[0] = ITEM_MUSHROOM;
        gPlayerData.invItems[1] = ITEM_MUSHROOM;
        gPlayerData.coins = 50;
        pm_log("warpitems: 2 mushrooms and 50 coins set before the live warp");
    }
    pm_warp_live((int)ARRAY_COUNT(presets) + 1 + idx, entry);
}

/* WARPFIX2 test switches. fswarp=<scene frame>:<map>:<entry>: the file select warp (pm_warp_now, new-file state) at that frame, then the
 * map change as the file select exit makes it. bosstest=<scene frame>:<boss index>: the Boss tab launch (pm_boss_launch) at that frame.
 * Both debug tools only. */
static int test_frame_args(const char* key, unsigned f, char* name, unsigned nn, int* entry) {
    char v[48];
    char *c1, *c2;
    size_t n;
    if (!pm_debug_value(key, v, sizeof v) || f != (unsigned)atoi(v)) return 0;
    c1 = strchr(v, ':');
    if (!c1) return 0;
    c2 = strchr(c1 + 1, ':');
    *entry = c2 ? atoi(c2 + 1) : 0;
    n = c2 ? (size_t)(c2 - (c1 + 1)) : strlen(c1 + 1);
    if (n >= nn) n = nn - 1;
    memcpy(name, c1 + 1, n);
    name[n] = 0;
    return 1;
}
static void test_items(void) { /* WARPFIX test only: two mushrooms and 50 coins, so a warp or a fight has items to keep */
    if (!pm_debug_has("warpitems")) return;
    gPlayerData.invItems[0] = ITEM_MUSHROOM;
    gPlayerData.invItems[1] = ITEM_MUSHROOM;
    gPlayerData.coins = 50;
    pm_log("warpitems: 2 mushrooms and 50 coins set");
}
static void fswarp_hook(unsigned f) {
    char name[16];
    int entry, idx;
    if (!test_frame_args("fswarp", f, name, sizeof name, &entry)) return;
    idx = pm_warp_map_index(name);
    if (idx < 0) { pm_log_w("fswarp: map %s not found", name); return; }
    pm_log("fswarp: scene frame %u to %s entry %d", f, name, entry);
    pm_warp_pending = 0;
    pm_warp_now((int)ARRAY_COUNT(presets) + 1 + idx, entry);
    if (pm_warp_pending) {
        pm_warp_pending = 0;
        warp_end_intro(GAME_MODE_WORLD);
        set_map_transition_effect(TRANSITION_STANDARD);
        set_game_mode(GAME_MODE_CHANGE_MAP);
    }
}
static void bosstest_hook(unsigned f) {
    char v[16];
    if (!pm_debug_value("bosstest", v, sizeof v)) return;
    if (f != (unsigned)atoi(v)) return;
    {
        const char* c = strchr(v, ':');
        int i = c ? atoi(c + 1) : 0;
        pm_log("bosstest: scene frame %u, boss %d", f, i);
        test_items();
        pm_boss_launch(i);
    }
}

extern int pm_stretch;
void pm_warp_tick(unsigned frame) {
    pm_statehash(frame); /* MA1 step 0: runs in every scene, also without warp */
    boss_tick();
    battle_tick(); /* BT2 */
    bypass_tick();
    {   /* AMBTITLE test: titleret=<frame> returns to the title once, in the world (the soft reset path, as the Select menu) */
        static int tr = -2, ambchk_armed;
        if (tr == -2) {
            char tv[16];
            tr = pm_debug_value("titleret", tv, sizeof tv) ? atoi(tv) : -1;
        }
        if (frame == (unsigned)tr && get_game_mode() == GAME_MODE_WORLD) {
            tr = -1;
            pm_title_return();
            pm_log_evt("titleret f%u", frame);
            ambchk_armed = 1;
        }
        if (ambchk_armed && get_game_mode() == GAME_MODE_TITLE_SCREEN) { /* AMBTITLE: the ambient player at the first title frame, many audio frames after the stop */
            AuResult rc = snd_ambient_is_stopped(0);
            ambchk_armed = 0;
            pm_log_evt("ambchk title f%u stopped %d rc %d", frame, rc == AU_RESULT_OK || rc == AU_AMBIENCE_ERROR_3, (int)rc);
        }
    }
    wq_tick(); /* WARPQ: a warp queued during a map change runs in the world */
    pos_tick(frame); /* POSWARP */
    pm_cheat_tick(); /* CH6: moon jump, after the game logic of this step */
    if (scene_f0 < 0) return;
    if (tour_next) tour_tick(frame);
    if (bt_st != BT_OFF) bt_tick(frame); /* BT1 */
    unsigned f = frame - (unsigned)scene_f0;
    pm_sweep_tick(f); /* SWEEP: markers and end of one item of the full sweep (pm_sweep.c, switch sweep=<name>) */
    battle_hooks(f);
    livewarp_hook(f); /* WARPFIX test switch */
    modewarp_hook(f); /* WARPQ test switch */
    fswarp_hook(f); /* WARPFIX2 test switch */
    bosstest_hook(f); /* WARPFIX2 test switch */
    boss_restore_tick(); /* WARPFIX2 */
    {   /* BTLDIM test: invitem=<hex> puts that item in slot 0 at scene frame 30 (Shooting Star fade test) */
        char iv[16];
        if (f == 30 && pm_debug_value("invitem", iv, sizeof iv)) {
            gPlayerData.invItems[0] = (s16)strtol(iv, NULL, 0);
            pm_log("invitem: slot 0 = 0x%03X", (unsigned)gPlayerData.invItems[0]);
        }
    }
    char ti[8];
    if (f == 30 && pm_debug_value("testitem", ti, sizeof ti)) { /* V07 debug: a real item entity beside the player */
        make_item_entity_nodelay(ti[0] == 'c' ? ITEM_COIN : ti[0] == 's' ? ITEM_STAR_PIECE : ti[0] == 'k' ? ITEM_PULSE_STONE : ITEM_MUSHROOM, gPlayerStatus.pos.x + 60.0f, gPlayerStatus.pos.y, gPlayerStatus.pos.z, ITEM_SPAWN_MODE_FIXED_NEVER_VANISH, 0);
    }
    if (pm_debug_has("marlog") && f == 100) { /* UA1 numeric check: 2D px (40 + get_screen_coords x) and the 3D draw px, 100 units to each side of Mario. Values in tenths of a px. */
        extern void get_screen_coords(s32 camID, f32 x, f32 y, f32 z, s32* sx, s32* sy, s32* sz);
        extern float pm_debug_draw_px(s32 camID, float x, float y, float z);
        s32 sx, sy, sz;
        int side;
        for (side = -100; side <= 100; side += 200) {
            f32 wx = gPlayerStatus.pos.x + side;
            float a, b;
            get_screen_coords(gCurrentCameraID, wx, gPlayerStatus.pos.y, gPlayerStatus.pos.z, &sx, &sy, &sz);
            a = 40.0f + sx;
            b = pm_debug_draw_px(gCurrentCameraID, wx, gPlayerStatus.pos.y, gPlayerStatus.pos.z);
            pm_log_d("marlog f%u side %d wide %d a10 %d b10 %d err10 %d", f, side, pm_wide, (int)(a * 10.0f), (int)(b * 10.0f), (int)((a - b) * 10.0f));
        }
    }
    {   /* HUD1 debug: uishow=bar opens the status bar (open_status_bar_slowly) at frame 200, for the wide layout shots in the world */
        char uv[16];
        if (f == 200 && pm_debug_value("uishow", uv, sizeof uv) && uv[0] == 'b' && uv[1] == 'a') {
            extern void open_status_bar_slowly(void);
            open_status_bar_slowly();
            pm_log_d("debug uishow: bar opened f%u", f);
        }
    }
    {   /* WIDE3 debug: uishow=shop. From scene frame 200 the map's shop keeps its item info on (the shop clears it off its trigger). At 200 the item positions are logged and Mario is placed on item 2. */
        static s32 uishowShop;
        char uv[16];
        if (f == 200) {
            uishowShop = pm_debug_value("uishow", uv, sizeof uv) && uv[0] == 's' && uv[1] == 'h';
            if (uishowShop && gGameStatusPtr->mapShop != NULL) {
                extern void pm_pos_set(const float* v, int n);
                s32 k;
                float pv[3];
                for (k = 0; k < gGameStatusPtr->mapShop->numItems && k < 6; k++) {
                    ShopItemEntity* e = &gGameStatusPtr->shopItemEntities[k];
                    pm_log_d("debug uishow: shop item %d at %d %d %d", k, (int) e->pos.x, (int) e->pos.y, (int) e->pos.z);
                }
                pv[0] = gGameStatusPtr->shopItemEntities[2].pos.x;
                pv[1] = gGameStatusPtr->shopItemEntities[2].pos.y;
                pv[2] = gGameStatusPtr->shopItemEntities[2].pos.z;
                pm_pos_set(pv, 3);
                pm_log_d("debug uishow: shop items f%u", f);
            }
        }
        if (uishowShop && gGameStatusPtr->mapShop != NULL) {
            gGameStatusPtr->mapShop->flags |= SHOP_FLAG_SHOWING_ITEM_INFO;
            gGameStatusPtr->mapShop->curItemSlot = 2;
        }
    }
    {   /* UA3 debug: uishow=coins opens the coin counter window (show_coin_counter, the pickup's own call) at frame 120 */
        char uv[16];
        if (f == 200 && pm_debug_value("uishow", uv, sizeof uv) && uv[0] == 'c' && uv[1] == 'o') {
            extern void show_coin_counter(void);
            extern void pm_debug_status_always(void);
            pm_debug_status_always();
            show_coin_counter();
            pm_log_d("debug uishow: coins opened f%u", f);
        }
        /* UI1 debug: uishow=choice opens the save choice (MSG_Choice_000A, the glove and text menu) at frame 200 */
        if (f == 200 && pm_debug_value("uishow", uv, sizeof uv) && uv[0] == 'c' && uv[1] == 'h') {
            static bool uishow_choice_closed;
            extern MessagePrintState* msg_get_printer_for_msg(s32 msgID, bool* donePrintingWriteback);
            msg_get_printer_for_msg(MSG_Choice_000A, &uishow_choice_closed);
            pm_log_d("debug uishow: choice opened f%u", f);
        }
        /* MSGBOX debug: uishow=talk opens a style right message (MSG_MAC_Plaza_0000: its first page waits with the next-page star) and a tattle
         * bubble (MSG_EntityTattle_HammerBlock1_CanBreak) whose origin is left of the 4:3 area (x -30, y 220), so the bubble clamps to the left edge */
        if (f == 200 && pm_debug_value("uishow", uv, sizeof uv) && uv[0] == 't' && uv[1] == 'a') {
            static bool uishow_talk_closed, uishow_tattle_closed;
            extern MessagePrintState* msg_get_printer_for_msg(s32 msgID, bool* donePrintingWriteback);
            MessagePrintState* tp;
            msg_get_printer_for_msg(MSG_MAC_Plaza_0000, &uishow_talk_closed);
            tp = msg_get_printer_for_msg(MSG_EntityTattle_HammerBlock1_CanBreak, &uishow_tattle_closed);
            if (tp != NULL) msg_printer_set_origin_pos(tp, -30, 220);
            pm_log_d("debug uishow: talk opened f%u", f);
        }
    }
    if (f % 300 == 0) {
        pm_log_d("warp: scene f%u t=%u pos %d %d %d inputDisabled %d actionState %u vy %d maxJump %d walk %d run %d wall %d push %d", f, pm_uptime_ms(), (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y,
               (int)gPlayerStatus.pos.z, (gPlayerStatus.flags & PS_FLAG_INPUT_DISABLED) != 0, (unsigned)gPlayerStatus.actionState,
               (int)(gPlayerStatus.gravityIntegrator[0] * 100), (int)(gPlayerStatus.maxJumpSpeed * 1000), (int)(gPlayerStatus.walkSpeed * 100), (int)(gPlayerStatus.runSpeed * 100), (int)gCollisionStatus.curWall, (int)gCollisionStatus.pushingAgainstWall);
    }
    if (smoke_on && f == smoke_frames) pm_audio_hash_log();
    if (smoke_on && f == smoke_frames) pm_log("warp: smoke done (%d frames in the scene) t=%u ms", smoke_frames, pm_uptime_ms());
    { /* BOOTSTATE: bootstate=<first>:<count> logs one text line per step frame of the window: the global draw inputs (layout, interp, seed, camera, Mario). The base compare of two runs reads them. */
        static int bs0 = -2, bs1;
        if (bs0 == -2) {
            char bsv[24];
            bs0 = -1;
            if (pm_debug_value("bootstate", bsv, sizeof bsv) && sscanf(bsv, "%d:%d", &bs0, &bs1) == 2) bs1 += bs0;
            else bs0 = -1;
        }
        if (bs0 >= 0 && (int)frame >= bs0 && (int)frame < bs1) {
            pm_log_evt("bootstate f%u wide %d stretch %d interp %d optwide %d fps60 %d seed %08x cam %d %d %d tgt %d %d %d mario %d %d %d mode %d",
                frame, pm_wide, pm_stretch, pm_interp_on, pm_opt.wide, pm_opt.fps60, (unsigned)gRandSeed,
                (int)gCameras[0].lookAt_eye.x, (int)gCameras[0].lookAt_eye.y, (int)gCameras[0].lookAt_eye.z,
                (int)gCameras[0].lookAt_obj.x, (int)gCameras[0].lookAt_obj.y, (int)gCameras[0].lookAt_obj.z,
                (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y, (int)gPlayerStatus.pos.z, (int)get_game_mode());
        }
    }
}

/* P02: switch `ptrcheck`. Called by the PTRCHECK macro (include/macros.h) when a value in the host pointer range reaches an
 * ID, number or float place. Logs once per (what, caller). `caller` is a code address: use addr2line on it. */
void pm_ptrcheck(const char* what, unsigned val, const void* caller) {
    static int on = -1;
    static unsigned keys[512];
    static int n;
    unsigned key = (unsigned)caller ^ ((unsigned)what * 2654435761u);
    int i;

    if (on < 0) {
        on = pm_debug_has("ptrcheck");
    }
    if (!on) {
        return;
    }
    for (i = 0; i < n; i++) {
        if (keys[i] == key) {
            return;
        }
    }
    if (n < 512) {
        keys[n++] = key;
    }
    pm_log("ptrcheck: %s value %08x caller %08x", what, val, (unsigned)caller);
}

/* X02: one log line when the darkness alpha or the partner action state changes (Watt test) */
void pm_x02_log(int isWorld, int alpha, int x, int y) {
    static int la = -1, ls = -1;
    if (alpha != la || (int)gPartnerStatus.partnerActionState != ls) {
        la = alpha;
        ls = (int)gPartnerStatus.partnerActionState;
        pm_log_d("X02: darkness world %d alpha %d pos %d %d partnerAction %d cur %d", isWorld, alpha, x, y, ls, (int)gPlayerData.curPartner);
    }
}


/* BTRANS caplog: the star wipe state of the encounter (state * 1000 + fadeOutAmount, 0 when no encounter runs). */
int pm_enc_wipe(void) { return (int)gEncounterState * 1000 + (int)gCurrentEncounter.fadeOutAmount; }
