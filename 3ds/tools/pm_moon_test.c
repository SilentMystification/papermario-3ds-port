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
