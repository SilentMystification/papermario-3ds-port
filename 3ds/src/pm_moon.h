/* MOON6: the game's midair window (src/7BB60.c:1095-1097: peakJumpTime >= 6, timeInAir < 18, A pressed) has priority. It opens at the apex.
 * Before the apex (peakJumpTime 0) the moon jump waits: a moon rise there would move the apex. A press in an open window is the game's (spin or
 * tornado jump, PS_FLAG_FLYING), so the cheat does not see it. The moon hold does not lower timeInAir: the window closes at 18 as in a normal jump. */
#pragma once
#define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
#define PM_MOON_AIR_TIME 18 /* the moon hold caps timeInAir here (src/77480.c:614 respawns above 100) */
static inline int pm_moon_allowed(int bootsLevel, int timeInAir, int peakJumpTime) { return bootsLevel < 1 || peakJumpTime > 0 || timeInAir >= PM_MOON_WIN_END; }
static inline int pm_moon_air_cap(int timeInAir) { return timeInAir > PM_MOON_AIR_TIME ? PM_MOON_AIR_TIME : timeInAir; }
#define PM_MOON_STICK_MAX 70.0f /* player_input_to_move_vector clamps the stick magnitude here (src/7B440.c:147) */
/* MOON5: the air speed of a moon jump: the run speed times the stick share */
static inline float pm_moon_air_speed(float mag, float run) { if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; return run * mag * (1.0f / PM_MOON_STICK_MAX); }
