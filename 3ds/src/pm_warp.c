/* Slice W01 (class E, port-side debug tool): `warp=<map>:<entry>`, `story=<name|number>`, `preset=<name>`.
 * No decomp function is replaced. When the game leaves GAME_MODE_STARTUP (its real set_game_mode(GAME_MODE_LOGOS)
 * call, see __wrap_set_game_mode in pm_map.c) the hook does what the real new-file menu does (clear_player_data,
 * clear_saved_variables, area/map/entry, GB_StoryProgress through evt_set_variable, fio_serialize_state) and then
 * enters the real GAME_MODE_ENTER_WORLD, which loads the map through the real state machine.
 * So logos, intro, title and file select are skipped. Without the switches nothing changes. */
#include "common.h"
#include "functions.h"
#include "game_modes.h"
#include "fio.h"
#include "battle/battle.h"
extern bool fio_fetch_saved_file_info(void);
#include "map.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>

void fio_serialize_state(void);
unsigned pm_uptime_ms(void);

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
static int padscript_log, warp_on, warp_done, scene_f0 = -1, smoke_on;
/* S30: tour=<map>:<entry>,<map>:<entry>,... After warp= (or preset=) has loaded its first map, the real GotoMap path
   (set_map_transition_effect + set_game_mode(GAME_MODE_CHANGE_MAP), as in src/evt/map_api.c goto_map) visits every
   map of the list, TOUR_FRAMES frames each. Log lines "tour: ...". Debug tool only (class E). */
#define TOUR_FRAMES 40
static char tour[6000];
static char blist[8000];
static const char* blist_next;
static int bl_state, bl_n, bl_t;
static const char* tour_next;
static unsigned tour_f0, tour_ts;
static int tour_n, tour_wait;
static s32 w_story = STORY_INTRO;
static char w_map[16] = "kmr_20";
static s32 w_entry, w_flag, w_partner;

