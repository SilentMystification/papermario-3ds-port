/* FX1a (class E, port-side debug tool): `fxtest=<list|all|n,n,...>` together with the `shots` switch.
 * Spawns each real fx_ call of src/effects one at a time, with the arguments the map scripts and battle code use
 * (values from PlayEffect lines in src/world and from call sites in src). Position = player position.
 * Per effect, in a 44 frame slot: clean shot, spawn, shot A, shot B, remove all effects.
 * Log lines "fxtest: ..." map shot numbers to effect names. 3ds/tools/fxtest_report.py reads them.
 * fxtest=list only prints the names. Nothing runs without the switch. No decomp function is replaced. */
#include "common.h"
#include "effects.h"
#include "npc.h"
#include "pm_port.h"
#include <stdio.h>
int atoi(const char*);
#include <string.h>

int pm_debug_value(const char*, char*, unsigned);
void pm_gpu_shot_burst(int n);
extern EffectInstance* gEffectInstances[96];
extern PlayerStatus gPlayerStatus;

#define X fx_x
#define Y fx_y
#define Z fx_z
static float fx_x, fx_y, fx_z;
static EffectInstance* oe;
static Npc* fx_npc;

/* FX3f: lil_oink draws only the pigs a script sets visible (mac_03 LoadLilOinks): show one. */
static void fxt_lil_oink(void);
/* FX3g: emote follows the player (PTR_LIST_END), the first NPC of the map was off screen. */
/* E(name, call). Names follow src/effects/<name>.c. */
#define FXLIST(E) \
E(big_smoke_puff, fx_big_smoke_puff(X, Y, Z)) \
E(landing_dust, fx_landing_dust(4, X, Y, Z, 0)) \
E(walking_dust, fx_walking_dust(2, X, Y, Z, 0, 0)) \
E(flower_splash, fx_flower_splash(X, Y, Z, 0)) \
E(flower_trail, fx_flower_trail(1, X, Y, Z, 0, 0)) \
E(cloud_puff, fx_cloud_puff(X, Y, Z, 0)) \
E(cloud_trail, fx_cloud_trail(0, X, Y, Z)) \
E(footprint, fx_footprint(X, Y - 25.0f, Z, 0, 0)) \
E(floating_flower, fx_floating_flower(0, X, Y, Z, 100)) \
E(snowflake, fx_snowflake(X, Y, Z, 60)) \
E(star, fx_star(FX_STAR_LARGE_BOUNCING, X, Y, Z, X + 20, 0, Z, 10)) \
E(emote, fx_emote(EMOTE_EXCLAMATION, (Npc*)PTR_LIST_END, 0, 40.0f, 1.0f, 2.0f, -20.0f, 15, &oe)) \
E(sparkles, fx_sparkles(0, X, Y, Z, 10)) \
E(shape_spell, fx_shape_spell(0, X, Y, Z, X + 60, Y + 40, Z, 30)) \
E(gather_energy_pink, fx_gather_energy_pink(0, X, Y, Z, 1.0f, 60)) \
E(drop_leaves, fx_drop_leaves(0, X, Y, Z, 100)) \
E(dust, fx_dust(0, X, Y, Z, 30)) \
E(shattering_stones, fx_shattering_stones(0, X, Y, Z, 187)) \
E(smoke_ring, fx_smoke_ring(0, X, Y, Z)) \
E(damage_stars, fx_damage_stars(FX_DAMAGE_STARS_2, X, Y, Z, 0, -1.0f, 0, 5)) \
E(explosion, fx_explosion(0, X, Y, Z)) \
E(lens_flare, fx_lens_flare(0, X, Y, Z, 20)) \
E(got_item_outline, fx_got_item_outline(0, X, Y, Z, 1.0f, &oe)) \
E(spiky_white_aura, fx_spiky_white_aura(0, X, Y, Z, 60)) \
E(smoke_impact, fx_smoke_impact(0, X, Y, Z, 20, 10, 0, 60)) \
E(damage_indicator, fx_damage_indicator(0, X, Y, Z, 10.0f, 0, 7, &oe)) \
E(purple_ring, fx_purple_ring(0, X, Y, Z, X + 40, Y, Z, 1.0f)) \
E(flame, fx_flame(FX_FLAME_RED, X, Y, Z, 0.3f, &oe)) \
E(stars_burst, fx_stars_burst(0, X, Y, Z, 0, 54.0f, 2)) \
E(stars_shimmer, fx_stars_shimmer(5, X, Y, Z, 50.0f, 50.0f, 40, 30)) \
E(rising_bubble, fx_rising_bubble(0, X, Y, Z, 0)) \
E(ring_blast, fx_ring_blast(0, X, Y, Z, 1.0f, 24)) \
E(shockwave, fx_shockwave(0, X, Y, Z)) \
E(music_note, fx_music_note(1, X, Y, Z)) \
E(smoke_burst, fx_smoke_burst(0, X, Y, Z, 3.0f, 40)) \
E(sweat, fx_sweat(0, X, Y, Z, 5.0f, 45.0f, 20)) \
E(sleep_bubble, fx_sleep_bubble(0, X, Y, Z, 25.0f, 0, &oe)) \
E(windy_leaves, fx_windy_leaves(2, X, Y, Z)) \
E(falling_leaves, fx_falling_leaves(0, X, Y, Z)) \
E(stars_spread, fx_stars_spread(0, X, Y, Z, 6, 20)) \
E(steam_burst, fx_steam_burst(0, X, Y, Z, 1.0f, 20)) \
E(stars_orbiting, fx_stars_orbiting(0, X, Y, Z, 20.0f, 3, &oe)) \
E(big_snowflakes, fx_big_snowflakes(0, X, Y, Z)) \
E(debuff, fx_debuff(0, X, Y, Z)) \
E(green_impact, fx_green_impact(false, X, Y, Z, 45.0f)) \
E(radial_shimmer, fx_radial_shimmer(2, X, Y, Z, 1.0f, 100)) \
E(ending_decals, fx_ending_decals(2, X, Y, Z, 1.0f, &oe)) \
E(light_rays, fx_light_rays(1, X, Y, Z, 1.0f, &oe)) \
E(lightning, fx_lightning(3, X, Y, Z, 1.0f, 1.0f)) \
E(fire_breath, fx_fire_breath(0, X, Y, Z, X + 10, Y, Z, 5, 2, 30)) \
E(shimmer_burst, fx_shimmer_burst(0, X, Y, Z, 1.0f, 30)) \
E(energy_shockwave, fx_energy_shockwave(0, X, Y, Z, 1.0f, 90)) \
E(shimmer_wave, fx_shimmer_wave(0, X, Y, Z, 300.0f, 100.0f, 10, 40)) \
E(aura, fx_aura(FX_AURA_GOLD, X, Y, Z, 1.0f, &oe)) \
E(bulb_glow, fx_bulb_glow(2, X, Y, Z, 1.0f, &oe)) \
E(effect_3D, fx_effect_3D(0, X, Y, Z, 0, 0, 1.0f, 60, &oe)) \
E(blast, fx_blast(0, X, Y, Z, 3.0f, 30)) \
E(fire_flower, fx_fire_flower(0, X, Y, Z, 0)) \
E(recover, fx_recover(2, X, Y, Z, 0)) \
E(disable_x, fx_disable_x(0, X, Y, Z, 0)) \
E(bombette_breaking, fx_bombette_breaking(1, 25, 3, 1.0f, 10, 30)) \
E(firework, fx_firework(0, X, Y, Z, 1.0f, 0)) \
E(confetti, fx_confetti(3, X, Y, Z, 1.0f, 120)) \
E(snowfall, fx_snowfall(0, 120)) \
E(effect_46, fx_effect_46(6, &gPlayerStatus, 1.0f, 60)) \
E(gather_magic, fx_gather_magic(0, X, Y, Z, 0.5f, 30)) \
E(attack_result_text, fx_attack_result_text(0, X, Y, Z, 12.0f, 90)) \
E(small_gold_sparkle, fx_small_gold_sparkle(0, X, Y, Z, 1.0f, 0)) \
E(flashing_box_shockwave, fx_flashing_box_shockwave(0, X, Y, Z, 0, 0)) \
E(balloon, fx_balloon(2, X, Y, Z, 1.0f, 150)) \
E(floating_rock, fx_floating_rock(2, X, Y + 100, Z, Y - 25, -0.5f, -0.9f, 0.5f, 60)) \
E(chomp_drop, fx_chomp_drop(0, X, Y, Z, 0.2f, 0, 1.4f, 255, 0.1f, 150)) \
E(quizmo_stage, fx_quizmo_stage(0, X, Y, Z)) \
E(radiating_energy_orb, fx_radiating_energy_orb(0, X, Y, Z, 1.0f, -1)) \
E(quizmo_answer, fx_quizmo_answer(0, 0, 0, 0)) \
E(motion_blur_flame, fx_motion_blur_flame(0, X, Y, Z, 1.0f, -1)) \
E(energy_orb_wave, fx_energy_orb_wave(5, X, Y, Z, 1.0f, 20)) \
E(merlin_house_stars, fx_merlin_house_stars(0, X, Y, Z)) \
E(quizmo_audience, fx_quizmo_audience(0, X, Y, Z)) \
E(butterflies, fx_butterflies(5, X, Y, Z)) \
E(stat_change, fx_stat_change(0, X, Y, Z, 1.5f, 60)) \
E(snaking_static, fx_snaking_static(0, X, Y, Z, 1.0f, 30)) \
E(thunderbolt_ring, fx_thunderbolt_ring(0, X, Y, Z, 1.0f, 60)) \
E(squirt, fx_squirt(0, X, Y, Z, X + 60, Y, Z, 1.0f, 30)) \
E(water_block, fx_water_block(FX_WATER_BLOCK_DESTROY, X, Y, Z, 1.5f, 10)) \
E(waterfall, fx_waterfall(0, X, Y, Z, 1.0f, 50)) \
E(water_fountain, fx_water_fountain(0, X, Y, Z, 1.0f, 40)) \
E(underwater, fx_underwater(0, X, Y, Z, 1.0f, 120)) \
E(lightning_bolt, fx_lightning_bolt(0, X, Y + 80, Z, X + 60, Y + 80, Z, 1.0f, 30)) \
E(water_splash, fx_water_splash(2, X, Y, Z, 0.7f, 30)) \
E(snowman_doll, fx_snowman_doll(0, X, Y, Z, 1.0f, 160)) \
E(fright_jar, fx_fright_jar(0, X, Y, Z, 1.5f, 60)) \
E(stop_watch, fx_stop_watch(0, X, Y, Z, 1.0f, 200)) \
E(effect_63, fx_effect_63(1, X, Y, Z, X + 60, Y, Z, 1.0f, 20, 20)) \
E(throw_spiny, fx_throw_spiny(0, X, Y, Z, X + 60, Y, Z, 1.5f, 30)) \
E(effect_65, fx_effect_65(2, X, Y, Z, 1.0f, 0)) \
E(tubba_heart_attack, fx_tubba_heart_attack(0, X, Y, Z, 1.0f, 200)) \
E(whirlwind, fx_whirlwind(0, X, Y, Z, 1.0f, 0)) \
E(red_impact, fx_red_impact(0, X, Y, Z, 1.0f, 0)) \
E(floating_cloud_puff, fx_floating_cloud_puff(0, X, Y, Z, 1.0f, 25)) \
E(energy_in_out, fx_energy_in_out(1, X, Y, Z, 1.0f, 30)) \
E(tattle_window, fx_tattle_window(0, 106.0f, 144.0f, 0, 1.0f, 0)) \
E(shiny_flare, fx_shiny_flare(0, X, Y, Z, 1.5f, 0)) \
E(huff_puff_breath, fx_huff_puff_breath(1, X, Y, Z, 190.0f, -1.0f, 1.5f, 100)) \
E(cold_breath, fx_cold_breath(0, X, Y, Z, 2.0f, 45)) \
E(embers, fx_embers(0, X, Y, Z, 70.0f, 80.0f, 2.5f, 80, 55, 2.0f, 2.0f)) \
E(hieroglyphs, fx_hieroglyphs(0, X, Y, Z, 1.0f, 45)) \
E(misc_particles, fx_misc_particles(1, X, Y, Z, 24.0f, 24.0f, 1.0f, 5, 0)) \
E(static_status, fx_static_status(0, X, Y, Z, 0.9f, 5, 0)) \
E(moving_cloud, fx_moving_cloud(0, X + 100, Y, Z, X - 100, Y, Z, 5.0f, Y)) \
E(effect_75, fx_effect_75(0, X, Y, Z, 1.0f, -1)) \
E(firework_rocket, fx_firework_rocket(0, X, Y, Z, -1.0f, 4.0f, -1.0f, 1.6f, 70)) \
E(peach_star_beam, fx_peach_star_beam(0, X, Y, Z, 1.0f, 0)) \
E(chapter_change, fx_chapter_change(0, 100.0f, 100.0f, 0.0f, 1.0f, 0)) \
E(ice_shard, fx_ice_shard(0, X, Y, Z, 1.0f, 30)) \
E(spirit_card, fx_spirit_card(1, X, Y, Z, 1.0f, 0)) \
E(lil_oink, fxt_lil_oink()) \
E(something_rotating, fx_something_rotating(2, X, Y, Z, 1.0f, 0)) \
E(breaking_junk, fx_breaking_junk(0, X, Y, Z, 1.0f, 30)) \
E(partner_buff, fx_partner_buff(0, X, Y, Z, 1.0f, 0)) \
E(quizmo_assistant, fx_quizmo_assistant(0, X, Y, Z, 1.0f, 0)) \
E(ice_pillar, fx_ice_pillar(0, X, Y, Z, 1.0f, 0)) \
E(sun, fx_sun(0, 0, 0, 0, 0, 0)) \
E(star_spirits_energy, fx_star_spirits_energy(2, X, Y, Z, 1.0f, 0)) \
E(pink_sparkles, fx_pink_sparkles(0, X, Y, Z, 1.0f, 0)) \
E(star_outline, fx_star_outline(0, X, Y, Z, 1.0f, 0)) \
E(effect_86, fx_effect_86(0, X, Y, Z, 3.0f, 0))

