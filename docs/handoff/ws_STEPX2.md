# ws_STEPX2 (Haiku): 4 more stepx fields: audio sample decodes, ROM bytes, Yay0 decodes, effect instances
Designer: oracle NATIVE19 (plan_walk.md 2 rows 8, 9). Worktree build3ds/wt/STEPX2, branch STEPX2. PM_ROW=STEPX2 PM_LANE=STEPX2. Start: main after STEPX merges.
Why Haiku: 3 counters, 4 reads in sx_step, 2 print changes, given below. No draw change. Indentation: as in the file.
FACT: the stepx code of ws_STEPX is in 3ds/src/pm_phase.c (enum SX_*, sx_step, the stepx prints). pm_dspa_dec_read (3ds/src/pm_dspaud.c:37) gives the sample decodes
since boot. The game makes effects only through create_effect_instance (src/effects.c:197). Yay0 data goes through nd_yay0_raw (3ds/src/nd_yay0.c:11).
Objective: the stepx slow, norm and max lines end with ` a <n> b <n> y <n> e <n>`. Counter: none (it decides plan_walk.md rows 8 and 9).
## Edits (CRLF files: use the Edit tool; `/* STEPX2 */` on each new line)
1. 3ds/src/pm_os.c: next to `unsigned pm_rom_lg_n;` add `unsigned pm_rom_lg_b;`; next to `pm_rom_lg_n++;` add `pm_rom_lg_b += size;`.
2. 3ds/src/nd_yay0.c: above `int nd_yay0_raw(` (:11) add `unsigned pm_yay0_n;`; first line inside nd_yay0_raw: `    pm_yay0_n++;`.
3. src/effects.c:197-200: after the line `    s32 i;` of create_effect_instance add `    { extern unsigned pm_fx_new_n; pm_fx_new_n++; }`.
   3ds/src/pm_phase.c: next to `volatile signed char pm_act_state;` add `unsigned pm_fx_new_n; extern unsigned pm_rom_lg_b, pm_yay0_n; void pm_dspa_dec_read(unsigned* n, unsigned* us, unsigned* max);`
4. pm_phase.c enum: before `SX_F` add `SX_A, SX_B, SX_Y, SX_E,` (audio decodes, ROM bytes, Yay0 decodes, effect instances).
5. sx_step, after `v[SX_M] = pm_mdl_miss_n;` add:
   `    { unsigned an_, au_, am_; pm_dspa_dec_read(&an_, &au_, &am_); v[SX_A] = an_; } v[SX_B] = pm_rom_lg_b; v[SX_Y] = pm_yay0_n; v[SX_E] = pm_fx_new_n;`
6. The 2 `stepx %s` lines and the `stepx max` line: append ` a %u b %u y %u e %u` to the format. slow and norm: a, y, e = sum * 100 / n (counts x 100 per step),
   b = sum / n (bytes per step). max: the raw sx_mv values. Keep the zeroing of the sums after the print.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=STEPX2 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`; run and cmp with `--row STEPX2`: byte-equal.
3. `python 3ds/tools/capture_check.py --row STEPX2 --proof "stepx max" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   The proof line must end with the 4 new fields. Never add phase=on.
Fix rights: compile errors, declaration placement, a host link error from a new extern (define the counter in its own file). A DIFF or a base fault: stop, send the output path to oracle NATIVE19.
Stop: commit `STEPX2 1: gate. <host> <c0gate> <cmp line> <CAPTURE CHECK line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
