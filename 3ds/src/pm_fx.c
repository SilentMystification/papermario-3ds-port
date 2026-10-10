#include "common.h"
#include "effects.h"
#include <string.h>

/* The intro's effect calls. Retail display lists are still in the ROM overlay,
 * so these are the same instances the script pokes (position, dismiss) drawn
 * as camera-facing quads through the F3DEX path. */

#define PM_FX_SLOTS 48
#define PM_FX_VERTS 512

typedef struct PmFx {
    s32 live;
    EffectInstance inst;
    s32 kind;
    f32 x, y, z, x2, y2, z2, scale;
    s32 life, maxLife;
    u8 r, g, b;
    union {
        MiscParticlesFXData misc;
        LightRaysFXData rays;
        AuraFXData aura;
        EndingDecalsFXData decal;
        BulbGlowFXData bulb;
        SomethingRotatingFXData spin[16];
    } u;
} PmFx;

static PmFx slots[PM_FX_SLOTS];
static Vtx vert_pool[PM_FX_VERTS];
static int vert_used;
static Mtx ident_mtx;
static int ident_ready;
static u8 seen_kind[32];

extern void pm_log(const char* fmt, ...);

static void bind_data(PmFx* s) {
    s->inst.flags = FX_INSTANCE_FLAG_ENABLED;
    s->inst.effectID = s->kind;
    s->inst.numParts = 1;
    s->inst.shared = NULL;
    s->inst.data.any = NULL;
    switch (s->kind) {
        case EFFECT_MISC_PARTICLES:
            s->u.misc.variation = 0;
            s->u.misc.pos.x = s->x;
            s->u.misc.pos.y = s->y;
            s->u.misc.pos.z = s->z;
            s->inst.data.miscParticles = &s->u.misc;
            break;
        case EFFECT_LIGHT_RAYS:
            s->u.rays.type = 0;
            s->u.rays.pos.x = s->x;
            s->u.rays.pos.y = s->y;
            s->u.rays.pos.z = s->z;
            s->inst.data.lightRays = &s->u.rays;
            break;
        case EFFECT_AURA:
            s->u.aura.type = 0;
            s->u.aura.posA.x = s->x;
            s->u.aura.posA.y = s->y;
            s->u.aura.posA.z = s->z;
            s->inst.data.aura = &s->u.aura;
            break;
        case EFFECT_ENDING_DECALS:
            s->u.decal.pos.x = s->x;
            s->u.decal.pos.y = s->y;
            s->u.decal.pos.z = s->z;
            s->u.decal.scale = s->scale;
            s->inst.data.endingDecals = &s->u.decal;
            break;
        case EFFECT_BULB_GLOW:
            s->u.bulb.pos.x = s->x;
            s->u.bulb.pos.y = s->y;
            s->u.bulb.pos.z = s->z;
            s->inst.data.bulbGlow = &s->u.bulb;
            break;
        case EFFECT_SOMETHING_ROTATING:
            s->inst.numParts = 16;
            s->u.spin[0].pos.x = s->x;
            s->u.spin[0].pos.y = s->y;
            s->u.spin[0].pos.z = s->z;
            s->inst.data.somethingRotating = s->u.spin;
            break;
        default:
            break;
    }
}

static void pull_pos(PmFx* s) {
    switch (s->kind) {
        case EFFECT_MISC_PARTICLES:
            s->x = s->u.misc.pos.x;
            s->y = s->u.misc.pos.y;
            s->z = s->u.misc.pos.z;
            break;
        case EFFECT_LIGHT_RAYS:
            s->x = s->u.rays.pos.x;
            s->y = s->u.rays.pos.y;
            s->z = s->u.rays.pos.z;
            break;
        case EFFECT_AURA:
            s->x = s->u.aura.posA.x;
            s->y = s->u.aura.posA.y;
            s->z = s->u.aura.posA.z;
            break;
        case EFFECT_ENDING_DECALS:
            s->x = s->u.decal.pos.x;
            s->y = s->u.decal.pos.y;
            s->z = s->u.decal.pos.z;
            break;
        case EFFECT_BULB_GLOW:
            s->x = s->u.bulb.pos.x;
            s->y = s->u.bulb.pos.y;
            s->z = s->u.bulb.pos.z;
            break;
        case EFFECT_SOMETHING_ROTATING:
            s->x = s->u.spin[0].pos.x;
            s->y = s->u.spin[0].pos.y;
            s->z = s->u.spin[0].pos.z;
            break;
        default:
            break;
    }
}

static EffectInstance* spawn_to(s32 kind, f32 x, f32 y, f32 z, f32 x2, f32 y2, f32 z2, f32 scale, s32 life, u8 r, u8 g, u8 b);

