# ws_SPM0: star power census (Haiku, closed edits). Plan: docs/handoff/plan_upaway.md. Base main 67b6f47608. Counters C1..C12: none moves (census only).
Objective: switch `spmove=<i>[:<i>:<i>]` starts StarPowersTable[i] (src/battle/use_star_powers.c:29-42: 7 Up and Away, 8 Star Beam, 9 Peach Beam) at the next player menu of a debug battle. Line `spm` per 30 steps.
Edit order: 3ds/src/pm_gpu.c, 3ds/src/pm_main.c, 3ds/src/pm_fxtest.c, 3ds/src/pm_warp.c, 3ds/tools/azt.d/SPM0.json, 3ds/tools/bench.d/SPM0.json. Edit no other file.
E1 pm_gpu.c:2890 in pm_gpu_grab_prev. Before: `    prev_done = r == 2 ? 1 : -1;` After: `    prev_done = r == 2 ? 1 : -1; if (r == 2) pm_spm_t3++; /* SPM0: old-frame grabs */`. Put `unsigned pm_spm_t3; /* SPM0 */` above `int pm_gpu_grab_prev`.
E2 pm_main.c:380 in pm_cfb_capture_prev. Before: `            cap_note("cpu");` After: `            cap_note("cpu"); pm_spm_cap++; /* SPM0: CPU frame reads */`. Put `unsigned pm_spm_cap; /* SPM0 */` above `void pm_cfb_capture_prev(void) {`.
E3 pm_fxtest.c: put this block after the closing brace of fxc_print (the brace under the `pm_log_evt("fxcenc` line, :250).
```c
/* SPM0: the star power census (pm_warp.c spmove_tick). take 1 = snapshot only. Counts only (CLAUDE.md 2). */
void pm_log_prof_line(const char* fmt, ...); void pm_gpu_gw_now(unsigned o[4]); extern unsigned pm_rom_lg_n, pm_rom_lg_b, pm_rom_spr_n, pm_fx_new_n, pm_spm_t3, pm_spm_cap, nd_rnr_n;
static FxCen spm_s; static unsigned spm_w[4], spm_x[7];
void pm_spm_cen(unsigned k, unsigned f, int bs, int take) {
    FxCen c; unsigned w[4], x[7] = { pm_rom_lg_n, pm_rom_lg_b, pm_rom_spr_n, pm_fx_new_n, pm_spm_t3, pm_spm_cap, nd_rnr_n }; int i, live = 0;
    fxc_take(&c, f); pm_gpu_gw_now(w);
    if (!take) { for (i = 0; i < 96; i++) if (gEffectInstances[i]) live++;
        pm_log_prof_line("spm k%u f%u bs %d n %u gd %u gw %u nd %u tri %u tm %u tkb %u spl %u mm %u live %d fn %u rr %u rkb %u rs %u t3 %u cap %u u %u", k, f, bs, f - spm_s.f,
            w[0] - spm_w[0], w[1] - spm_w[1], c.cnt[C_NDRAW] - spm_s.cnt[C_NDRAW], c.cnt[C_TRI] - spm_s.cnt[C_TRI], c.cnt[C_TEXMISS] - spm_s.cnt[C_TEXMISS], (c.cnt[C_TEXBYTE] - spm_s.cnt[C_TEXBYTE]) >> 10,
            c.g[3] - spm_s.g[3], c.mmiss - spm_s.mmiss, live, x[3] - spm_x[3], x[0] - spm_x[0], (x[1] - spm_x[1]) >> 10, x[2] - spm_x[2], x[4] - spm_x[4], x[5] - spm_x[5], x[6] - spm_x[6]); }
    spm_s = c; memcpy(spm_w, w, sizeof w); memcpy(spm_x, x, sizeof x);
}
```
E4 pm_warp.c: put this block above `static void battle_hooks(unsigned f) {` (:1787). Add `    spmove_tick(f); /* SPM0 */` as the first line of battle_hooks (above `encwarp_tick(f);`).
```c
/* SPM0: spmove=<i>[:<i>:<i>] runs StarPowersTable[i] at the next player menus. States: 0 wait battle, 1 wait menu, 2 move, 3 and 4 post windows, 9 off. */
void pm_spm_cen(unsigned k, unsigned f, int bs, int take); extern int pm_cnt_on; void pm_log_prof_line(const char* fmt, ...);
static void spmove_tick(unsigned f) {
    static int st = -1, n, q[3], qi; static unsigned k, f0; char v[16]; const int bs = gBattleState;
    if (st < 0) { st = 9; if (pm_debug_value("spmove", v, sizeof v) && (n = sscanf(v, "%d:%d:%d", &q[0], &q[1], &q[2])) > 0) { st = 0; pm_cnt_on = 1; } }
    if (st == 9) return;
    if (st == 0) { if (get_game_mode() == GAME_MODE_BATTLE) { pm_spm_cen(0, f, bs, 1); st = 1; } return; }
    if (st == 1) { if (gBattleState != BATTLE_STATE_PLAYER_MENU || gBattleSubState != BTL_SUBSTATE_INIT) return;
        const int i = q[qi]; if (i < 0 || i > 9) { pm_log_prof_line("spmove: bad index %d", i); st = 9; return; }
        gPlayerData.maxStarPower = 7; gPlayerData.starPower = 7 * SP_PER_BAR;
        gBattleStatus.moveCategory = BTL_MENU_TYPE_STAR_POWERS; gBattleStatus.selectedMoveID = MOVE_FOCUS + i; gBattleStatus.moveArgument = i;
        gBattleStatus.curTargetListFlags = gMoveTable[MOVE_FOCUS + i].flags; create_current_pos_target_list(gBattleStatus.playerActor); btl_set_state(BATTLE_STATE_PLAYER_MOVE);
        pm_spm_cen(k++, f, bs, 0); pm_log_prof_line("spmove: start %d f%u", i, f); st = 2; f0 = f; return; }
    if (st == 2) { if (gBattleState != BATTLE_STATE_PLAYER_MOVE) { pm_spm_cen(k++, f, bs, 0); st = 3; f0 = f; } else if (f - f0 >= 30u) { pm_spm_cen(k++, f, bs, 0); f0 = f; } return; }
    if (f - f0 < 30u) return;
    pm_spm_cen(k++, f, bs, 0); f0 = f; if (++st < 5) return;
    pm_log_prof_line("spmove: end %d f%u", q[qi], f); if (++qi < n) { st = 1; return; }
    pm_log_prof_line("spmove: done f%u", f); st = 9;
}
```
E5 azt.d/SPM0.json: `{"scenarios": [...]}`, 3 entries spm_uaa, spm_sbeam, spm_pbeam in the form of dmg_star (azt_scenarios.json). debug `warp=kmr_20:0 battle=BTL_KMR_1_FORMATION_00:BTL_KMR_1_STAGE_01 battlef=30 spmove=<7|8|9> det fixedseed loglevel=debug`, extra `["-Turbo"]`, until `spmove: done`, cap 45, deadline 40, first_marker `warp: kmr_20 entry 0`, expects `warp`, needs_picture false, expected_events `spmove: start` and `spmove: done`.
E6 bench.d/SPM0.json: 3 benches of the same names in the form of bench.d/MISCP.json: kind `azahar_stream`, until `spmove: done`, cap 45, tags `["azahar","det","spm"]`, gates `[]`.
Tests in this order (Azahar budget 8 starts, the row needs 6):
1. `PM_LANE=SPM0 sh 3ds/build.sh`. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) passes. `python 3ds/tools/c0gate.py <elf>` gives the main values.
2. Gate: `bench.py base spot_mac01_drec --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_drec --row SPM0 --why "SPM0 gate"`, `bench.py cmp spot_mac01_drec --row SPM0`: byte-exact.
3. Census: `bench.py run spm_uaa,spm_sbeam,spm_pbeam --row SPM0 --why "SPM0 census"`. Each log has `spmove: start`, 5 or more `spm k` lines and `spmove: done`.
   Self-test (seeded by the moves): spm_uaa has t3 above 0 in a window after `spmove: start`. spm_sbeam and spm_pbeam have t3 0 in all windows and fn above 0 after `spmove: start`.
Fix scope: a compile fault in E1 to E4, a JSON fault in E5 or E6, a cap hit (cap 45 max, keep -Turbo). Any other fail (no `spmove: start`, a crash, a self-test miss): SendMessage the log path to the oracle. Change nothing else.
Stop: tests 1 to 3 pass. The last commit deletes this page. Hand-back: commit hash, c0gate values, the cmp line, the 3 log paths, all `spm` and `spmove` lines of the 3 logs.
