/* PT67: debug options that change the current save data (gPlayerData) at once: partners, badges, key items, level and stats.
 * Called from the one row table of pm_options.c, so the Select menu and the options Debug tab share them. Nothing is written to the save file here:
 * the data goes to the file only when the player saves. */
#include "common.h"
#include "functions.h"
#include "pm_port.h"
#include "game_modes.h"
#include "npc.h"
#include <string.h>
#include <stdio.h>

enum { PM_DBG_MAXHP, PM_DBG_MAXFP, PM_DBG_BP, PM_DBG_LEVEL, PM_DBG_COINS, PM_DBG_SP, PM_DBG_STARPC, PM_DBG_FORT, PM_DBG_NSTAT };
static const struct { int lo, hi, st; } rg[PM_DBG_NSTAT] = { { 10, 75, 5 }, { 5, 75, 5 }, { 3, 30, 3 }, { 1, 27, 1 }, { 0, 999, 10 }, { 0, 99, 1 }, { 0, MAX_STAR_PIECES, 1 }, { 0, 99, 1 } }; /* Set Stats steps: HP 5, FP 5, BP 3 (level-up gains); star pieces 0 to the game maximum; fortress keys 0 to 99 */
static const char* const pm_dbgg_stat_name[PM_DBG_NSTAT] = { "max HP", "max FP", "BP", "level", "coins", "star points", "star pieces", "fortress keys" };
/* Level (read only) from the stats: each level-up gives +5 Max HP, +5 Max FP or +3 BP from the level 1 base 10, 5, 3. */
int pm_dbgg_level_calc(void) {
    PlayerData* p = &gPlayerData;
    return 1 + (p->hardMaxHP - 10) / 5 + (p->hardMaxFP - 5) / 5 + (p->maxBP - 3) / 3;
}
/* PROFILE: every change of a field logs profile: <field> <old> -> <new>. RAM only: nothing is written to the save file until the player saves. */
static void profile_log(const char* field, int from, int to) { pm_log("profile: %s %d -> %d", field, from, to); }
/* the written game level is the computed one, capped at the game maximum 27 */
static void level_sync(void) {
    int l = pm_dbgg_level_calc();
    gPlayerData.level = (s16)(l < 1 ? 1 : l > 27 ? 27 : l);
}
int pm_dbgg_level_over(void) { return pm_dbgg_level_calc() > 27; }

/* CHEATSAVE2: the Cheats tab record, the values the cheat file holds beside the switches. rec_cur: the values the Cheats rows set since the load or
 * the last save. rec_saved: the record as loaded or saved last. Dirty (pm_cheatrec_changed) when they differ. Only the rows that set a value are in
 * it: game changes by play (coins won, badges found) are not. Fixed fields F_*: Set Stats, star power, equipment, Merlee, current partner, partner on
 * and partner level. Items: iv[0] badges, iv[1] key items, per item id: 1 on, 2 off, 0 not in the record. */
enum { F_STAT = 0, F_SPW = 8, F_EQ = 11, F_MER = 14, F_CURP = 16, F_PON = 17, F_PLV = 28, F_N = 39 }; /* F_STAT + k: the stat index k of pm_dbgg_stat_get (k 3, Level, is never set) */
#define REC_IT ((int)(sizeof gItemTable / sizeof gItemTable[0]))
typedef struct { int fv[F_N]; unsigned char fset[F_N]; unsigned char iv[2][REC_IT]; } CheatRec;
static CheatRec rec_cur, rec_saved;
static void rec_put(int f, int v) { rec_cur.fv[f] = v; rec_cur.fset[f] = 1; }
static void rec_item(int g, int id, int on) { if (id > 0 && id < REC_IT) rec_cur.iv[g - 1][id] = on ? 1 : 2; }
static const char* const rec_fixed_name[F_PON] = { "stat.maxhp", "stat.maxfp", "stat.bp", "", "stat.coins", "stat.starpts", "stat.starpc", "stat.fort",
    "sp.max", "sp.power", "sp.beam", "eq.boots", "eq.hammer", "eq.action", "mer.spell", "mer.casts", "curpartner" };