static EffectInstance* spawn(s32 kind, f32 x, f32 y, f32 z, f32 scale, s32 life, u8 r, u8 g, u8 b) {
    return spawn_to(kind, x, y, z, x, y, z, scale, life, r, g, b);
}

static EffectInstance* spawn_to(s32 kind, f32 x, f32 y, f32 z, f32 x2, f32 y2, f32 z2, f32 scale, s32 life, u8 r, u8 g, u8 b) {
    PmFx* s = NULL;
    s32 i;
    /* life < 0 stays until the script dismisses it. */
    if (scale < 0.05f) scale = 0.05f;
    for (i = 0; i < PM_FX_SLOTS; i++) {
        if (!slots[i].live) {
            s = &slots[i];
            break;
        }
    }
    if (!s) s = &slots[0];
    memset(s, 0, sizeof(*s));
    s->live = 1;
    s->kind = kind;
    s->x = x;
    s->y = y;
    s->z = z;
    s->x2 = x2;
    s->y2 = y2;
    s->z2 = z2;
    s->scale = scale;
    s->life = s->maxLife = life;
    s->r = r;
    s->g = g;
    s->b = b;
    bind_data(s);
    if (kind >= 0 && kind < (s32)sizeof(seen_kind) && !seen_kind[kind]) {
        seen_kind[kind] = 1;
        pm_log("fx %d", kind);
    }
    return &s->inst;
}

static PmFx* from_inst(EffectInstance* effect) {
    s32 i;
    if (!effect) return NULL;
    for (i = 0; i < PM_FX_SLOTS; i++) {
        if (&slots[i].inst == effect) return &slots[i];
    }
    return NULL;
}

void clear_effect_data(void) {
    memset(slots, 0, sizeof(slots));
}

void remove_effect(EffectInstance* effect) {
    PmFx* s = from_inst(effect);
    if (s) s->live = 0;
}

void update_effects(void) {
    s32 i;
    for (i = 0; i < PM_FX_SLOTS; i++) {
        PmFx* s = &slots[i];
        if (!s->live) continue;
        pull_pos(s);
        if (s->inst.flags & FX_INSTANCE_FLAG_DISMISS) {
            if (s->life < 0 || s->life > 8) s->life = 8;
        }
        if (s->life > 0 && --s->life <= 0) s->live = 0;
    }
}

static float pm_sqrt(float x) {
    float g;
    int i;
    if (x <= 0.f) return 0.f;
    g = x > 1.f ? x : 1.f;
    for (i = 0; i < 6; i++) g = 0.5f * (g + x / g);
    return g;
}

static float pm_sin(float rad) { return sin_deg(rad * 57.29578f); }
static float pm_cos(float rad) { return cos_deg(rad * 57.29578f); }

static s16 clamp_s(float v) {
    if (v > 32000.f) return 32000;
    if (v < -32000.f) return -32000;
    return (s16)v;
}

