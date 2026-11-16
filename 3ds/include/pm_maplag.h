/* MAPLAG (a) (user 2026-11-16): the fade-in of a map change (src/state_map_transitions.c:190, CHANGE_MAP_FADE_IN) keeps the front overlay at
 * full cover (progress 255) PM_MAPLAG_HOLD more steps. Under full cover the walker builds the new map's model records (pm_xfer.c pm_xfer_skip,
 * 64 a frame) and the hidden models' lists (pm_xfer_hidden, 12 a frame), so that work leaves the visible fade-in frames. pm_map.c arms the hold
 * at each map load and wraps update_enter_map_screen_overlay. Only GAME_MODE_CHANGE_MAP holds; another mode (ENTER_WORLD) clears it. */
#pragma once
#define PM_MAPLAG_HOLD 3
/* One call per update_enter_map_screen_overlay call. hold: the steps left. chg: the game mode is GAME_MODE_CHANGE_MAP. full: the progress
 * was 255 at the call. Returns 1 when the caller must set the progress back to 255 and return 0 (not done). */
static inline int pm_maplag_keep(int* hold, int chg, int full) {
    if (!chg) { *hold = 0; return 0; }
    if (!full || *hold <= 0) return 0;
    (*hold)--;
    return 1;
}