/* the file key of field f */
static void rec_key(int f, char* out, unsigned n) {
    if (f < F_PON) snprintf(out, n, "%s", rec_fixed_name[f]);
    else if (f < F_PLV) snprintf(out, n, "partner.%d.on", f - F_PON + 1);
    else snprintf(out, n, "partner.%d.lv", f - F_PLV + 1);
}
/* the field of a file key, -1 when none */
static int rec_find(const char* key) {
    char nm[24];
    for (int f = 0; f < F_N; f++) {
        if (f == F_STAT + 3) continue;
        rec_key(f, nm, sizeof nm);
        if (!strcmp(nm, key)) return f;
    }
    return -1;
}
/* read: one key=value line of the cheat file (pm_cheatsave.c). 1 when the key is a record key; the switch keys are not here. */
int pm_cheatrec_load_key(const char* key, int v) {
    int g = 0, f;
    if (!strncmp(key, "badge.", 6)) { g = 1; key += 6; }
    else if (!strncmp(key, "key.", 4)) { g = 2; key += 4; }
    if (g) {
        const char* d = key;
        long id = 0;
        for (; *d >= '0' && *d <= '9'; d++) id = id * 10 + (*d - '0');
        if (d == key || *d || id <= 0 || id >= REC_IT) return 0;
        rec_cur.iv[g - 1][id] = v ? 1 : 2;
        return 1;
    }
    f = rec_find(key);
    if (f < 0) return 0;
    rec_put(f, v);
    return 1;
}
/* write: entries 0 to pm_cheatrec_count-1. 1 when entry i is in the record: its key and value (an item: 1 on, 0 off). */
int pm_cheatrec_count(void) { return F_N + 2 * REC_IT; }
int pm_cheatrec_entry(int i, char* key, unsigned n, int* v) {
    int g;
    if (i < F_N) {
        if (i == F_STAT + 3 || !rec_cur.fset[i]) return 0;
        rec_key(i, key, n);
        *v = rec_cur.fv[i];
        return 1;
    }
    i -= F_N;
    g = i / REC_IT;
    i %= REC_IT;
    if (!rec_cur.iv[g][i]) return 0;
    snprintf(key, n, "%s.%d", g ? "key" : "badge", i);
    *v = rec_cur.iv[g][i] == 1;
    return 1;
}
int pm_cheatrec_changed(void) { return memcmp(&rec_cur, &rec_saved, sizeof rec_cur) != 0; }
void pm_cheatrec_commit(void) { rec_saved = rec_cur; } /* the record as loaded or saved */
/* the apply waits for the first world step: the save data of the slot is still loaded after the mode change to ENTER_WORLD */
static int rec_pending;
void pm_cheatrec_pend(void) { rec_pending = 1; }
void pm_cheatrec_apply(void);
void pm_cheatrec_clear(void) { memset(&rec_cur, 0, sizeof rec_cur); rec_saved = rec_cur; }

/* PT73: the flag alone also matches entries outside the real ranges (one flagged entry sits before ITEM_FIRST_BADGE, it sorts first and the pause badge list
 * stops at it). The pause menus read only IS_BADGE ids (ITEM_FIRST_BADGE to ITEM_LAST_BADGE) and key items ITEM_FIRST_KEY to ITEM_LAST_KEY. */
static int is_item(int id, int flag) {
    if (flag == ITEM_TYPE_FLAG_BADGE ? !IS_BADGE(id) : !(id >= ITEM_FIRST_KEY && id <= ITEM_LAST_KEY)) return 0;
    if (gItemTable[id].nameMsg == 0) return 0; /* unused entry: the port data has no name message (MSG_ItemName_x is 0), the pause list shows a blank row with BP 0 */
    return gItemTable[id].typeFlags & flag;
}

/* CH0: the item names, indexed by item id (item_name_table.h, generated by tools/gen_item_names.py, which checks the ids are 0..n-1). */
static const char* const item_names[] = {
#include "item_name_table.h"
};
const char* pm_item_name(int id) { return (unsigned)id < ARRAY_COUNT(item_names) ? item_names[id] : "?"; }
void pm_dbgg_count(int g, int* have, int* total); void pm_dbgg_give(int g);
s32 pause_badges_try_remove(s16 badgeID); /* src/pause/pause_badges.c: removes the badge from equippedBadges, 1 when it was there */

/* CH2: the rows of a group (1 badges, 2 key items) are the items is_item accepts, in id order. Row k is item k of that list. */
int pm_dbgg_item_count(int g) {
    int i, n = 0;
    for (i = 1; i < (int)ARRAY_COUNT(gItemTable); i++) if (is_item(i, g == 1 ? ITEM_TYPE_FLAG_BADGE : ITEM_TYPE_FLAG_KEY)) n++;
    return n;
}
int pm_dbgg_item_id(int g, int row) {
    int i, n = 0;
    for (i = 1; i < (int)ARRAY_COUNT(gItemTable); i++) {
        if (!is_item(i, g == 1 ? ITEM_TYPE_FLAG_BADGE : ITEM_TYPE_FLAG_KEY)) continue;
        if (n++ == row) return i;
    }
    return 0;
}
int pm_dbgg_item_has(int g, int id) {
    PlayerData* p = &gPlayerData;
    s16* a = g == 1 ? p->badges : p->keyItems;
    int j, n = g == 1 ? (int)ARRAY_COUNT(p->badges) : (int)ARRAY_COUNT(p->keyItems);
    for (j = 0; j < n; j++) if (a[j] == id) return 1;
    return 0;
}
/* CH2: on adds id to the first empty slot. Off unequips it first (badges only: pause_badges_try_remove) and closes the hole in the list. */
void pm_dbgg_item_set(int g, int id, int on) {
    rec_item(g, id, on);
    PlayerData* p = &gPlayerData;
    s16* a = g == 1 ? p->badges : p->keyItems;
    int j, n = g == 1 ? (int)ARRAY_COUNT(p->badges) : (int)ARRAY_COUNT(p->keyItems), u = 0;
    if (on) {
        if (pm_dbgg_item_has(g, id)) return;
        for (j = 0; j < n; j++) if (a[j] == ITEM_NONE) { a[j] = (s16)id; break; }
        pm_log("dbggame: %s %d on", g == 1 ? "badge" : "key", id);
        return;
    }
    if (g == 1) u = pause_badges_try_remove((s16)id);
    for (j = 0; j < n; j++) if (a[j] == id) break;
    if (j == n) return;
    for (; j < n - 1; j++) a[j] = a[j + 1];
    a[n - 1] = ITEM_NONE;
    pm_log("dbggame: %s %d off%s", g == 1 ? "badge" : "key", id, u ? " (unequipped)" : "");
}
/* CH2: Toggle all. None of the group owned: all owned (as give). Otherwise all removed, each equipped badge unequipped first. */
void pm_dbgg_toggle_all(int g) {
    int i, h, t;
    pm_dbgg_count(g, &h, &t);
    if (h == 0) { pm_dbgg_give(g); return; }
    for (i = 0; i < pm_dbgg_item_count(g); i++) pm_dbgg_item_set(g, pm_dbgg_item_id(g, i), 0);
    pm_log("dbggame: toggle all %d off", g);
}

