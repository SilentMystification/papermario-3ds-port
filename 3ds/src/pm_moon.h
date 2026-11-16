/* MOON7 (user 2026-11-16): the moon jump keeps the game's jump, spin jump and tornado jump. One air time, from the take-off:
 *  HOLD: A held since a JUMP take-off. The game arc runs unchanged until its next speed is below PM_MOON_FLOOR (step 5 of a held jump).
 *        Then the cheat lifts: the speed is PM_MOON_FLOOR and grows by PM_MOON_RAMP a step up to rise, then stays. No apex, no stop.
 *  RELEASE: A up while the cheat lifts: the fall of a normal apex (fall integrator, the caller sets FALLING), not the game's HOP: HOP runs
 *        initialize_jump (src/world/actions.c:151, src/world/action/jump.c:18-19), which sets timeInAir and peakJumpTime to 0 and closes the
 *        window (src/7BB60.c:1095 needs peakJumpTime >= 6, timeInAir < 18). peakJumpTime is set here, so a press after it gets the spin.
 *  LATE: a press or hold after a release waits until the window has ended (timeInAir >= 18). Normal boots (bootsLevel < 1) have no window.
 *        Then the action goes back to JUMP at the rise speed (MOON4, the caller).
 *  STEER: the stick steers only while A is held, the cheat lifts and timeInAir >= 18.
 * timeInAir is capped at 18 while the cheat lifts (src/77480.c:614 respawns above 100). No division, no double. */
#pragma once
#define PM_MOON_WIN_END 18  /* the window is timeInAir < 18 */
#define PM_MOON_AIR_TIME 18 /* the cheat caps timeInAir here */
#define PM_MOON_FLOOR 2.0f  /* the lowest speed of a hold lift */
#define PM_MOON_RAMP 1.0f   /* the speed gain a step from the floor to the rise speed */
#define PM_MOON_STICK_MAX 70.0f /* player_input_to_move_vector clamps the stick magnitude here (src/7B440.c:147) */
enum { PM_MOON_NONE, PM_MOON_REENTER, PM_MOON_RELEASE };
typedef struct { int was_air, start, lifted, up; } PmMoon; /* start: A held since the take-off; lifted: the hold left the game arc; up: the cheat lifts */
static inline int pm_moon_late(int bootsLevel, int timeInAir) { return bootsLevel < 1 || timeInAir >= PM_MOON_WIN_END; }
/* One cheat tick after the game step. air: JUMPING or FALLING, not FLYING, cheat on. held: A down. jump: the action is JUMP. g: the integrator
 * (changed in place). fall: GravityParamsStartFall. rise: the rise speed. Returns PM_MOON_REENTER (caller: action JUMP) or PM_MOON_RELEASE
 * (caller: set_action_state FALLING). *steer: 1 when the stick steers this step. */
static inline int pm_moon_tick(PmMoon* m, int air, int held, int jump, int bootsLevel, int* timeInAir, int* peakJumpTime,
                               float* g, const float* fall, float rise, int* steer) {
    int r = PM_MOON_NONE;
    *steer = 0;
    if (!air) { m->was_air = m->start = m->lifted = m->up = 0; return r; }
    if (!m->was_air) { m->start = held && jump; m->lifted = m->up = 0; }
    m->was_air = 1;
    if (!held) {
        if (m->up) {
            g[0] = fall[0]; g[1] = fall[1]; g[2] = fall[2]; g[3] = fall[3];
            if (*peakJumpTime == 0) *peakJumpTime = *timeInAir;
            r = PM_MOON_RELEASE;
        }
        m->start = m->lifted = m->up = 0;
        return r;
    }
    if (m->start && jump) {
        if (!m->lifted) {
            if (g[0] + g[1] + g[2] + g[3] >= PM_MOON_FLOOR) return r;
            g[0] = PM_MOON_FLOOR - PM_MOON_RAMP; g[1] = PM_MOON_RAMP; g[2] = g[3] = 0.0f;
            m->lifted = m->up = 1;
        } else if (g[0] + g[1] >= rise) { g[0] = rise; g[1] = g[2] = g[3] = 0.0f; }
    } else if (pm_moon_late(bootsLevel, *timeInAir)) {
        if (!jump) r = PM_MOON_REENTER;
        g[0] = rise; g[1] = g[2] = g[3] = 0.0f;
        m->up = 1;
    } else return r;
    if (*timeInAir > PM_MOON_AIR_TIME) *timeInAir = PM_MOON_AIR_TIME;
    *steer = *timeInAir >= PM_MOON_WIN_END;
    return r;
}
/* MOON5: the air speed of a moon jump: the run speed times the stick share */
static inline float pm_moon_air_speed(float mag, float run) { if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; return run * mag * (1.0f / PM_MOON_STICK_MAX); }
