# ws_IK0 (Haiku): K frame cost census: kc_build and key show CPU, patches, patched draws, equal copies, twin loads, core 1 lerp and leaf pairs (INTERP row 1)
Oracle NATIVE23 (plan_interp.md 1, 2, 4). Worktree build3ds/wt/IK0, branch IK0, PM_ROW=IK0 PM_LANE=IK0. Start: main. ws_PRES0 edits the same files after this row.
Why Haiku: counters and 2 prof lines, all given below. No draw change. Counter: none of C1..C12. Row metric: the `kcost` and `kcost1` lines. Order: 3ds/src/pm_interp.c, pm_nusys.c, nd_mtx_set.c, pm_gpu.c, pm_phase.c. Sources are CRLF: use the Edit tool. Each edit ends with the comment `/* IK0 */`.
## Edits
1. pm_interp.c above pm_interp_lerp (:136): `static unsigned ik_lv, ik_pr, ik_st, ik_rn, ik_rl;` and `void pm_interp_ik(unsigned o[5]) { o[0] = ik_lv; o[1] = ik_pr; o[2] = ik_st; o[3] = ik_rn; o[4] = ik_rl; }`.
   Leaf loop (:152-173): `if (!ok) continue;` becomes `if (!ok) { ik_rn++; continue; }`. Before `h = hash4(l->node, l->idx, 0, 0)` (:159) add `int hit = 0;`. After
   `if (o->node != l->node || o->idx != l->idx) continue;` (:162) add `hit = 1; if (!memcmp(o->mf, l->mf, sizeof o->mf)) ik_st++;`. After the inner for loop (:172)
   add `if (!hit) ik_rl++;`. After `pair_leaves += paired;` (:174) add `ik_lv += nleaf; ik_pr += paired;`.
2. pm_nusys.c: after `volatile int pm_lerp_busy; unsigned pm_slot_race;` (:124) add `unsigned long long pm_ik_lt;`. Edit shape (wrap a call with ticks), :172:
   before: `... __sync_synchronize(); pm_interp_lerp(0.5f); __sync_synchronize(); ...`
   after:  `... __sync_synchronize(); { const uint64_t lt0_ = pm_pf_on ? svcGetSystemTick() : 0; pm_interp_lerp(0.5f); if (pm_pf_on) pm_ik_lt += svcGetSystemTick() - lt0_; } /* IK0 */ __sync_synchronize(); ...`
3. nd_mtx_set.c: after `extern int pm_tw_on;` (:43) add `extern unsigned long long pm_ik[10];`. After `int lm = nd_mtx_replace(src, in);` (:137) add `if (pm_tw_on) pm_ik[lm ? 7 : 8]++;`.
4. pm_gpu.c above `static unsigned ndn_tgen;` (:1270): `unsigned long long pm_ik[10]; /* IK0: 0 kc_build ticks, 1 patches of valid copies, 2 max patches (window), 3 copies with 0 patches, 4 valid copies, 5 key show ticks, 6 patched draws, 7 twin loads replaced, 8 plain */`.
5. pm_gpu.c nd_native_submit: after `NDT_B(tsub);` (:1382) add `const unsigned kp0_ = nd_kp_n;`. Edit shape (a counter line before an anchor), :1430:
   before: `    dc_kick();`   after: `    if (nd_kp_n != kp0_) pm_ik[6]++; /* IK0 */` on its own line, then `    dc_kick();`.
6. pm_gpu.c kc_build: after `kc_valid = 1; kc_ok_gen = kc_gen;` (:3190) add `if (pm_pf_on) { pm_ik[4]++; pm_ik[1] += np; if (np > pm_ik[2]) pm_ik[2] = np; if (!np) pm_ik[3]++; }`.
   At :3340 `kc_build();` becomes `{ const unsigned kb0_ = pm_pf_on ? (unsigned)svcGetSystemTick() : 0u; kc_build(); if (pm_pf_on) pm_ik[0] += (unsigned)svcGetSystemTick() - kb0_; }`.
7. pm_gpu.c pm_gpu_key_show: after the CPT0 line `if (pm_pf_on) { pm_c0k[1] += ...` (:3204) add `const unsigned ks0_ = pm_pf_on ? (unsigned)svcGetSystemTick() : 0u;`. After `c0k_kf = 1; /* CPT0 */` (:3225) add `if (pm_pf_on) pm_ik[5] += (unsigned)svcGetSystemTick() - ks0_;`.
8. pm_phase.c: after the c0k block (the line that ends `/* CPT0 */`, :755) add (prof only, one per window):
```c
            { extern unsigned long long pm_ik[10], pm_ik_lt; extern void pm_interp_ik(unsigned o[5]); /* IK0 */
              static unsigned long long m_[10], ml_; static unsigned mi_[5]; unsigned long long d_[10]; unsigned o_[5], e_[5];
              for (int k = 0; k < 10; k++) { d_[k] = pm_ik[k] - m_[k]; m_[k] = pm_ik[k]; }
              pm_interp_ik(o_); for (int k = 0; k < 5; k++) { e_[k] = o_[k] - mi_[k]; mi_[k] = o_[k]; }
              const unsigned long long lt_ = pm_ik_lt - ml_; ml_ = pm_ik_lt;
              const unsigned w_ = pf_w ? pf_w : 1u, cp_ = (unsigned)d_[4], np_ = cp_ ? (unsigned)(d_[1] / cp_) : 0u, mx_ = (unsigned)pm_ik[2];
              const unsigned kb_ = (unsigned)(d_[0] * 100ull / PF_TPM / w_), ks_ = (unsigned)(d_[5] * 100ull / PF_TPM / w_), lr_ = (unsigned)(lt_ * 100ull / PF_TPM / w_);
              pm_ik[2] = 0; m_[2] = 0;
              pm_log_prof_line("prof f%u w%u kcost kb %u.%02u ks %u.%02u np %u npmax %u pd %u eq %u cp %u tw %u %u", pf_total, pf_w, kb_ / 100u, kb_ % 100u,
                  ks_ / 100u, ks_ % 100u, np_, mx_, (unsigned)(d_[6] / w_), (unsigned)d_[3], cp_, (unsigned)(d_[7] / w_), (unsigned)(d_[8] / w_));
              pm_log_prof_line("prof f%u w%u kcost1 lrp %u.%02u lv %u pr %u st %u rn %u rl %u fm %d", pf_total, pf_w, lr_ / 100u, lr_ % 100u,
                  e_[0] / w_, e_[1] / w_, e_[2] / w_, e_[3] / w_, e_[4] / w_, pm_opt.fps60); }
```
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS (pm_interp_occ_test builds pm_interp.c). `PM_LANE=IK0 sh 3ds/build.sh`. `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_fps60_det --row IK0 --why "IK0 census"`, `bench.py cmp spot_mac01_fps60_det --row IK0`: 12/12 byte-equal.
3. `python 3ds/tools/capture_check.py --row IK0 --proof "kcost kb,kcost1 lrp" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`. Never add phase=on.
   Expected in Azahar (counts valid, ms not): cp near the steps of the window, np above 0, pd above 0, lv above 0, pr not above lv, tw above 0, eq 0 (plan 1.4).
Fix rights: compile and link errors, a missing declaration. A DIFF, a base fault, a missing line: stop, send the output path to oracle NATIVE23.
Stop: commit `IK0 1: gate. <host> <c0gate> <cmp line> <kcost line> <kcost1 line>` deletes this page. Hand-back: one SubagentHandback: commit hash and both lines.