void switch_to_partner(s32 arg0); /* src/world/partners.c, live (menus.c) */
/* CH4: partners 1 to 11 (PARTNER_GOOMBARIO .. PARTNER_TWINK, include/enums.h). Index 0 is no partner. */
static const char* const partner_names[12] = { "", "Goombario", "Kooper", "Bombette", "Parakarry", "Goompa", "Watt", "Sushie", "Lakilester", "Bow", "Goombaria", "Twink" };
const char* pm_partner_name(int i) { return i >= 1 && i <= 11 ? partner_names[i] : "?"; }
/* the value of a partner row: Off, or its level (0 Normal, 1 Super, 2 Ultra) */
const char* pm_partner_value(int i) {
    static const char* const lv[3] = { "Normal", "Super", "Ultra" };
    PlayerData* p = &gPlayerData;
    int l = p->partners[i].level;
    if (!p->partners[i].enabled) return "Off";
    return lv[l < 0 ? 0 : l > 2 ? 2 : l];
}
int pm_dbgg_partner_on(int i) { return gPlayerData.partners[i].enabled != 0; }
/* CH4: on or off. Off on the current partner moves curPartner to the next enabled partner, or PARTNER_NONE. The world partner entity is not switched
 * here: see pm_dbgg_partner_set. */
/* CH4 gap: the world partner entity follows curPartner. switch_to_partner is the live path of the game (world menu, src/world/partners.c):
 * it queues the switch (take out, switch or put away) for the next world step, so the frozen menu only queues it. Other modes: the entity
 * is set at the next world entry. */
static void partner_entity(void) {
    if (get_game_mode() == GAME_MODE_WORLD) {
        switch_to_partner(gPlayerData.curPartner);
        pm_log("profile: partner entity queued %d", gPlayerData.curPartner);
    } else pm_log("profile: partner entity at the next world entry (mode %d)", get_game_mode());
}
void pm_dbgg_partner_set(int i, int on) {
    PlayerData* p = &gPlayerData;
    int j, was = p->partners[i].enabled != 0;
    p->partners[i].enabled = on ? 1 : 0;
    if (on && p->curPartner == 0) p->curPartner = (s8)i;
    if (!on && p->curPartner == i) {
        p->curPartner = PARTNER_NONE;
        for (j = 1; j < 12; j++) if (j != i && p->partners[j].enabled) { p->curPartner = (s8)j; break; }
    }
    rec_put(F_PON + i - 1, on ? 1 : 0);
    rec_put(F_CURP, p->curPartner);
    { char f[48]; snprintf(f, sizeof f, "%s enabled", pm_partner_name(i)); profile_log(f, was, on ? 1 : 0); }
    partner_entity();
}
/* CH4: left or right steps the level of an enabled partner (wrap). Returns 1 when the row changed. */
int pm_dbgg_partner_level(int i, int dir) {
    PlayerData* p = &gPlayerData;
    int l;
    if (!p->partners[i].enabled) return 0;
    l = ((p->partners[i].level + dir) % 3 + 3) % 3;
    { char f[48]; snprintf(f, sizeof f, "%s upgrade", pm_partner_name(i)); profile_log(f, p->partners[i].level, l); } /* 0 Normal, 1 Super, 2 Ultra */
    p->partners[i].level = (s8)l;
    rec_put(F_PLV + i - 1, l);
    return 1;
}

/* CH5: Spawn item. The rows are the consumables with a name (ITEM_FIRST_CONSUMABLE .. ITEM_LAST_CONSUMABLE). One A spawns one item over Mario
 * (plan answer Q2, row_MENU.md). The entity is made in the frozen menu; the game moves it at the resume. World mode only. */