int pm_warp_active(void) {
    char v[40];
    int has_preset = pm_debug_value("preset", v, sizeof v);
    /* PT01b: the Warp-to choice of the options (pm_opt.warp) is not used at boot. Only the switches preset, warp and story warp at boot. */
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
    if (has_warp) {
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
    if (pm_debug_value("tour", tour, sizeof tour)) {
        tour_next = tour;
        if (tour[0] == '@') { /* tour=@file: map:entry list in a file of the SD app folder (the switch string is only 256 bytes) */
            char fn[40];
            FILE* tf;
            strncpy(fn, tour + 1, sizeof fn - 1);
            fn[sizeof fn - 1] = 0;
            tour[0] = 0;
            if ((tf = fopen(fn, "r")) != NULL) {
                size_t n = fread(tour, 1, sizeof tour - 1, tf);
                fclose(tf);
                tour[n] = 0;
                for (n = 0; tour[n]; n++) if (tour[n] == 10 || tour[n] == 13 || tour[n] == ' ') tour[n] = ',';
            }
        }
    }
    if (pm_debug_value("battlelist", v, sizeof v) && v[0] == '@') { /* C2R: battlelist=@file, one <formation>:<stage> per line, one Azahar start for the whole list (battlen=<frames> each, default 30) */
        FILE* bf = fopen(v + 1, "r");
        if (bf) {
            size_t n = fread(blist, 1, sizeof blist - 1, bf);
            fclose(bf);
            blist[n] = 0;
            blist_next = blist;
        }
    }
    /* F01: a tour visits maps at story 0 with a new file (no partner). The real game has a partner at trd_06 and later maps (trd_06 InitializeFallingPartner, partner_walking_enable need one), so the tour uses Goombario unless partner= says otherwise. */
    if (tour_next != NULL && w_partner == 0) w_partner = 1;
    /* H06e: a warp past the prologue also needs a partner. tik_01_CheckDripCollisionWithNPC reads the partner npc, the real game has one in the sewers. */
    if ((has_warp || opt_map) && w_partner == 0 && strncmp(w_map, "kmr_", 4) != 0) w_partner = 1;
    smoke_on = pm_debug_has("smoke");
    if (pm_debug_value("loadslot", v, sizeof v)) w_loadslot = atoi(v); /* E02: loadslot=<n> loads save slot n first */
    if (pm_debug_value("smokeframes", v, sizeof v)) smoke_frames = atoi(v); /* E02: smoke length, default 300 */
    padscript_log = pm_debug_has("padlog") || pm_debug_value("padscript", v, sizeof v); /* position every 2 frames while a script runs */
    return has_preset || has_warp || has_story || opt_map;
}

static void warp_setup(s16 areaID, s16 mapID);

/* Called from the set_game_mode wrapper. Returns the mode to enter (the caller then calls the real set_game_mode). */
s16 pm_warp_hook(s16 mode) {
    s16 areaID, mapID;
    if (warp_done || mode != GAME_MODE_LOGOS) return mode;
    warp_done = 1;
    if (!pm_warp_active()) return mode;
    if (!get_map_IDs_by_name(w_map, &areaID, &mapID)) {
        pm_log_w("warp: map %s not found, normal start", w_map);
        return mode;
    }
    warp_setup(areaID, mapID);
    return GAME_MODE_ENTER_WORLD;
}

static void warp_setup(s16 areaID, s16 mapID) {
    if (w_loadslot >= 0) { /* E02: load the file as the file select does (name, save count), then override map and story */
        fio_fetch_saved_file_info();
        pm_log("warp: loadslot %d %s", w_loadslot, fio_load_game(w_loadslot) ? "loaded" : "FAILED");
    }
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)w_entry;
    if (w_loadslot < 0) clear_player_data();
    if (w_loadslot < 0) clear_saved_variables();
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)w_entry;
    evt_set_variable(NULL, GB_StoryProgress, w_story);
    if (w_flag) evt_set_variable(NULL, w_flag, 1);
    if (pm_debug_has("mapflags")) { /* V23b: mapflags sets every GF_MAP_* flag (visited spaces) so the pause map shows the path dots */
        s32 mi;
        for (mi = GF_MAP_ToadTown; mi <= GF_MAP_MariosHouse; mi++) evt_set_variable(NULL, mi, 1);
    }
    if (w_partner > 0 && w_partner < 12) { /* slice F02: a map script that calls EnablePartnerAI needs a partner */
        gPlayerData.partners[w_partner].enabled = 1;
        gPlayerData.curPartner = w_partner;
    }
    char vs[16];
    if (pm_debug_value("sp", vs, sizeof vs)) gPlayerData.starPoints = (s8)atoi(vs); /* slice S19d: sp=<n> star points, 99 + one win = level up */
    if (pm_debug_value("hp", vs, sizeof vs)) gPlayerData.curHP = (s8)atoi(vs); /* slice S19d: hp=<n> Mario HP, 1 = lose the battle fast */
    fio_serialize_state();
    warp_on = 1;
    pm_log("warp: %s entry %d (area %d map %d) story %d flag %x", w_map, w_entry, areaID, mapID, w_story, w_flag);
}

int pm_warp_preset_count(void) { return (int)ARRAY_COUNT(presets); }

/* PT01b: test maps (area_tst, ids tst_*) are cut content. tst_12 fills the 96 effect slots and crashed the N3DS (docs/plans/n3crash.md, Q-F10 option B).
 * The list of maps for the Warp-to option and the options.ini reader do not offer them, and a saved or switch value is ignored. Switch warptest allows them. */
int pm_warp_is_test_map(const char* id) { return id && strncmp(id, "tst_", 4) == 0 && !pm_debug_has("warptest"); }

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
/* A warp choice: 0 = off, 1 to P = preset, P+1 and up = map k = choice - P - 1. */
int pm_warp_choice_count(void) { return 1 + (int)ARRAY_COUNT(presets) + pm_warp_map_count(); }
const char* pm_warp_choice_text(int c, int entry) {
    static char b[24];
    int P = (int)ARRAY_COUNT(presets);
    if (c <= 0) return "Off";
    if (c <= P) return presets[c - 1].name;
    snprintf(b, sizeof b, "%s", pm_warp_map_name(c - 1 - P));
    (void)entry;
    return b;
}
const char* pm_warp_preset_name(int i) { return presets[i].name; }
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
    warp_setup(areaID, mapID);
    pm_warp_pending = 1;
}

