/* MOON4: the game's midair window (src/7BB60.c:1095-1097) has priority. The moon jump acts only when no window can open in this air time. */
#pragma once
#define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
#define PM_MOON_AIR_TIME 18 /* the moon hold keeps timeInAir here */
static inline int pm_moon_allowed(int bootsLevel, int timeInAir) { return bootsLevel < 1 || timeInAir >= PM_MOON_WIN_END; }