int pm_dbgg_spawn_count(void) {
    int i, n = 0;
    for (i = ITEM_FIRST_CONSUMABLE; i <= ITEM_LAST_CONSUMABLE; i++) if (gItemTable[i].nameMsg != 0) n++;
    return n;
}
int pm_dbgg_spawn_id(int row) {
    int i, n = 0;
    for (i = ITEM_FIRST_CONSUMABLE; i <= ITEM_LAST_CONSUMABLE; i++) {
        if (gItemTable[i].nameMsg == 0) continue;
        if (n++ == row) return i;
    }
    return ITEM_NONE;
}
void pm_menu_msg(const char* fmt, ...);
void pm_dbgg_spawn(int id) {
    PlayerStatus* ps = &gPlayerStatus;
    if (get_game_mode() != GAME_MODE_WORLD) { pm_menu_msg("Spawn item needs the world"); return; }
    make_item_entity_nodelay(id, ps->pos.x, ps->pos.y + 40.0f, ps->pos.z, ITEM_SPAWN_MODE_FALL_NEVER_VANISH, 0);
    pm_log("dbggame: spawn item 0x%03X at %.1f %.1f %.1f", id, ps->pos.x, ps->pos.y + 40.0f, ps->pos.z);
}

/* FADV: Frame advance row. pm_fa_on is the requested state (set here, read by the main loop at the menu close: pm_main.c fa_state). On: the game holds
 * until R steps it one frame (pm_pad_take_step). The menu is opened with Select as before. */
int pm_fa_on;
int pm_fa_get(void) { return pm_fa_on; }
void pm_fa_set(int on) { pm_fa_on = on ? 1 : 0; pm_log("fa: %s", pm_fa_on ? "on" : "off"); }

/* CH6: Moon Jump (RAM only, not saved). The launch switch moonjump turns it on at boot. Per step (pm_warp_tick, after the game logic) while
 * A is held and Mario jumps or falls: the upward velocity is at least 4 and the other integrator terms are zero, so the jump floats. */
static int moon_on = -1;
int pm_cheat_moon_get(void) {
    if (moon_on < 0) moon_on = pm_debug_has("moonjump") ? 1 : 0;
    return moon_on;
}
void pm_cheat_moon_set(int on) {
    moon_on = on ? 1 : 0;
    pm_log("cheat: moon jump %s", moon_on ? "on" : "off");
}
/* CH7: batch 1 cheats (RAM only): bits in cheat_bits, set at boot by the launch value cheats=<list> (hp, fp, sp, nobattle, speed, comma separated).
 * The tick runs once per step (pm_warp_tick). Each cheat logs once when it first runs, and a refill logs its change. */