/* PT63: warp from the Select menu in any game mode that has a world to leave. A map change from the world, ENTER_WORLD from the title or the intro
 * (as pm_warp_hook does at boot). The state is set as a preset sets it (new player data at the story value). Nothing is written to disk. */
/* PT74: a warp or a boss launch from the Select menu while a battle runs. The battle is ended through the real BATTLE_STATE_END_BATTLE (the way
 * battlelist ends one), the game returns to the world, then the action runs. The state does not need to be exact. 0 none, 1 warp, 2 boss. */
static int bypass_kind, bypass_a, bypass_b;
static unsigned bypass_n;
static int bypass_begin(int kind, int a, int b) {
    if (bypass_kind) { pm_menu_msg("A battle end is already running"); return 0; }
    bypass_kind = kind; bypass_a = a; bypass_b = b; bypass_n = 0;
    if (get_game_mode() == GAME_MODE_PAUSE) set_game_mode(GAME_MODE_UNPAUSE); /* PT81: the pause menu closes the way its own close key does, then the action runs in the world */
    else btl_set_state(BATTLE_STATE_END_BATTLE);
    pm_menu_msg("Ending the battle, then %s", kind == 1 ? "warp" : "boss");
    pm_log("bypass: battle end for kind %d (%d, %d)", kind, a, b);
    return 0;
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
    pm_log("warp: song and ambient sounds stopped, demoState was %d, game mode %d", (int)gGameStatusPtr->demoState, mode);
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
int pm_warp_live(int c, int entry) {
    if (get_game_mode() == GAME_MODE_BATTLE || get_game_mode() == GAME_MODE_PAUSE) return bypass_begin(1, c, entry);
    return pm_warp_live_now(c, entry);
}
static int pm_warp_live_now(int c, int entry) {
    int mode = get_game_mode();
    int ok = mode == GAME_MODE_WORLD || mode == GAME_MODE_TITLE_SCREEN || mode == GAME_MODE_INTRO || mode == GAME_MODE_DEMO;
    if (!ok) {
        pm_log_w("warp: not possible in game mode %d", mode);
        pm_menu_msg("Warp not possible in game mode %d", mode);
        return 0;
    }
    pm_warp_pending = 0;
    pm_warp_now(c, entry);
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
    if (mode != GAME_MODE_WORLD && mode != GAME_MODE_TITLE_SCREEN && mode != GAME_MODE_INTRO && mode != GAME_MODE_DEMO) {
        pm_menu_msg("Boss launch not possible in game mode %d", mode);
        return 0;
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
    if (++boss_frames < 30) return;
    v[0] = 0;
    strncat(v, boss_tab[boss_pending].battle, sizeof v - 1);
    c = strchr(v, ':');
    formation = battle_name_value(v, c ? (unsigned)(c - v) : (unsigned)strlen(v), BTL_KMR_1_FORMATION_00);
    stage = battle_name_value(c ? c + 1 : "", c ? (unsigned)strlen(c + 1) : 0, BTL_KMR_1_STAGE_01);
    pm_log("boss: start battle formation %d stage %d", (int)formation, (int)stage);
    boss_pending = -1;
    gEncounterState = ENCOUNTER_STATE_NONE;
    set_battle_stage(stage);
    load_battle(formation);
}

/* PT66: Return to Title Screen. The game own soft reset: the Mario overlay fades in, the game sounds stop, then GAME_MODE_STARTUP runs
 * the engine setup again and the logos lead to the title. The flash is flushed and the audio drained first, as for the HOME menu. */
extern s16 SoftResetOverlayAlpha, SoftResetState;
void pm_title_return(void) {
    pm_flash_flush("return to title");
    pm_audio_drain(3);
    SoftResetOverlayAlpha = 0;
    SoftResetState = 0;
    gOverrideFlags |= GLOBAL_OVERRIDES_SOFT_RESET;
    pm_log("title: soft reset requested");
}

void pm_padscript_start(void);
extern unsigned pm_pad_scans;

/* The game reaches GAME_MODE_WORLD: first frame of the chosen scene. */
void pm_warp_world_mode(unsigned frame) {
    if (tour_next && warp_on) {
        tour_f0 = frame; /* a map of the tour is in */
        tour_wait = 0;
        pm_log_d("tour: in map area %d map %d entry %d frame %u t=%u ms", gGameStatusPtr->areaID, gGameStatusPtr->mapID, gGameStatusPtr->entryID, frame, pm_uptime_ms());
    }
    if (!warp_on || scene_f0 >= 0) return;
    scene_f0 = (int)frame;
    pm_log_d("warp: scene frame0 %u scan %u t=%u ms", frame, pm_pad_scans, pm_uptime_ms());
    pm_padscript_start();
}

static void tour_step(void) {
    char name[16];
    s16 areaID, mapID;
    int entry = 0;
    unsigned i = 0;
    while (*tour_next == ',') tour_next++;
    if (!*tour_next) {
        pm_log("tour: done, %d maps", tour_n);
        tour_next = NULL;
        return;
    }
    while (*tour_next && *tour_next != ':' && *tour_next != ',' && i + 1 < sizeof name) name[i++] = *tour_next++;
    name[i] = 0;
    if (*tour_next == ':') entry = atoi(++tour_next);
    while (*tour_next && *tour_next != ',') tour_next++;
    if (*tour_next == ',') tour_next++;
    tour_n++;
    if (!get_map_IDs_by_name(name, &areaID, &mapID)) {
        pm_log_w("tour: map %s not in gAreas", name);
        return;
    }
    pm_log("tour: next %s entry %d (area %d map %d)", name, entry, areaID, mapID);
    /* F01: the tour skips the exit script of the map (kmr_23 N(func_80240010_907A40)). Do what it does, else the curtain callback runs on the next map. */
    set_curtain_scale_goal(2.0f);
    set_curtain_draw_callback(NULL);
    /* F01 nok_01: the tour skips the scripts that stop BGM player B (trd_05 plays SONG_KOOPA_BROS_INTERLUDE on it). nok_01 loads the radio MSEQ files
       into dataMSEQ[1], the same buffer as dataBGM[1] (decomp layout, engine.c), so a song left on B reads overwritten data. */
    bgm_set_song(1, AU_SONG_NONE, 0, 250, VOL_LEVEL_FULL);
    gGameStatusPtr->areaID = areaID;
    gGameStatusPtr->mapID = mapID;
    gGameStatusPtr->entryID = (s16)entry;
    {   /* X01: tourfx=<n> uses ScreenTransition n for the tour changes (default TRANSITION_STANDARD) */
        char fx[8];
        set_map_transition_effect(pm_debug_value("tourfx", fx, sizeof fx) ? (ScreenTransition)atoi(fx) : TRANSITION_STANDARD);
    }
    set_game_mode(GAME_MODE_CHANGE_MAP);
    tour_wait = 1;
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

/* C2R: battlelist runs one battle after the other in one process. A battle ends through the real BATTLE_STATE_END_BATTLE (the way a won battle ends). */
static void battlelist_tick(unsigned f) {
    char vn[16];
    unsigned per = pm_debug_value("battlen", vn, sizeof vn) ? (unsigned)atoi(vn) : 30;
    if (!blist_next) return;
    if (bl_state == 0 && get_game_mode() == GAME_MODE_WORLD && f >= (unsigned)bl_t) {
        char line[96];
        unsigned i = 0;
        while (*blist_next == '\n' || *blist_next == '\r' || *blist_next == ' ') blist_next++;
        if (!*blist_next) {
            pm_log("battlelist: done, %d battles", bl_n);
            blist_next = NULL;
            return;
        }
        while (*blist_next && *blist_next != '\n' && *blist_next != '\r' && i + 1 < sizeof line) line[i++] = *blist_next++;
        line[i] = 0;
        const char* c = strchr(line, ':');
        s32 formation = battle_name_value(line, c ? (unsigned)(c - line) : i, BTL_KMR_1_FORMATION_00);
        s32 stage = battle_name_value(c ? c + 1 : "", c ? (unsigned)strlen(c + 1) : 0, BTL_KMR_1_STAGE_01);
        pm_log("battlelist: %d start %s (formation %d stage %d) at scene frame %u", bl_n, line, (int)formation, (int)stage, f);
        bl_n++;
        gEncounterState = ENCOUNTER_STATE_NONE;
        set_battle_stage(stage);
        load_battle(formation);
        bl_state = 1;
        bl_t = (int)f;
    } else if (bl_state == 1 && get_game_mode() == GAME_MODE_BATTLE && f - (unsigned)bl_t >= per) {
        btl_set_state(BATTLE_STATE_END_BATTLE);
        bl_state = 2;
        bl_t = (int)f;
    } else if (bl_state == 2) {
        if (get_game_mode() == GAME_MODE_WORLD) {
            bl_state = 0;
            bl_t = (int)f + 10;
        } else if (f - (unsigned)bl_t > 900) {
            pm_log_w("battlelist: battle %d did not end in 900 frames, mode %d, stop", bl_n - 1, (int)get_game_mode());
            blist_next = NULL;
        }
    } else if (bl_state == 1 && f - (unsigned)bl_t > 900) {
        pm_log_w("battlelist: battle %d never reached the battle mode, mode %d, stop", bl_n - 1, (int)get_game_mode());
        blist_next = NULL;
    }
}

static void battle_hooks(unsigned f) {
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
        gEncounterState = ENCOUNTER_STATE_NONE;
        set_battle_stage(stage);
        load_battle(formation);
    }
    if (pm_debug_value("flashheavy", v, sizeof v)) { /* V16b2: `flashheavy=<scene frame>` forces a heavy flash (damage 7) on every enemy part, again every 40 frames, with the real part_flash_on path */
        unsigned ff = (unsigned)atoi(v);
        if (f >= ff && (f - ff) % 40 == 0 && get_game_mode() == GAME_MODE_BATTLE) {
            int i;
            gBattleStatus.lastAttackDamage = 7;
            for (i = 0; i < ARRAY_COUNT(gBattleStatus.enemyActors); i++) {
                Actor* a = gBattleStatus.enemyActors[i];
                ActorPart* p = a ? a->partsTable : NULL;
                for (; p; p = p->nextPart) {
                    if (p->decorationTable) { p->decorationTable->flashState = 1; p->decorationTable->flashStateChanged = true; }
                }
            }
            pm_log("debug flashheavy at scene frame %u", f);
        }
    }
    if (f == at && (pm_debug_has("demobattle") || pm_debug_value("demoscene", v, sizeof v))) { /* demoscene=<n>: any real demo scene (index in DemoScenes of src/state_demo.c; 0 nok_12, 5 jan_04 is unchecked) */
        s32 ds = pm_debug_has("demobattle") ? 1 : atoi(v);
        pm_log("debug demo: enter GAME_MODE_DEMO at demo scene %d", (int)ds);
        gGameStatusPtr->demoState = DEMO_STATE_ACTIVE;
        gGameStatusPtr->nextDemoScene = ds;
        set_game_mode(GAME_MODE_DEMO);
    }
}

/* PT26L: switch camlog logs the default camera and the player once per game step: step, map, area, story, camera update mode, target, lookAt eye and
 * object, yaw, boom yaw, pitch, length, settings pointer, interp alpha, player pos and action state. At most 30 lines per second (one per step). No cost when off. */
static void pm_camlog(unsigned f) {
    static int on = -1;
    static unsigned sec0, n_sec;
    if (on < 0) on = pm_debug_has("camlog");
    if (!on) return;
    unsigned now = pm_uptime_ms();
    if (now - sec0 >= 1000u) { sec0 = now; n_sec = 0; }
    if (n_sec++ >= 60) return;
    Camera* c = &gCameras[CAM_DEFAULT];
    pm_log("camlog f%u map %d area %d story %d mode %d tgt %.1f %.1f %.1f eye %.1f %.1f %.1f obj %.1f %.1f %.1f yaw %.1f byaw %.1f pitch %.1f len %.1f set %08x alpha %.3f pl %.1f %.1f %.1f as %d",
           f, (int)gGameStatusPtr->mapID, (int)gGameStatusPtr->areaID, (int)evt_get_variable(NULL, GB_StoryProgress), (int)c->updateMode,
           c->targetPos.x, c->targetPos.y, c->targetPos.z, c->lookAt_eye.x, c->lookAt_eye.y, c->lookAt_eye.z, c->lookAt_obj.x, c->lookAt_obj.y, c->lookAt_obj.z,
           c->curYaw, c->curBoomYaw, c->curBoomPitch, c->curBoomLength, (unsigned)(uintptr_t)c->curSettings, c->interpAlpha,
           gPlayerStatus.pos.x, gPlayerStatus.pos.y, gPlayerStatus.pos.z, (int)gPlayerStatus.actionState);
}

/* Once per drawn frame (pm_frame_loop). */
static void pm_statehash(unsigned f) {
    pm_camlog(f);
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
/* Debug tool, switch modeldump=<scene frame>: one log line per model of the map (id, flags, texture id, render mode, display list) and the IA4 texture loads
 * found in its list. Finds a model that the map has but the draw list does not show (PT40). */
static void model_dump(unsigned f) {
    char v[16];
    int i;
    if (!pm_debug_value("modeldump", v, sizeof v) || f != (unsigned)atoi(v)) return;
    for (i = 0; i < MAX_MODELS; i++) {
        Model* m = (*gCurrentModels)[i];
        Gfx* dl;
        int n, ia4 = 0;
        if (!m) continue;
        dl = (m->modelNode && m->modelNode->displayData) ? m->modelNode->displayData->displayList : NULL;
        for (n = 0; dl && n < 400; n++) {
            u32 w0 = dl[n].words.w0;
            if ((w0 >> 24) == 0xDF) break; /* G_ENDDL */
            if ((w0 >> 24) == 0xFD && ((w0 >> 21) & 7) == 3 && ((w0 >> 19) & 3) == 0) ia4++; /* G_SETTIMG IA 4b */
        }
        pm_log("modeldump %d id %x flags %04x tex %d var %d rm %d cgfx %d dl %p ia4loads %d", i, (unsigned)m->modelID, (unsigned)m->flags, (int)m->textureID, (int)m->textureVariation, (int)m->renderMode, (int)m->customGfxIndex, (void*)dl, ia4);
    }
}

void pm_warp_tick(unsigned frame) {
    pm_statehash(frame); /* MA1 step 0: runs in every scene, also without warp */
    model_dump(frame);
    boss_tick();
    bypass_tick();
    if (scene_f0 < 0) return;
    if (tour_next && !tour_wait && frame - tour_f0 >= TOUR_FRAMES) {
        tour_ts = frame;
        tour_step();
    } else if (tour_next && tour_wait && frame - tour_ts > 300) { /* the change was refused or never came */
        pm_log_w("tour: no map change within 300 frames");
        tour_wait = 0;
        tour_f0 = frame - TOUR_FRAMES + 1;
    }
    unsigned f = frame - (unsigned)scene_f0;
    pm_sweep_tick(f); /* SWEEP: markers and end of one item of the full sweep (pm_sweep.c, switch sweep=<name>) */
    battle_hooks(f);
    battlelist_tick(f);
    { /* V24b: btlstatus=poison|paralyze|static|dizzy: every frame in a battle the first enemy actor gets that status, to see the palette effect of actor_rendering.c */
        char sv[12];
        if (pm_debug_value("btlstatus", sv, sizeof sv) && get_game_mode() == GAME_MODE_BATTLE && gBattleStatus.enemyActors[0]) {
            Actor* ea = gBattleStatus.enemyActors[0];
            if (!strcmp(sv, "flicker")) { ActorPart* ap; for (ap = ea->partsTable; ap; ap = ap->nextPart) ap->opacity = 128; } /* V16: the damage flicker */
            else if (!strcmp(sv, "static")) { ea->staticStatus = STATUS_KEY_STATIC; ea->staticDuration = 99; }
            else {
                ea->debuff = !strcmp(sv, "poison") ? STATUS_KEY_POISON : !strcmp(sv, "paralyze") ? STATUS_KEY_PARALYZE : STATUS_KEY_DIZZY;
                ea->debuffDuration = 99;
            }
        }
    }
    char ti[8];
    if (f == 30 && pm_debug_value("testitem", ti, sizeof ti)) { /* V07 debug: a real item entity beside the player */
        make_item_entity_nodelay(ti[0] == 'c' ? ITEM_COIN : ti[0] == 's' ? ITEM_STAR_PIECE : ti[0] == 'k' ? ITEM_PULSE_STONE : ITEM_MUSHROOM, gPlayerStatus.pos.x + 60.0f, gPlayerStatus.pos.y, gPlayerStatus.pos.z, ITEM_SPAWN_MODE_FIXED_NEVER_VANISH, 0);
    }
    if (pm_debug_has("simlog") && f >= 110 && f <= 150) { /* V26: every step, Mario and demo Bills */
        extern unsigned pm_step_calls;
        char b[120] = ""; int n = 0, i;
        for (i = 0; i < 8; i++) { Npc* np = get_npc_safe(i); if (np) n += snprintf(b + n, sizeof b - n, " n%d:%d,%d", i, (int)np->pos.x, (int)np->pos.y); }
        pm_log("simstep f%u steps %u mx %d my %d st %u btn %x%s", f, pm_step_calls, (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y, (unsigned)gPlayerStatus.actionState, (unsigned)gGameStatusPtr->demoButtonInput, b);
    }
    if (pm_debug_has("simlog") && f % 10 == 0) /* V26: game state per 10 steps, to compare default, fps60 and wide */
        pm_log("simlog f%u seed %08x pos %d %d %d state %u dstate %d", f, (unsigned)gRandSeed, (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y, (int)gPlayerStatus.pos.z, (unsigned)gPlayerStatus.actionState, (int)gGameStatusPtr->demoState);
    if (f % (padscript_log ? 2 : 100) == 0 || f == 5) {
        pm_log_d("warp: scene f%u t=%u pos %d %d %d inputDisabled %d actionState %u vy %d maxJump %d walk %d run %d wall %d push %d", f, pm_uptime_ms(), (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y,
               (int)gPlayerStatus.pos.z, (gPlayerStatus.flags & PS_FLAG_INPUT_DISABLED) != 0, (unsigned)gPlayerStatus.actionState,
               (int)(gPlayerStatus.gravityIntegrator[0] * 100), (int)(gPlayerStatus.maxJumpSpeed * 1000), (int)(gPlayerStatus.walkSpeed * 100), (int)(gPlayerStatus.runSpeed * 100), (int)gCollisionStatus.curWall, (int)gCollisionStatus.pushingAgainstWall);
    }
    /* D03 check: switch prevtest turns on the game's own previous-frame blur overlay (OVERLAY_BLUR, real
     * set_screen_overlay_params_front) from scene frame 100 on (each frame), so the frame-buffer read can be seen. */
    if (f >= 100 && pm_debug_has("prevtest")) {
        set_screen_overlay_params_front(12, 200.0f);
        set_screen_overlay_center(0, 0, 20, 20);
        set_screen_overlay_center(0, 1, 300, 220);
    }
    /* PT08 check: switch msgtest shows MSG_MAC_Bridge_0026 ([Static 50][Shake][Wave][PrintRising]) at the screen position 100, 100 at scene frame 60. */
    if (f == 60 && pm_debug_has("msgtest")) {
        static bool msg_done;
        MessagePrintState* pr = msg_get_printer_for_msg(MSG_MAC_Bridge_0026, &msg_done);
        msg_printer_set_origin_pos(pr, 100, 100);
    }
    /* P02 control: switch ptrtest feeds a host pointer value to each ptrcheck site once, to prove the check logs. Needs ptrcheck. */
    if (f == 50 && pm_debug_has("ptrtest")) {
        get_npc_safe(0x200000);
        evt_get_float_variable(NULL, 0x200000);
        evt_set_variable(NULL, 0x200000, 1);
        evt_set_float_variable(NULL, 0x200000, 1.0f);
    }
    /* P02 control for the fixes: an Npc pointer must come back from resolve_npc, a raw message buffer must be measured. */
    if (f == 60 && pm_debug_has("ptrtest")) {
        extern u8 MessagePlural[];
        Npc* n = get_npc_safe(NPC_PARTNER);
        s32 w = get_msg_width((s32)MessagePlural, 0);
        pm_log("ptrtest: resolve_npc(ptr) %s, partner npc %08x, get_msg_width(raw buffer) %d", (n != NULL && resolve_npc(NULL, (s32)n) == n) ? "ok" : "FAIL", (unsigned)n, (int)w);
    }
    if (smoke_on && f == smoke_frames) pm_audio_hash_log();
    if (smoke_on && f == smoke_frames) pm_log("warp: smoke done (%d frames in the scene) t=%u ms", smoke_frames, pm_uptime_ms());
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
