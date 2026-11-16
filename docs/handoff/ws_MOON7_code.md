# ws_MOON7 code blocks (exact text, LF line ends). Designer to ask: the oracle.
## Block A: 3ds/src/pm_moon.h (replace the whole file)
```c
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
```
## Block B: 3ds/src/pm_dbg_game.c
B1, line 303. Before: `extern f32 GravityParamsStartJump[]; /* src/world/actions.c */`
After: `extern f32 GravityParamsStartJump[], GravityParamsStartFall[]; /* src/world/actions.c */`
B2: replace lines 320 to 368 (from the line that starts `    /* MOON3 (user): with the moon jump on` to the line `    }` just
before the `}` that ends pm_cheat_tick) with this text:
```c
    /* MOON7 (user 2026-11-16): the moon jump state machine is pm_moon_tick (pm_moon.h). A hold from the take-off lifts Mario with no stop at
     * the apex. A release while the cheat lifts is the fall of a normal apex (FALLING, peakJumpTime set), so a new press gets the spin or
     * tornado jump (src/7BB60.c:1092-1110). A later press or hold waits until the window has ended (timeInAir >= 18). The stick steers only
     * while A is held after the window. This code applies the action change and the stick. Cheat off: no change. */
    {
        static PmMoon mst;
        const int held = (ps->curButtons & BUTTON_A) != 0;
        const int air = pm_cheat_moon_get() && ((ps->flags & (PS_FLAG_JUMPING | PS_FLAG_FALLING)) != 0) && !(ps->flags & PS_FLAG_FLYING);
        int tia = ps->timeInAir, peak = ps->peakJumpTime, steer;
        const int r = pm_moon_tick(&mst, air, held, ps->actionState == ACTION_STATE_JUMP, p->bootsLevel, &tia, &peak, ps->gravityIntegrator,
                                   GravityParamsStartFall, MOON_RISE, &steer);
        ps->timeInAir = (s16)tia;
        ps->peakJumpTime = (s16)peak;
        if (r == PM_MOON_REENTER) { /* MOON4: back to JUMP as a tap does (set_action_state would reset the integrator to the full start jump) */
            ps->prevActionState = ps->actionState;
            ps->actionState = ACTION_STATE_JUMP;
            ps->flags &= ~PS_FLAG_FALLING;
            ps->flags |= PS_FLAG_JUMPING;
            pm_log("cheat: moon jump re-entered JUMP, y %.1f", ps->pos.y);
        } else if (r == PM_MOON_RELEASE) { /* MOON7: the fall of a normal apex instead of the game's HOP */
            set_action_state(ACTION_STATE_FALLING);
            pm_log("cheat: moon jump release y %.1f", ps->pos.y);
        }
        if (air && held && mst.up) pm_log_d("cheat: moon jump y %.1f x %.1f", ps->pos.y, ps->pos.x);
        if (steer) { /* MOON5 (user 3a): the stick steers a moon jump */
            f32 yaw, mag;
            player_input_to_move_vector(&yaw, &mag);
            if (mag > 0.0f) { ps->targetYaw = yaw; ps->curSpeed = pm_moon_air_speed(mag, ps->runSpeed); }
        }
    }
```
## Block C: 3ds/tools/pm_moon_test.c (replace the whole file)
```c
#include <stdio.h>
#include "pm_moon.h"
/* MOON7 host test. A model of one air time, step by step in the game's order (phys_update_standard, src/77480.c:714-733): the apex check
 * (src/7E9D0.c:243-248), the action update (a HOP runs initialize_jump: timeInAir and peakJumpTime 0, src/world/action/jump.c:18-19,
 * src/world/actions.c:151; timeInAir++: jump.c:74 and 134), phys_update_jump (src/7BB60.c:202-270), the fall (7BB60.c:328-336),
 * check_input_midair_jump (7BB60.c:1092-1110). Then the cheat tick (pm_moon_tick): pm_cheat_tick runs after the game step.
 * Integrator values: src/world/actions.c:45-46. */
enum { A_JUMP, A_HOP, A_FALL, A_SPIN, A_GROUND };
static const float J[4] = { 15.7566404343f, -7.38624f, 3.44693994522f, -0.75f };
static const float F[4] = { 0.154342994094f, -0.350080013275f, -0.182262003422f, 0.0115200001746f };
#define RISE (15.7566404343f * 0.5f)
typedef struct { float g[4], y, last; int tia, peak, act, changed, steer; PmMoon m; } S;
static void integ(float* g) { g[2] += g[3]; g[1] += g[2]; g[0] += g[1]; }
static void setg(float* g, const float* v) { g[0] = v[0]; g[1] = v[1]; g[2] = v[2]; g[3] = v[3]; }
static int step(S* s, int held, int pressed, int boots, int cheat) {
    s->steer = 0;
    if (s->tia != 0) { if (s->g[0] <= 0.0f && s->last > 0.0f) s->peak = s->tia; s->last = s->g[0]; }
    if (s->changed) { s->changed = 0; if (s->act == A_HOP) { s->tia = 0; s->peak = 0; } }
    s->tia++;
    if (s->act == A_JUMP) {
        if (!held) { s->act = A_HOP; s->changed = 1; integ(s->g); }
        integ(s->g);
        if (s->g[0] <= 0.0f) { setg(s->g, F); integ(s->g); s->act = A_FALL; s->changed = 1; }
        s->y += s->g[0];
    } else if (s->act == A_HOP) {
        s->g[0] -= 4.5f;
        s->y += s->g[0];
        if (s->g[0] <= 0.0f) { setg(s->g, F); integ(s->g); s->act = A_FALL; s->changed = 1; }
    } else {
        integ(s->g);
        s->y += s->g[0];
    }
    if (s->y <= 0.0f) return s->act = A_GROUND;
    if (s->peak >= 6 && s->tia < 18 && pressed && boots >= 1) return s->act = A_SPIN;
    if (cheat) {
        int r = pm_moon_tick(&s->m, 1, held, s->act == A_JUMP, boots, &s->tia, &s->peak, s->g, F, RISE, &s->steer);
        if (r == PM_MOON_REENTER) s->act = A_JUMP;
        if (r == PM_MOON_RELEASE) { s->act = A_FALL; s->changed = 1; }
    }
    return s->act;
}
/* One jump from the ground. A is held at step n when n <= h1 or (h2 > 0 and n >= h2); the take-off press is step 0.
 * Returns the landing step, minus the step of a spin jump, or 61 when Mario is still in the air after 60 steps. ys[n]: y after step n. */
static int run(int boots, int cheat, int h1, int h2, float* ys, int* steer) {
    S s = { { J[0], J[1], J[2], J[3] }, 0.0f, 0.0f, 0, 0, A_JUMP, 0, 0, { 0, 0, 0, 0 } };
    int n, prev = 1;
    ys[0] = 0.0f;
    for (n = 1; n <= 60; n++) {
        const int held = n <= h1 || (h2 > 0 && n >= h2);
        const int act = step(&s, held, held && !prev, boots, cheat);
        prev = held;
        ys[n] = s.y;
        if (steer) steer[n] = s.steer;
        if (act == A_SPIN) return -n;
        if (act == A_GROUND) return n;
    }
    return 61;
}
int main(void) {
    static const int c[][3] = { {1,17,0}, {1,18,1}, {2,5,0}, {2,40,1}, {0,1,1}, {-1,1,1} }; /* bootsLevel, timeInAir, pm_moon_late */
    static const float sp[3][3] = { {70.f, 4.f, 4.f}, {35.f, 4.f, 2.f}, {140.f, 4.f, 4.f} }; /* stick mag, run speed, air speed */
    float a[61], b[61];
    int st[61], i, k, n;
    for (i = 0; i < 6; i++)
        if (pm_moon_late(c[i][0], c[i][1]) != c[i][2]) { printf("moon FAIL case %d\n", i); return 1; }
    for (k = 1; k <= 4; k++) { /* a tap of 1 to 4 steps is the game's own jump: the lift comes at step 5 */
        n = run(1, 0, k, 0, a, 0);
        if (run(1, 1, k, 0, b, 0) != n) { printf("moon FAIL tap %d\n", k); return 1; }
        for (i = 1; i <= n; i++) if (a[i] != b[i]) { printf("moon FAIL tap %d\n", k); return 1; }
    }
    /* hold through the apex (8 steps), release 2 steps, press at step 11: the spin jump, cheat off and on */
    if (run(1, 0, 8, 11, a, 0) != -11) { printf("moon FAIL spin off\n"); return 1; }
    if (run(1, 1, 8, 11, b, 0) != -11) { printf("moon FAIL spin on\n"); return 1; }
    /* hold 60 steps: y rises every step (no stop at the apex); the stick steers from timeInAir 18 only */
    n = run(1, 1, 60, 0, b, st);
    if (n != 61) { printf("moon FAIL hold end %d\n", n); return 1; }
    for (i = 1; i <= 60; i++) {
        if (st[i] != (i >= PM_MOON_WIN_END)) { printf("moon FAIL steer step %d\n", i); return 1; }
        if (b[i] <= b[i - 1]) { printf("moon FAIL rise step %d\n", i); return 1; }
    }
    /* super boots: tap 2 steps, press at step 4 and hold: the cheat waits for the window, so this is the game's own hop and fall */
    n = run(1, 0, 2, 4, a, 0);
    if (n < 1 || run(1, 1, 2, 4, b, 0) != n) { printf("moon FAIL early\n"); return 1; }
    for (i = 1; i <= n; i++) if (a[i] != b[i]) { printf("moon FAIL early\n"); return 1; }
    /* normal boots: tap 2 steps, press at step 4 and hold: no window, the cheat lifts at once */
    n = run(0, 1, 2, 4, b, 0);
    if (n != 61) { printf("moon FAIL reenter %d\n", n); return 1; }
    for (i = 5; i <= 60; i++) if (b[i] <= b[i - 1]) { printf("moon FAIL reenter step %d\n", i); return 1; }
    for (i = 0; i < 3; i++) {
        float d = pm_moon_air_speed(sp[i][0], sp[i][1]) - sp[i][2];
        if (d > 1e-4f || d < -1e-4f) { printf("moon FAIL speed %d\n", i); return 1; }
    }
    printf("moon ok 10/10\n");
    return 0;
}
```
## Block D: 3ds/tests/host.d/moon.sh (replace the whole file)
```sh
#!/bin/sh
# MOON7: host test of pm_moon.h (pm_moon_late table, jump model: taps, spin after a hold, hold lift, steer gate, re-entry, stick speed).
# Each seeded fault must print its exact line. SSE math: the cheat-off and cheat-on runs compare floats bit for bit.
mkdir -p /tmp/moon_seed
CF="-m32 -msse2 -mfpmath=sse -Wall"
seed() {
    sed "$1" 3ds/src/pm_moon.h > /tmp/moon_seed/pm_moon.h
    gcc $CF -I /tmp/moon_seed -o /tmp/moon_seed/t 3ds/tools/pm_moon_test.c || exit 1
    got=$(/tmp/moon_seed/t)
    if [ "$got" != "$2" ]; then echo "moon seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/|| timeInAir >= PM_MOON_WIN_END/|| 1/' 'moon FAIL case 0'
seed 's/>= PM_MOON_FLOOR) return r;/>= 1e9f) return r;/' 'moon FAIL tap 1'
seed 's/if (m->up) {/if (0) {/' 'moon FAIL spin on'
seed 's/\*steer = \*timeInAir >= PM_MOON_WIN_END;/*steer = 1;/' 'moon FAIL steer step 5'
seed 's/if (mag > PM_MOON_STICK_MAX) mag = PM_MOON_STICK_MAX; //' 'moon FAIL speed 2'
gcc $CF -I 3ds/src -o /tmp/moon_t 3ds/tools/pm_moon_test.c || exit 1
/tmp/moon_t || exit 1
```
Seed facts (oracle arithmetic over actions.c:45-46): the held arc speeds are 11.07, 8.32, 6.78, 5.68, 4.28, then 1.82 (below 2.0),
so the lift comes in the tick of step 5. Seed 3: no release fall, the game's HOP resets peakJumpTime, no spin at step 11.