enum { CHB_HP = 1, CHB_FP = 2, CHB_SP = 4, CHB_NOBATTLE = 8, CHB_SPEED = 16, CHB_ACTION = 32 };
#define PM_CHEAT_ACTION 5 /* index of the Perfect action commands row (pm_options.c btl_cheat) */
static const char* const cheat_name[6] = { "infinite HP", "infinite FP", "infinite star power", "no battles", "speed x2", "perfect action commands" };
static const int cheat_bit[6] = { CHB_HP, CHB_FP, CHB_SP, CHB_NOBATTLE, CHB_SPEED, CHB_ACTION };
static int cheat_bits = -1, cheat_seen;
static unsigned boot_bits; /* CHEATSAVE: the bits the boot switch cheats= set (cheat_init) */
/* a whole token of the comma list (sp is not speed) */
static int has_token(const char* list, const char* tok) {
    size_t n = strlen(tok);
    for (const char* p = list; (p = strstr(p, tok)) != NULL; p += n)
        if ((p == list || p[-1] == ',') && (p[n] == 0 || p[n] == ',')) return 1;
    return 0;
}
static void cheat_init(void) {
    char v[96];
    if (cheat_bits >= 0) return;
    cheat_bits = 0;
    if (pm_debug_value("cheats", v, sizeof v)) {
        if (has_token(v, "hp")) cheat_bits |= CHB_HP;
        if (has_token(v, "fp")) cheat_bits |= CHB_FP;
        if (has_token(v, "sp")) cheat_bits |= CHB_SP;
        if (has_token(v, "nobattle")) cheat_bits |= CHB_NOBATTLE;
        if (has_token(v, "speed")) cheat_bits |= CHB_SPEED;
        if (has_token(v, "action")) cheat_bits |= CHB_ACTION;
    }
    boot_bits = (unsigned)cheat_bits;
}
/* CHEATSAVE: the persistent set (pm_cheatsave.c). Bit k is the pm_cheat_get index k (cheat_bit[k] = 1 << k), bit 6 is moon jump. */
unsigned pm_cheat_persist_get(void) { cheat_init(); return ((unsigned)cheat_bits & 0x3Fu) | ((unsigned)pm_cheat_moon_get() << 6); }
void pm_cheat_persist_set(unsigned bits) { /* no log: the load line of pm_cheatsave.c says what changed */
    cheat_init();
    cheat_bits = (cheat_bits & ~0x3F) | (int)(bits & 0x3Fu);
    moon_on = (int)((bits >> 6) & 1u);
}
/* CHEATSAVE: the bits the boot switches set for this run (cheats=<list>, moonjump). A load ORs them in: a boot switch wins over the file. */
unsigned pm_cheat_boot_bits(void) { cheat_init(); return boot_bits | (pm_debug_has("moonjump") ? 1u << 6 : 0u); }
int pm_cheat_get(int k) { cheat_init(); return (cheat_bits & cheat_bit[k]) != 0; }
/* Perfect action commands (CH9): read by action_command_init_status and check_block_input (src/battle/action_cmd.c) under TARGET_3DS */
int pm_cheat_action_on(void) { return pm_cheat_get(PM_CHEAT_ACTION); }
void pm_cheat_set(int k, int on) {
    cheat_init();
    if (on) cheat_bits |= cheat_bit[k]; else cheat_bits &= ~cheat_bit[k];
    pm_log("cheat: %s %s", cheat_name[k], on ? "on" : "off");
}
extern f32 GravityParamsStartJump[]; /* src/world/actions.c */
#define MOON_RISE (GravityParamsStartJump[0] * 0.5f) /* MOON3: the rise velocity of a hold or a tap push, half the jump start */
void pm_cheat_tick(void) {
    PlayerStatus* ps = &gPlayerStatus;
    PlayerData* p = &gPlayerData;
    cheat_init();
    if (rec_pending && get_game_mode() == GAME_MODE_WORLD) { rec_pending = 0; pm_cheatrec_apply(); }
    if (cheat_bits & CHB_HP && p->curHP != p->curMaxHP) { pm_log("cheat: infinite HP refill %d -> %d", (int)p->curHP, (int)p->curMaxHP); p->curHP = p->curMaxHP; }
    if (cheat_bits & CHB_FP && p->curFP != p->curMaxFP) { pm_log("cheat: infinite FP refill %d -> %d", (int)p->curFP, (int)p->curMaxFP); p->curFP = p->curMaxFP; }
    if (cheat_bits & CHB_SP) p->starPower = (s16)(p->maxStarPower * 256);
    if (cheat_bits & CHB_NOBATTLE) gCurrentEncounter.battleTriggerCooldown = 2;
    if (cheat_bits & CHB_SPEED) { ps->walkSpeed *= 2.0f; ps->runSpeed *= 2.0f; } /* the action code sets the speeds each step, so this does not compound */
    if ((cheat_bits & ~cheat_seen) != 0) {
        int k;
        for (k = 0; k < 6; k++) if ((cheat_bits & ~cheat_seen) & cheat_bit[k]) pm_log("cheat: %s active, star power %d, walk %.2f", cheat_name[k], (int)p->starPower, ps->walkSpeed);
        cheat_seen = cheat_bits;
    }
    /* MOON3 (user): with the moon jump on, A raises Mario in the air (JUMPING or FALLING, not FLYING). A press of A (the rising edge) gives an
     * upward push: if he is falling, the action goes back to JUMP (flags JUMPING on, FALLING off; the action is not re-initialized, because
     * set_action_state would reset the integrator to the full start jump on the next step). The push is velocity MOON_RISE (half of
     * GravityParamsStartJump[0]) with the normal arc accelerations, so the arc decays. Holding A in JUMP keeps the velocity at MOON_RISE with
     * accelerations 0, so he rises at a constant MOON_RISE per step, and timeInAir is held at 1 (update_player puts Mario back on lastGoodPos
     * above 100). The game's own release (A up in JUMP switches to HOP, a fast fall) runs as before. On release in JUMP the normal arc resumes
     * (the accelerations go back to the jump start values). Cheat off: no change. */
    {
        static int moon_held;
        const int moon = pm_cheat_moon_get();
        const int held = (ps->curButtons & BUTTON_A) != 0;
        const int air = ((ps->flags & (PS_FLAG_JUMPING | PS_FLAG_FALLING)) != 0) && !(ps->flags & PS_FLAG_FLYING);
        const int pushed = moon && air && (ps->pressedButtons & BUTTON_A) != 0;
        int rising;
        if (pushed && ps->actionState != ACTION_STATE_JUMP) {
            ps->prevActionState = ps->actionState;
            ps->actionState = ACTION_STATE_JUMP;
            ps->flags &= ~PS_FLAG_FALLING;
            ps->flags |= PS_FLAG_JUMPING;
            pm_log("cheat: moon jump re-entered JUMP from a fall, y %.1f", ps->pos.y);
        }
        if (pushed) pm_log("cheat: moon jump tap y %.1f", ps->pos.y);
        rising = moon && air && held && ps->actionState == ACTION_STATE_JUMP;
        if (rising) {
            ps->gravityIntegrator[0] = MOON_RISE;
            ps->gravityIntegrator[1] = 0.0f;
            ps->gravityIntegrator[2] = 0.0f;
            ps->gravityIntegrator[3] = 0.0f;
            ps->timeInAir = 1;
            pm_log_d("cheat: moon jump y %.1f x %.1f", ps->pos.y, ps->pos.x);
        } else {
            if (air && moon_held && moon && ps->actionState == ACTION_STATE_JUMP) {
                ps->gravityIntegrator[1] = GravityParamsStartJump[1];
                ps->gravityIntegrator[2] = GravityParamsStartJump[2];
                ps->gravityIntegrator[3] = GravityParamsStartJump[3];
                pm_log("cheat: moon jump release y %.1f", ps->pos.y);
            }
            if (air) pm_log_d("cheat: jump y %.1f", ps->pos.y);
        }
        moon_held = rising;
    }
}

