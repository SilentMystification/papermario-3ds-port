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
static const char* tour_next;
static unsigned tour_f0, tour_ts;
static int tour_n, tour_wait;
static s32 w_story = STORY_INTRO;
static char w_map[16] = "kmr_20";
static s32 w_entry, w_flag, w_partner;

int pm_warp_active(void) {
    char v[40];
    int has_preset = pm_debug_value("preset", v, sizeof v);
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
        if (!found) pm_log("warp: unknown preset '%s'", v);
    }
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
    smoke_on = pm_debug_has("smoke");
    if (pm_debug_value("loadslot", v, sizeof v)) w_loadslot = atoi(v); /* E02: loadslot=<n> loads save slot n first */
    if (pm_debug_value("smokeframes", v, sizeof v)) smoke_frames = atoi(v); /* E02: smoke length, default 300 */
    padscript_log = pm_debug_has("padlog") || pm_debug_value("padscript", v, sizeof v); /* position every 2 frames while a script runs */
    return has_preset || has_warp || has_story;
}

/* Called from the set_game_mode wrapper. Returns the mode to enter (the caller then calls the real set_game_mode). */
s16 pm_warp_hook(s16 mode) {
    s16 areaID, mapID;
    if (warp_done || mode != GAME_MODE_LOGOS) return mode;
    warp_done = 1;
    if (!pm_warp_active()) return mode;
    if (!get_map_IDs_by_name(w_map, &areaID, &mapID)) {
        pm_log("warp: map %s not found, normal start", w_map);
        return mode;
    }
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
    return GAME_MODE_ENTER_WORLD;
}

void pm_padscript_start(void);
extern unsigned pm_pad_scans;

/* The game reaches GAME_MODE_WORLD: first frame of the chosen scene. */
void pm_warp_world_mode(unsigned frame) {
    if (tour_next && warp_on) {
        tour_f0 = frame; /* a map of the tour is in */
        tour_wait = 0;
        pm_log("tour: in map area %d map %d entry %d frame %u t=%u ms", gGameStatusPtr->areaID, gGameStatusPtr->mapID, gGameStatusPtr->entryID, frame, pm_uptime_ms());
    }
    if (!warp_on || scene_f0 >= 0) return;
    scene_f0 = (int)frame;
    pm_log("warp: scene frame0 %u scan %u t=%u ms", frame, pm_pad_scans, pm_uptime_ms());
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
        pm_log("tour: map %s not in gAreas", name);
        return;
    }
    pm_log("tour: next %s entry %d (area %d map %d)", name, entry, areaID, mapID);
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
    if (f == at && pm_debug_has("demobattle")) {
        pm_log("debug demobattle: enter GAME_MODE_DEMO at demo scene 1 (battle index 0)");
        gGameStatusPtr->demoState = DEMO_STATE_ACTIVE;
        gGameStatusPtr->nextDemoScene = 1;
        set_game_mode(GAME_MODE_DEMO);
    }
}

/* Once per drawn frame (pm_frame_loop). */
void pm_warp_tick(unsigned frame) {
    if (scene_f0 < 0) return;
    if (tour_next && !tour_wait && frame - tour_f0 >= TOUR_FRAMES) {
        tour_ts = frame;
        tour_step();
    } else if (tour_next && tour_wait && frame - tour_ts > 300) { /* the change was refused or never came */
        pm_log("tour: no map change within 300 frames");
        tour_wait = 0;
        tour_f0 = frame - TOUR_FRAMES + 1;
    }
    unsigned f = frame - (unsigned)scene_f0;
    battle_hooks(f);
    char ti[8];
    if (f == 30 && pm_debug_value("testitem", ti, sizeof ti)) { /* V07 debug: a real item entity beside the player */
        make_item_entity_nodelay(ti[0] == 'c' ? ITEM_COIN : ti[0] == 's' ? ITEM_STAR_PIECE : ITEM_MUSHROOM, gPlayerStatus.pos.x + 60.0f, gPlayerStatus.pos.y, gPlayerStatus.pos.z, ITEM_SPAWN_MODE_FIXED_NEVER_VANISH, 0);
    }
    if (f % (padscript_log ? 2 : 100) == 0 || f == 5) {
        pm_log("warp: scene f%u t=%u pos %d %d %d inputDisabled %d actionState %u vy %d maxJump %d walk %d run %d wall %d push %d", f, pm_uptime_ms(), (int)gPlayerStatus.pos.x, (int)gPlayerStatus.pos.y,
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
        pm_log("X02: darkness world %d alpha %d pos %d %d partnerAction %d cur %d", isWorld, alpha, x, y, ls, (int)gPlayerData.curPartner);
    }
}