static void quad(float x, float y, float z, float hx, float hy, u8 r, u8 g, u8 b, u8 a) {
    Camera* cam = &gCameras[gCurrentCameraID];
    float fx, fy, fz, fl, rx, ry, rz, rl, ux, uy, uz;
    Vtx* v;
    int i;
    static const float sx[4] = { -1.f, 1.f, 1.f, -1.f };
    static const float sy[4] = { -1.f, -1.f, 1.f, 1.f };
    if (vert_used + 4 > PM_FX_VERTS || !gMainGfxPos) return;
    fx = cam->lookAt_obj.x - cam->lookAt_eye.x;
    fy = cam->lookAt_obj.y - cam->lookAt_eye.y;
    fz = cam->lookAt_obj.z - cam->lookAt_eye.z;
    fl = pm_sqrt(fx * fx + fy * fy + fz * fz);
    if (fl < 0.001f) fl = 1.f;
    fx /= fl;
    fy /= fl;
    fz /= fl;
    rx = -fz;
    ry = 0.f;
    rz = fx;
    rl = pm_sqrt(rx * rx + rz * rz);
    if (rl < 0.001f) {
        rx = 1.f;
        rz = 0.f;
        rl = 1.f;
    }
    rx /= rl;
    rz /= rl;
    ux = ry * fz - rz * fy;
    uy = rz * fx - rx * fz;
    uz = rx * fy - ry * fx;
    v = &vert_pool[vert_used];
    vert_used += 4;
    memset(v, 0, sizeof(Vtx) * 4);
    for (i = 0; i < 4; i++) {
        v[i].v.ob[0] = clamp_s(x + (rx * sx[i] * hx) + (ux * sy[i] * hy));
        v[i].v.ob[1] = clamp_s(y + (ry * sx[i] * hx) + (uy * sy[i] * hy));
        v[i].v.ob[2] = clamp_s(z + (rz * sx[i] * hx) + (uz * sy[i] * hy));
        v[i].v.cn[0] = r;
        v[i].v.cn[1] = g;
        v[i].v.cn[2] = b;
        v[i].v.cn[3] = a;
    }
    if (!ident_ready) {
        guMtxIdent(&ident_mtx);
        ident_ready = 1;
    }
    gSPMatrix(gMainGfxPos++, &ident_mtx, G_MTX_PUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPClearGeometryMode(gMainGfxPos++, G_LIGHTING | G_CULL_BOTH | G_TEXTURE_GEN);
    gSPSetGeometryMode(gMainGfxPos++, G_SHADE | G_ZBUFFER | G_SHADING_SMOOTH);
    gDPSetCombineMode(gMainGfxPos++, G_CC_SHADE, G_CC_SHADE);
    gSPVertex(gMainGfxPos++, v, 4, 0);
    gSP1Triangle(gMainGfxPos++, 0, 1, 2, 0);
    gSP1Triangle(gMainGfxPos++, 0, 2, 3, 0);
    gSPPopMatrix(gMainGfxPos++, G_MTX_MODELVIEW);
}

static void draw_one(PmFx* s) {
    f32 age = (s->maxLife > 0) ? (1.f - (f32)s->life / (f32)s->maxLife) : 0.35f;
    f32 grow = s->scale * (8.f + 28.f * age);
    u8 a = (s->maxLife > 0) ? (u8)(40 + (215 * s->life) / s->maxLife) : 200;
    s32 i;
    switch (s->kind) {
        case EFFECT_LIGHT_RAYS:
            for (i = 0; i < 4; i++) {
                f32 ang = age * 6.28f + (f32)i * 1.57f;
                quad(s->x + pm_sin(ang) * 18.f, s->y, s->z + pm_cos(ang) * 18.f, 3.f, 40.f * s->scale, s->r, s->g, s->b, a);
            }
            break;
        case EFFECT_ENDING_DECALS:
        case EFFECT_SOMETHING_ROTATING:
            quad(s->x, s->y, s->z, 14.f * s->scale, 20.f * s->scale, s->r, s->g, s->b, a);
            for (i = 0; i < 6; i++) {
                f32 ang = age * 4.f + (f32)i;
                quad(s->x + pm_sin(ang) * 36.f, s->y + (f32)i * 2.f, s->z + pm_cos(ang) * 36.f,
                    8.f, 12.f, 255, 230, 140, a);
            }
            break;
        case EFFECT_MISC_PARTICLES:
        case EFFECT_SPARKLES:
            for (i = 0; i < 5; i++) {
                f32 ang = age * 9.f + (f32)i * 1.2f;
                quad(s->x + pm_sin(ang) * (6.f + s->scale), s->y + (f32)((i * 3) % 7), s->z + pm_cos(ang) * (6.f + s->scale),
                    2.f, 2.f, s->r, s->g, s->b, a);
            }
            break;
        case EFFECT_LIGHTNING:
            for (i = 0; i < 7; i++) {
                f32 jag = ((i & 1) ? 14.f : -14.f);
                quad(s->x + jag, s->y + 90.f - (f32)i * 22.f, s->z, 3.f, 16.f, 255, 255, 220, a);
            }
            quad(s->x, s->y + 20.f, s->z, 10.f, 6.f, 255, 255, 255, a);
            break;
        case EFFECT_FIRE_BREATH:
            for (i = 0; i < 6; i++) {
                f32 t = ((f32)i + 1.f) / 6.f;
                f32 heat = 1.f - t * 0.65f;
                quad(s->x + (s->x2 - s->x) * t, s->y + (s->y2 - s->y) * t, s->z + (s->z2 - s->z) * t,
                    4.f + 10.f * t, 4.f + 8.f * t,
                    255, (u8)(180.f * heat), (u8)(40.f * heat), a);
            }
            break;
        default:
            quad(s->x, s->y, s->z, grow, grow, s->r, s->g, s->b, a);
            break;
    }
}

void render_effects_scene(void) {
    s32 i;
    vert_used = 0;
    for (i = 0; i < PM_FX_SLOTS; i++) {
        if (slots[i].live) draw_one(&slots[i]);
    }
}

void render_effects_UI(void) {}

static s32 arg_i(Evt* script, s32 n) {
    return evt_get_variable(script, script->ptrReadPos[n]);
}

static f32 arg_f(Evt* script, s32 n) {
    return evt_get_float_variable(script, script->ptrReadPos[n]);
}

API_CALLABLE(PlayEffect_impl) {
    s32 id = arg_i(script, 0);
    s32 sub = arg_i(script, 1);
    f32 x = arg_f(script, 2);
    f32 y = arg_f(script, 3);
    f32 z = arg_f(script, 4);
    f32 sc = arg_f(script, 5);
    s32 life = arg_i(script, 6);
    EffectInstance* made = NULL;
    (void)sub;
    switch (id) {
        case EFFECT_RING_BLAST:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 4.f, life > 0 ? life : 20, 255, 220, 80);
            break;
        case EFFECT_MISC_PARTICLES:
            made = spawn(id, x, y, z, arg_f(script, 5), -1, 255, 255, 220);
            evt_set_variable(script, LVarF, (s32)made);
            break;
        case EFFECT_LIGHTNING:
            made = spawn(id, x, y, z, 1.f, -1, 255, 255, 255);
            evt_set_variable(script, LVarF, (s32)made);
            break;
        case EFFECT_ENDING_DECALS:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, -1, 255, 210, 80);
            evt_set_variable(script, script->ptrReadPos[6], (s32)made);
            break;
        case EFFECT_LIGHT_RAYS:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, -1, 255, 240, 180);
            evt_set_variable(script, script->ptrReadPos[6], (s32)made);
            break;
        case EFFECT_FIRE_BREATH:
            made = spawn_to(id, x, y, z, arg_f(script, 5), arg_f(script, 6), arg_f(script, 7), 1.f, -1, 255, 90, 20);
            evt_set_variable(script, LVarF, (s32)made);
            break;
        case EFFECT_SHIMMER_BURST:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, life > 0 ? life : 30, 255, 255, 255);
            break;
        case EFFECT_BULB_GLOW:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, -1, 255, 240, 120);
            evt_set_variable(script, script->ptrReadPos[6], (s32)made);
            break;
        case EFFECT_ENERGY_SHOCKWAVE:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, life > 0 ? life : 60, 120, 200, 255);
            break;
        case EFFECT_AURA:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, -1, 255, 180, 255);
            evt_set_variable(script, script->ptrReadPos[6], (s32)made);
            break;
        case EFFECT_SOMETHING_ROTATING:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, -1, 255, 200, 60);
            evt_set_variable(script, LVarF, (s32)made);
            break;
        case EFFECT_SPARKLES:
            made = spawn(id, x, y, z, sc > 0.f ? sc * 0.2f : 4.f, 20, 255, 255, 255);
            break;
        case EFFECT_RADIAL_SHIMMER:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, life > 0 ? life : 40, 255, 255, 255);
            break;
        case EFFECT_ENERGY_ORB_WAVE:
            made = spawn(id, x, y, z, sc > 0.f ? sc : 1.f, life > 0 ? life : 30, 140, 180, 255);
            break;
        default:
            break;
    }
    (void)made;
    return ApiStatus_DONE2;
}