/* PROFILE: the Cheats tab folders that edit Mario's profile (gPlayerData, RAM only). Labels and rows are in pm_options.c; these are the values,
 * the steps and the actions. Values clamp to the range the game uses (the item pickup, battle and menu code). */
static const char* const boots_names[4] = { "None", "Normal", "Super", "Ultra" }; /* bootsLevel -1 .. 2 */
static const char* const mer_names[5] = { "None", "Attack boost", "Defense boost", "EXP boost", "Coin boost" }; /* MERLEE_SPELL_NONE .. COIN_BOOST */
static const char* const beam_names[3] = { "None", "Star Beam", "Peach Beam" }; /* starBeamLevel 0 .. 2 */
static int inv_row, pos_saved, save_area, save_map, save_entry;
static float save_x, save_y, save_z;
static int lvl_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int wrap_step(int v, int dir, int lo, int hi) { /* one step, wrap from hi to lo and lo to hi */
    v += dir;
    if (v < lo) v = hi;
    if (v > hi) v = lo;
    return v;
}
/* Equipment: 0 boots and 1 hammer (None, Normal, Super, Ultra = -1 .. 2, wrap), 2 action commands (hasActionCommands, On or Off). */
const char* pm_dbgg_eq_value(int k) {
    PlayerData* p = &gPlayerData;
    if (k == 0) return boots_names[lvl_clamp(p->bootsLevel, -1, 2) + 1];
    if (k == 1) return boots_names[lvl_clamp(p->hammerLevel, -1, 2) + 1];
    return p->hasActionCommands ? "On" : "Off";
}
void pm_dbgg_eq_step(int k, int dir) {
    PlayerData* p = &gPlayerData;
    int old, v;
    if (k == 0) { old = p->bootsLevel; v = wrap_step(old, dir, -1, 2); p->bootsLevel = (s8)v; rec_put(F_EQ, v); profile_log("boots level", old, v); }
    else if (k == 1) { old = p->hammerLevel; v = wrap_step(old, dir, -1, 2); p->hammerLevel = (s8)v; rec_put(F_EQ + 1, v); profile_log("hammer level", old, v); }
    else { old = p->hasActionCommands; p->hasActionCommands = !old; rec_put(F_EQ + 2, !old); profile_log("action commands", old, !old); }
}
/* Star power: 0 max star spirits (0 .. 7), 1 star power (0 .. spirits * 256, a step is one spirit), 2 star beam (None, Star Beam, Peach Beam). */
const char* pm_dbgg_spw_value(int k) {
    static char b[32];
    PlayerData* p = &gPlayerData;
    if (k == 0) snprintf(b, sizeof b, "%d", (int)p->maxStarPower);
    else if (k == 1) snprintf(b, sizeof b, "%d / %d", (int)p->starPower / 256, (int)p->maxStarPower);
    else return beam_names[lvl_clamp(p->starBeamLevel, 0, 2)];
    return b;
}
void pm_dbgg_spw_step(int k, int dir) {
    PlayerData* p = &gPlayerData;
    int old, v;
    if (k == 0) {
        old = p->maxStarPower; v = wrap_step(old, dir, 0, 7); p->maxStarPower = (s8)v; rec_put(F_SPW, v);
        if (p->starPower > v * 256) { p->starPower = (s16)(v * 256); rec_put(F_SPW + 1, p->starPower); } /* the power never exceeds the spirits */
        profile_log("max star spirits", old, v);
    } else if (k == 1) {
        old = p->starPower; v = lvl_clamp(old + dir * 256, 0, p->maxStarPower * 256); p->starPower = (s16)v; rec_put(F_SPW + 1, v);
        profile_log("star power", old, v);
    } else {
        old = p->starBeamLevel; v = wrap_step(old, dir, 0, 2); p->starBeamLevel = (s8)v; rec_put(F_SPW + 2, v);
        profile_log("star beam", old, v);
    }
}
/* Inventory: 0 the item of the row (a consumable; the row steps the list), 1 fill all 10 slots with it. */
const char* pm_dbgg_inv_value(int k) { return k == 0 ? pm_item_name(pm_dbgg_spawn_id(inv_row)) : "A: fill"; }
void pm_dbgg_inv_step(int dir) { int n = pm_dbgg_spawn_count(); inv_row = ((inv_row + dir) % n + n) % n; }
void pm_dbgg_inv_fill(void) {
    PlayerData* p = &gPlayerData;
    int j, id = pm_dbgg_spawn_id(inv_row);
    for (j = 0; j < 10; j++) p->invItems[j] = (s16)id;
    pm_log("profile: inventory filled with 0x%03X %s", id, pm_item_name(id));
}
/* Merlee: 0 spell type (the five spells), 1 casts left (0 .. 9). */
const char* pm_dbgg_mer_value(int k) {
    static char b[16];
    PlayerData* p = &gPlayerData;
    if (k == 0) return mer_names[lvl_clamp(p->merleeSpellType, 0, 4)];
    snprintf(b, sizeof b, "%d", (int)p->merleeCastsLeft);
    return b;
}
void pm_dbgg_mer_step(int k, int dir) {
    PlayerData* p = &gPlayerData;
    int old, v;
    if (k == 0) { old = p->merleeSpellType; v = wrap_step(old, dir, 0, 4); p->merleeSpellType = (s8)v; rec_put(F_MER, v); profile_log("merlee spell", old, v); }
    else { old = p->merleeCastsLeft; v = wrap_step(old, dir, 0, 9); p->merleeCastsLeft = (s8)v; rec_put(F_MER + 1, v); profile_log("merlee casts left", old, v); }
}
/* Battle: the instant win (battle victory state, in battle only). The infinite and perfect rows are the pm_cheat_* bits (pm_options.c). */
void pm_dbgg_instant_win(void) {
    if (get_game_mode() != GAME_MODE_BATTLE) { pm_menu_msg("Instant win needs a battle"); return; }
    btl_set_state(BATTLE_STATE_VICTORY);
    pm_log("profile: instant win");
}
/* Position: 0 Speed x2 (the cheat bit, pm_options.c), 1 save position, 2 load position (RAM only, same map). */
const char* pm_dbgg_pos_value(int k) {
    if (k == 1) return pos_saved ? "saved" : "A: save";
    return pos_saved ? "A: load" : "none saved";
}
void pm_dbgg_pos_save(void) { /* area, map, entry and the position */
    PlayerStatus* ps = &gPlayerStatus;
    save_area = gGameStatusPtr->areaID; save_map = gGameStatusPtr->mapID; save_entry = gGameStatusPtr->entryID;
    save_x = ps->pos.x; save_y = ps->pos.y; save_z = ps->pos.z; pos_saved = 1;
    pm_log("profile: position saved, map %d:%d entry %d", save_area, save_map, save_entry);
}
void pm_dbgg_pos_load(void) { /* only on the saved map, else a message */
    PlayerStatus* ps = &gPlayerStatus;
    if (!pos_saved) { pm_menu_msg("No position saved"); return; }
    if (gGameStatusPtr->areaID != save_area || gGameStatusPtr->mapID != save_map) { pm_log_w("profile: position is for another map"); pm_menu_msg("Saved position is on another map"); return; }
    ps->pos.x = save_x; ps->pos.y = save_y; ps->pos.z = save_z;
    pm_log("profile: position loaded x %.1f y %.1f z %.1f", save_x, save_y, save_z);
}
/* Partners: Current partner cycles None and the enabled partners (wrap). The world partner entity follows (see pm_dbgg_partner_set). */
const char* pm_dbgg_curpartner_value(void) {
    int c = gPlayerData.curPartner;
    return c >= 1 && c <= 11 ? pm_partner_name(c) : "None";
}
void pm_dbgg_curpartner_step(int dir) {
    PlayerData* p = &gPlayerData;
    int list[12], n = 0, i, pos = 0, old = p->curPartner;
    list[n++] = PARTNER_NONE;
    for (i = 1; i < 12; i++) if (p->partners[i].enabled) list[n++] = i;
    for (i = 0; i < n; i++) if (list[i] == old) pos = i;
    p->curPartner = (s8)list[(pos + (dir > 0 ? 1 : n - 1)) % n];
    rec_put(F_CURP, p->curPartner);
    profile_log("current partner", old, p->curPartner);
    partner_entity();
}