static void fxt_lil_oink(void) {
    EffectInstance* o = fx_lil_oink(0, 0.0f, 0.0f, 0.0f, 0.0f, 0);
    LilOinkFXData* d = o->data.lilOink;
    d->type[0] = 0;
    d->x[0] = X; d->y[0] = Y - 25.0f; d->z[0] = Z; d->rot[0] = 270.0f;
    d->flags[0] |= LIL_OINK_FLAG_VISIBLE;
}

#define E_ENUM(n, c) FXT_##n,
enum { FXLIST(E_ENUM) FXT_COUNT };
#define E_NAME(n, c) #n,
static const char* const fx_names[] = { FXLIST(E_NAME) };
#define E_CASE(n, c) case FXT_##n: c; break;
static void fx_spawn(int i) {
    oe = NULL;
    switch (i) { FXLIST(E_CASE) }
}

#define SLOT 44
#define START 400
static unsigned char sel[FXT_COUNT];
static int nsel = -1, shotn;

static void shot(const char* what, int i) {
    { int k, live = 0; for (k = 0; k < 96; k++) if (gEffectInstances[k]) live++; /* FX3f: live effect count at the shot */
      pm_log("fxtest: shot %d effect %d %s %s", shotn++, i, fx_names[i], what);
      pm_log("fxtest: live %d", live); }
    pm_gpu_shot_burst(1);
}

