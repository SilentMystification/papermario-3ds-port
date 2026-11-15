/* MOON4: the game's midair window (src/7BB60.c:1095-1097) has priority. The moon jump acts only when no window can open in this air time. */
#pragma once
#define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
#define PM_MOON_AIR_TIME 18 /* the moon hold keeps timeInAir here */
static inline int pm_moon_allowed(int bootsLevel, int timeInAir) { return bootsLevel < 1 || timeInAir >= PM_MOON_WIN_END; }
#define PM_MOON_STICK_MAX 70.0f /* player_input_to_move_vector clamps the stick magnitude here (src/7B440.c:147) */
/* MOON5: the air speed of a moon jump: the run speed times the stick share */
static inline float pm_moon_air_speed(float mag, float run) { if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; return run * mag * (1.0f / PM_MOON_STICK_MAX); }