/* owned and total of a group: 0 partners, 1 badges, 2 key items */
void pm_dbgg_count(int g, int* have, int* total) {
    int i, h = 0, t = 0;
    PlayerData* p = &gPlayerData;
    if (g == 0) {
        for (i = 1; i < 12; i++) { t++; h += p->partners[i].enabled != 0; }
    } else {
        for (i = 1; i < (int)ARRAY_COUNT(gItemTable); i++) {
            int j, n = g == 1 ? (int)ARRAY_COUNT(p->badges) : (int)ARRAY_COUNT(p->keyItems);
            s16* a = g == 1 ? p->badges : p->keyItems;
            if (!is_item(i, g == 1 ? ITEM_TYPE_FLAG_BADGE : ITEM_TYPE_FLAG_KEY)) continue;
            t++;
            for (j = 0; j < n; j++) if (a[j] == i) { h++; break; }
        }
    }
    *have = h; *total = t;
}

void pm_dbgg_give(int g) {
    int i, j;
    PlayerData* p = &gPlayerData;
    if (g == 0) { /* 11 partners; level 0 stays the base level */
        for (i = 1; i < 12; i++) { p->partners[i].enabled = 1; rec_put(F_PON + i - 1, 1); }
        if (p->curPartner == 0) { p->curPartner = PARTNER_GOOMBARIO; rec_put(F_CURP, p->curPartner); }
    } else {
        int n = g == 1 ? (int)ARRAY_COUNT(p->badges) : (int)ARRAY_COUNT(p->keyItems);
        s16* a = g == 1 ? p->badges : p->keyItems;
        for (i = 1; i < (int)ARRAY_COUNT(gItemTable); i++) {
            if (!is_item(i, g == 1 ? ITEM_TYPE_FLAG_BADGE : ITEM_TYPE_FLAG_KEY)) continue;
            rec_item(g, i, 1);
            for (j = 0; j < n; j++) if (a[j] == i) break;
            if (j < n) continue; /* owned */
            for (j = 0; j < n; j++) if (a[j] == ITEM_NONE) { a[j] = i; break; }
        }
    }
    pm_dbgg_count(g, &i, &j);
    pm_log("dbggame: give %d -> owned %d of %d", g, i, j);
}

