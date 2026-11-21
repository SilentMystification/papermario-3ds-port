# ws_STEPX (Haiku): per-step census: player state bucket, step time, the causes of slow steps, the longest step (prof lines `stepx`)
Designer: oracle NATIVE19 (plan_walk.md 3, 4). Worktree build3ds/wt/STEPX, branch STEPX. PM_ROW=STEPX PM_LANE=STEPX. Start: main 0eb73a88e7 or later; grep the quoted text.
Why Haiku: 5 counter increments, one helper, one call, 4 print lines, given below. No draw change. Indentation: as in the file.
FACT: pm_pf_frame (3ds/src/pm_phase.c:764) runs once per drawn step; w = t0 - pf_last is the wall of the step before (:773-777); PF_OVER_T marks a slow step.
Objective: per window `stepx n <idle> <walk> <spin> <other> ft <ms x100 x4> slow <x4>`, `stepx slow|norm n g p d r s t m`, `stepx max ft b g p d r s t m`. Counter: none.
## Edits (CRLF files: use the Edit tool)
1. Counters (each defined in its own file; `/* STEPX */` on each line):
   - 3ds/src/pm_os.c:492, inside `if (!pm_on_audio()) {` as its first line: `        pm_rom_lg_n++;` and above `void nuPiReadRom(` (:490): `unsigned pm_rom_lg_n;`
   - src/101b90_len_8f0.c:361, before `    nuPiReadRom(SpriteDataHeader[0] + (playerRasterInfo & 0xFFFFF), ...`: `    { extern unsigned pm_rom_spr_n; pm_rom_spr_n++; }`
     and in pm_os.c next to pm_rom_lg_n: `unsigned pm_rom_spr_n;`
   - 3ds/src/pm_tex.c: after `void pm_pipe_note(unsigned k); /* pm_gpu.c PIPE0 */` (:15) add `unsigned pm_tex_miss_n;`; after `    pm_pipe_note(2u); /* PIPE0: PP_TEX */` (:1417) add `    pm_tex_miss_n++;`
   - 3ds/src/nd_recs.c: above `static __attribute__((cold, noinline)) unsigned rec_miss(` (:184) add `unsigned pm_mdl_miss_n;`; first line inside rec_miss:
     `    if (tried || !mce) pm_mdl_miss_n++; /* STEPX: no cache entry, or a failed hit (reads no record word: lkey census) */`
   - src/77480.c:1417, after `    playerStatus->animNotifyValue = spr_update_player_sprite(PLAYER_SPRITE_MAIN, playerStatus->trueAnimation, timescale);`:
     `    { extern volatile signed char pm_act_state; pm_act_state = playerStatus->actionState; }`
2. pm_phase.c, above `static void pf_report(void) {` (:579; pf_report and pm_pf_frame both use these):
   `volatile signed char pm_act_state; extern unsigned pm_rom_lg_n, pm_rom_spr_n, pm_tex_miss_n, pm_mdl_miss_n; extern unsigned long long pm_c0i_t[3], pm_c0k[6]; void pm_wq_pf(unsigned out[2]);`
   `enum { SX_G, SX_P, SX_D, SX_R, SX_S, SX_T, SX_M, SX_F }; /* GPU waits, pace, logic-thread waits less slot waits, ROM reads, raster reads, tex misses, model misses */`
   `static unsigned sx_last[SX_F]; static int sx_b, sx_mb; static unsigned long long sx_n[4], sx_ft[4], sx_sl[4], sx_v[2][SX_F], sx_vn[2], sx_max, sx_mv[SX_F];`
   `static void sx_step(int have, unsigned long long w) { unsigned bw[2], v[SX_F]; pm_wq_pf(bw);`
   `    v[SX_G] = (unsigned)(pm_c0i_t[2] + pm_c0k[1]); v[SX_P] = (unsigned)(pm_c0i_t[1] + pm_c0k[0]); v[SX_D] = bw[0] - bw[1];`
   `    v[SX_R] = pm_rom_lg_n; v[SX_S] = pm_rom_spr_n; v[SX_T] = pm_tex_miss_n; v[SX_M] = pm_mdl_miss_n;`
   `    if (have) { const int b = sx_b, sl = w > PF_OVER_T; sx_n[b]++; sx_ft[b] += w; sx_sl[b] += (unsigned)sl; sx_vn[sl]++;`
   `        for (int k = 0; k < SX_F; k++) sx_v[sl][k] += v[k] - sx_last[k];`
   `        if (w > sx_max) { sx_max = w; sx_mb = b; for (int k = 0; k < SX_F; k++) sx_mv[k] = v[k] - sx_last[k]; } }`
   `    memcpy(sx_last, v, sizeof v);`
   `    const int a = pm_act_state; sx_b = a == 0 ? 0 : (a == 1 || a == 2) ? 1 : a == 0x1A ? 2 : 3; } /* include/enums.h:2415-2417, :2443 */`
3. pm_pf_frame: before `    pf_last = t0;` (:778) add `    sx_step(pf_last && !pf_prev_s, pf_last ? t0 - pf_last : 0ull); /* STEPX */`
4. pf_report: after the PIPE0 print block (the statement that starts `{ unsigned pp[14];`, :645-647) add one block that prints and then zeroes
   sx_n, sx_ft, sx_sl, sx_v, sx_vn, sx_max, sx_mv:
   - `prof f%u w%u stepx n %u %u %u %u ft %u %u %u %u slow %u %u %u %u`: sx_n[b]; ft = sx_ft[b] * 100 / PF_TPM / sx_n[b] (0 when n is 0); sx_sl[b].
   - 2 lines `prof f%u w%u stepx %s n %u g %u p %u d %u r %u s %u t %u m %u` for "slow" (sx_v[1]) and "norm" (sx_v[0]): g, p, d = sum * 100 / PF_TPM / n;
     r, s, t, m = sum * 100 / n (counts x 100 per step); n = sx_vn (use 1 in the division when 0).
   - `prof f%u w%u stepx max ft %u b %d g %u p %u d %u r %u s %u t %u m %u`: ft = sx_max * 100 / PF_TPM; g, p, d = sx_mv * 100 / PF_TPM; r, s, t, m raw.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=STEPX sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row STEPX`: byte-equal.
3. `python 3ds/tools/capture_check.py --row STEPX --proof "stepx n" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK. Never add phase=on.
Fix rights: compile errors, declaration placement, a host link error from a new extern (define the counter in that file). A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `STEPX 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
