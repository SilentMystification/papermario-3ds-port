/* PT67: debug options that change the current save data (gPlayerData) at once: partners, badges, key items, level and stats.
 * Called from the one row table of pm_options.c, so the Select menu and the options Debug tab share them. Nothing is written to the save file here:
 * the data goes to the file only when the player saves. */
#include "common.h"
#include "functions.h"
#include "pm_port.h"

enum { PM_DBG_MAXHP, PM_DBG_MAXFP, PM_DBG_BP, PM_DBG_LEVEL, PM_DBG_COINS, PM_DBG_SP, PM_DBG_NSTAT };
static const struct { int lo, hi, st; } rg[PM_DBG_NSTAT] = { { 5, 75, 5 }, { 0, 75, 5 }, { 0, 30, 1 }, { 1, 27, 1 }, { 0, 999, 10 }, { 0, 99, 1 } };

static int is_item(int id, int flag) { return gItemTable[id].typeFlags & flag; }

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
        for (i = 1; i < 12; i++) p->partners[i].enabled = 1;
        if (p->curPartner == 0) p->curPartner = PARTNER_GOOMBARIO;
    } else {
        int n = g == 1 ? (int)ARRAY_COUNT(p->badges) : (int)ARRAY_COUNT(p->keyItems);
        s16* a = g == 1 ? p->badges : p->keyItems;
        for (i = 1; i < (int)ARRAY_COUNT(gItemTable); i++) {
            if (!is_item(i, g == 1 ? ITEM_TYPE_FLAG_BADGE : ITEM_TYPE_FLAG_KEY)) continue;
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
        case PM_DBG_LEVEL: return p->level;
        case PM_DBG_COINS: return p->coins;
        default: return p->starPoints;
    }
}

/* dir steps by the table step (wraps at both ends); HP and FP are refilled */
void pm_dbgg_stat_step(int k, int dir) {
    PlayerData* p = &gPlayerData;
    int v = pm_dbgg_stat_get(k) + dir * rg[k].st;
    if (dir > 0 && pm_dbgg_stat_get(k) >= rg[k].hi) v = rg[k].lo;
    if (dir < 0 && pm_dbgg_stat_get(k) <= rg[k].lo) v = rg[k].hi;
    if (v < rg[k].lo) v = rg[k].lo;
    if (v > rg[k].hi) v = rg[k].hi;
    switch (k) {
        case PM_DBG_MAXHP: p->hardMaxHP = v; enforce_hpfp_limits(); p->curHP = p->curMaxHP; break;
        case PM_DBG_MAXFP: p->hardMaxFP = v; enforce_hpfp_limits(); p->curFP = p->curMaxFP; break;
        case PM_DBG_BP: p->maxBP = v; break;
        case PM_DBG_LEVEL: p->level = v; break;
        case PM_DBG_COINS: p->coins = v; break;
        default: p->starPoints = v; break;
    }
    pm_log("dbggame: stat %d = %d", k, pm_dbgg_stat_get(k));
}