static void clear_all(void) {
    int k;
    for (k = 0; k < 96; k++) {
        if (gEffectInstances[k]) remove_effect(gEffectInstances[k]);
    }
}

extern EffectInstance* gEffectInstances[96];
void pm_fxtest_tick(unsigned f) {
    static char sw[300];
    int slot, k, i, n;
    { /* switch fxlist=<frame>: one log line per live effect instance (name, graphics address) at that frame */
        static int fl = -2;
        char v[16];
        if (fl == -2) fl = pm_debug_value("fxlist", v, sizeof v) ? atoi(v) : -1;
        extern unsigned pm_frame_count(void);
        static int fdone;
        if (fl >= 0 && !fdone && pm_frame_count() >= (unsigned)fl && ++fdone)
            for (i = 0; i < 96; i++) {
                EffectInstance* e = gEffectInstances[i];
                if (e && e->shared && e->shared->effectIndex >= 0 && e->shared->effectIndex < FXT_COUNT)
                    pm_log("fxlist f%u slot %d effect %d %s graphics %p", pm_frame_count(), i, e->shared->effectIndex, fx_names[e->shared->effectIndex], (void*)e->shared->graphics);
            }
    }
    if (nsel < 0) {
        char* p;
        nsel = 0;
        if (!pm_debug_value("fxtest", sw, sizeof sw)) { nsel = -2; return; }
        if (!strcmp(sw, "list") || !strcmp(sw, "all")) {
            for (i = 0; i < FXT_COUNT; i++) {
                if (!strcmp(sw, "list")) pm_log("fxtest: effect %d %s", i, fx_names[i]);
                else sel[nsel++] = i;
            }
        } else {
            for (p = strtok(sw, ","); p; p = strtok(NULL, ",")) {
                n = atoi(p);
                if (n >= 0 && n < FXT_COUNT) sel[nsel++] = n;
            }
        }
        pm_log("fxtest: %d effects selected of %d, slot %d frames, start frame %d", nsel, FXT_COUNT, SLOT, START);
    }
    if (nsel <= 0 || f < START) return;
    slot = (f - START) / SLOT;
    k = (f - START) % SLOT;
    if (slot >= nsel) {
        if (slot == nsel && k == 0) pm_log("fxtest: done");
        return;
    }
    i = sel[slot];
    if (k == 0) {
        fx_x = gPlayerStatus.pos.x;
        { char dx[16]; if (pm_debug_value("fxdx", dx, sizeof dx)) fx_x += (slot & 1 ? -1.f : 1.f) * (float)atoi(dx); } /* F07-7: move the spawn point to the screen edge, odd slots to the other side */
        fx_y = gPlayerStatus.pos.y + 25.0f;
        fx_z = gPlayerStatus.pos.z;
        shot("base", i);
    } else if (k == 2) {
        int id;
        fx_npc = NULL;
        for (id = 0; id < 16 && !fx_npc; id++) fx_npc = get_npc_safe(id);
        pm_log("fxtest: spawn %d %s at %d %d %d", i, fx_names[i], (int)fx_x, (int)fx_y, (int)fx_z);
        if (i == FXT_emote && !fx_npc) pm_log_w("fxtest: not spawned %s: no NPC in this map", fx_names[i]);
        fx_spawn(i);
    } else if (k == (i == FXT_shiny_flare || i == FXT_emote ? 7 : 12)) shot("a", i); /* FX3f: shiny_flare lives 10 frames */
    else if (k == 32) shot("b", i);
    else if (k == 40) clear_all();
}