API_CALLABLE(DismissEffect) {
    Bytecode* args = script->ptrReadPos;
    EffectInstance* effect = (EffectInstance*)evt_get_variable(script, *args++);
    if (effect) effect->flags |= FX_INSTANCE_FLAG_DISMISS;
    return ApiStatus_DONE2;
}

API_CALLABLE(RemoveEffect) {
    Bytecode* args = script->ptrReadPos;
    remove_effect((EffectInstance*)evt_get_variable(script, *args++));
    return ApiStatus_DONE2;
}

void fx_aura(s32 type, f32 x, f32 y, f32 z, f32 scale, EffectInstance** out) {
    EffectInstance* e = spawn(EFFECT_AURA, x, y, z, scale, -1, 255, 180, 255);
    if (e) e->data.aura->type = type;
    if (out) *out = e;
}

void fx_stars_orbiting(s32 type, f32 x, f32 y, f32 z, f32 scale, s32 n, EffectInstance** out) {
    (void)type;
    (void)n;
    if (out) *out = spawn(EFFECT_SPARKLES, x, y, z, scale, 40, 255, 255, 200);
}

EffectInstance* fx_energy_orb_wave(s32 type, f32 x, f32 y, f32 z, f32 scale, s32 arg5) {
    (void)type;
    return spawn(EFFECT_ENERGY_ORB_WAVE, x, y, z, scale, arg5 > 0 ? arg5 : 30, 140, 180, 255);
}

EffectInstance* fx_misc_particles(s32 variation, f32 x, f32 y, f32 z, f32 sx, f32 sy, f32 unk, s32 count, s32 life) {
    EffectInstance* e;
    (void)variation;
    (void)sy;
    (void)unk;
    (void)count;
    e = spawn(EFFECT_MISC_PARTICLES, x, y, z, sx > 0.f ? sx * 0.05f : 1.f, life > 0 ? life : -1, 255, 255, 220);
    return e;
}

void fx_fire_breath(s32 type, f32 x, f32 y, f32 z, f32 x2, f32 y2, f32 z2, s32 a, s32 b, s32 life) {
    (void)type;
    (void)a;
    (void)b;
    spawn_to(EFFECT_FIRE_BREATH, x, y, z, x2, y2, z2, 1.f, life > 0 ? life : 20, 255, 90, 20);
}