int pm_dbgg_stat_get(int k) {
    PlayerData* p = &gPlayerData;
    switch (k) {
        case PM_DBG_MAXHP: return p->hardMaxHP;
        case PM_DBG_MAXFP: return p->hardMaxFP;
        case PM_DBG_BP: return p->maxBP;
        case PM_DBG_LEVEL: return pm_dbgg_level_calc(); /* read only: computed from the stats */
        case PM_DBG_COINS: return p->coins;
        case PM_DBG_STARPC: return p->starPieces;
        case PM_DBG_FORT: return p->fortressKeyCount;
        default: return p->starPoints;
    }
}

/* dir steps by the table step (wraps at both ends); HP and FP are refilled */
/* sets stat k to v (already in range): HP and FP refill, the game level follows the stats. Used by the rows and by the load. */
static void stat_put(int k, int v) {
    PlayerData* p = &gPlayerData;
    switch (k) {
        case PM_DBG_MAXHP: p->hardMaxHP = v; enforce_hpfp_limits(); p->curHP = p->curMaxHP; break;
        case PM_DBG_MAXFP: p->hardMaxFP = v; enforce_hpfp_limits(); p->curFP = p->curMaxFP; break;
        case PM_DBG_BP: p->maxBP = v; break;
        case PM_DBG_LEVEL: p->level = v; break;
        case PM_DBG_COINS: p->coins = v; break;
        case PM_DBG_STARPC: p->starPieces = (u8)v; break;
        case PM_DBG_FORT: p->fortressKeyCount = (s8)v; break;
        default: p->starPoints = v; break;
    }
    if (k <= PM_DBG_BP) level_sync(); /* a stat row changed: the game level follows */
}
/* dir steps by the table step (wraps at both ends); HP and FP are refilled */
void pm_dbgg_stat_step(int k, int dir) {
    int old, v = pm_dbgg_stat_get(k) + dir * rg[k].st;
    if (dir > 0 && pm_dbgg_stat_get(k) >= rg[k].hi) v = rg[k].lo;
    if (dir < 0 && pm_dbgg_stat_get(k) <= rg[k].lo) v = rg[k].hi;
    if (v < rg[k].lo) v = rg[k].lo;
    if (v > rg[k].hi) v = rg[k].hi;
    old = pm_dbgg_stat_get(k);
    stat_put(k, v);
    rec_put(F_STAT + k, pm_dbgg_stat_get(k));
    profile_log(pm_dbgg_stat_name[k], old, pm_dbgg_stat_get(k));
}

/* CHEATSAVE2: the loaded record applied to gPlayerData at each entry into the world (pm_cheat_enter_world, after the save data is loaded). Raw sets:
 * no partner entity switch (the world spawns the partner of curPartner), no profile log per row. One log line per item: was and now (owned). */
void pm_cheatrec_apply(void) {
    PlayerData* p = &gPlayerData;
    int f, g, id, n = 0;
    for (f = 0; f < F_N; f++) {
        int v;
        if (f == F_STAT + 3 || !rec_cur.fset[f]) continue;
        v = rec_cur.fv[f];
        n++;
        if (f < F_SPW) stat_put(f - F_STAT, v);
        else if (f == F_SPW) p->maxStarPower = (s8)v;
        else if (f == F_SPW + 1) p->starPower = (s16)v;
        else if (f == F_SPW + 2) p->starBeamLevel = (s8)v;
        else if (f == F_EQ) p->bootsLevel = (s8)v;
        else if (f == F_EQ + 1) p->hammerLevel = (s8)v;
        else if (f == F_EQ + 2) p->hasActionCommands = v != 0;
        else if (f == F_MER) p->merleeSpellType = (s8)v;
        else if (f == F_MER + 1) p->merleeCastsLeft = (s8)v;
        else if (f == F_CURP) p->curPartner = (s8)v;
        else if (f < F_PLV) p->partners[f - F_PON + 1].enabled = v != 0;
        else p->partners[f - F_PLV + 1].level = (s8)v;
    }
    for (g = 0; g < 2; g++) {
        for (id = 1; id < REC_IT; id++) {
            int on = rec_cur.iv[g][id], was;
            if (!on) continue;
            n++;
            was = pm_dbgg_item_has(g + 1, id);
            pm_dbgg_item_set(g + 1, id, on == 1);
            pm_log("cheats: %s %d was %d now %d", g ? "key" : "badge", id, was, pm_dbgg_item_has(g + 1, id));
        }
    }
    pm_log("cheats: applied %d record fields", n);
}
