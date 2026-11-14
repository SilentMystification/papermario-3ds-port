# ws_FXENT3: the 42 effect nd_rec_dl rest sites get a stated entry (Haiku; oracle 10, main f8a63e24a)
Objective: C10 recdl falls (FACT listcen f8a63e24a: 42 `nd_rec_dl(` lines in src/effects, list build3ds/out/fxent2_rest.txt of main). Draws stay byte-equal. Classes (FACT code read): 13 multi sites get a branch key; 1 site (recover.c:205) is ND_SELF (its 3 lists are in ntab_tables.txt); chapter_change.c:163 (nt)
joins the converter; the other unseen sites get fxtest variants and a state-only log. Owned files: the files below only. Branch FXENT3.
## Group 1 (one commit): tools, site keys, scenes. Host: `sh 3ds/tools/nd_host_test.sh` PASS and `sh 3ds/tests/entapply/run.sh` PASS. Then one build, rc 0.
1. 3ds/include/nd_gtab.h, after the `#define nd_draw_list` line, add:
   `void nd_rec_dl_key(const void* list, unsigned k); /* FXENT3: capture form; key k 0..14 goes to bits 28-31 of the entcap site word as k + 1 */` `#define nd_rec_dl_k(list, k, n) nd_rec_dl_key((list), (unsigned)(k)) /* n = the key count; entapply.py reads it */`
2. 3ds/src/nd_gtab.c: after `static const NdPsoDesc* dl_ent;` add `static uint32_t dl_site; /* FXENT3: site | (k + 1) << 28 of nd_rec_dl_key, 0: none */`. In the r4 line (:132) the third word `(uint32_t)(uintptr_t)__builtin_return_address(0)` becomes `(dl_site ? dl_site : (uint32_t)(uintptr_t)__builtin_return_address(0))`. After the nd_rec_dl_e line (inside the #endif) add: `void nd_rec_dl_key(const void* list, unsigned k) { dl_site = (uint32_t)(uintptr_t)__builtin_return_address(0) | ((k & 15u) + 1u) << 28; nd_rec_dl(list); dl_site = 0; }`
3. 3ds/src/nd_walk.c: before `static void gt_run(` add `entcap_so`, a copy of entcap_nt (nd_gtab.c:104-106) that logs `"entcap so l %08x s %08x"`. As the first line in `if (rec[1]) { /* state-only */` add `if (__builtin_expect(PMDL("entcap"), 0)) entcap_so(key, rec[2]);` (rec[2] exists under entcap: nd_gtab.c:132).
4. 3ds/tools/entcap.py: SO = `entcap so l (\w{8}) s (\w{8})` prints `so <list> <fn> <file:line>`. For l and so lines: `kb = site >> 28; site &= 0x0fffffff`; when kb > 0 append `#<kb - 1>` to loc.
5. 3ds/tools/entapply.py: SITE gets `(?:#(\d+))?` (key). A `so` line marks (file, line). Edit sites in descending line order per file. Per site line:
   `^(\s*)nd_rec_dl_k\((.+), (.+), (\d+)\);\s*$` (arg, K, N): every key 0..N-1 has exactly one tuple, else print `entapply kmiss <f>:<l> <keys>` or `entapply multi <f>:<l>#<k> <n>`, no edit.
   Else the line becomes the switch below, with desc pe_<stem>_<line>_k<k> per key. A site with so lines only: `nd_rec_dl(<arg>);` -> `nd_draw_list(<arg>, ND_SELF);`, print
   `entapply so <f>:<l>`. so and l lines at one site: `entapply mixed <f>:<l>`, no edit. Last line: `entapply sites <n> keyed <k> so <s> multi <m> kmiss <q> mixed <x> skip <s> other <o> files <f>`.
   Before `confetti.c:310  nd_rec_dl_k(spA0, (unk_24 + i) % 3, 3);`  After (each key one line, the last key is default):
   `switch ((unk_24 + i) % 3) {` / `    case 0: nd_draw_list(spA0, pe_confetti_310_k0); break;` / `    case 1: ...k1); break;` / `    default: nd_draw_list(spA0, pe_confetti_310_k2); break;` / `}`
   Fixture 3ds/tests/entapply/case: add one keyed site (2 keys), one kmiss site, one so site to fx.in.c and entcap.txt; regenerate fx.out.c and stdout.out; check them by hand against this item.
6. 3ds/tools/gen_gfx_syms.py scan(): first statement after the read: `text = JP.sub("", text)`, `JP = re.compile(r"^#if VERSION_JP\b.*?^#(?:else|endif)\b", re.M | re.S)`. New file 3ds/tools/gconv_src.d/FXENT3.txt, one line: `chapter_change`. Host check: the `gconv ntab tables` line of nd_host shows the counts; write them in the commit.
7. Site keys (src/effects; text anchors, the line numbers are main). fxk = `s32 fxk = 0;` on a new line before the named `for (`, and `fxk = 1;` on a new line after the call.
   cold_breath:234 `nd_rec_dl_k(D_E00DE810[unk00][temp_s32], unk00 * 2 + cond, 6);` | confetti:310 `nd_rec_dl_k(spA0, (unk_24 + i) % 3, 3);`
   effect_46:246 `nd_rec_dl_k(D_090003A0_38ED30, fxk, 2);` fxk, for :228 | floating_rock:136 `nd_rec_dl_k(D_E0098510[data->type], alpha != 255, 2);`
   gather_energy_pink:224 `nd_rec_dl_k(dlist2, unk_00 * 2 + fxk, 4);` fxk, for :205 | green_impact:202 `nd_rec_dl_k(dlist2, fxk, 2);` fxk, for :186
   misc_particles:330 `nd_rec_dl_k(D_E00E4D90[variation], variation * 2 + fxk, 12);` fxk, for :317 | red_impact:209 `nd_rec_dl_k(dlist2, fxk, 2);` fxk, for :193
   recover:237 `nd_rec_dl_k(D_090033F0_3851F0, (type == 0 || type == 2) ? (unk_44 == 0 ? 0 : 1) : 2, 3);` | ring_blast:141 `nd_rec_dl_k(dlist, unk_00 != 0, 2);`
   shockwave:341 `nd_rec_dl_k(dlist, (temp_v0 >= 2) * 2 + fxk, 4);` fxk, for :326 | stars_spread:153 `nd_rec_dl_k(dlist, fxk, 2);` fxk, for :144
   stat_change:274 `nd_rec_dl_k(D_E00AC7BC[arrowType], (data->unk_24 != 255) * 3 + arrowType, 6);` | recover:205 `nd_rec_dl(` -> `nd_draw_list(`, `);` -> `, ND_SELF);`
8. 3ds/src/pm_fxtest.c: add ` \` to the gather_energy_pink_t1 line, then 12 entries (index 128-139, ` \` on all but the last): E(landing_dust_t2, fx_landing_dust(2, X, Y, Z, 0)),
   E(damage_indicator_12, ..(0, X, Y, Z, 10.0f, 0, 12, &oe)), E(disable_x_t1, ..(1, X, Y, Z, 0)), E(disable_x_d3, ..(0, X, Y, Z, 3)), E(disable_x_d12, ..(0, X, Y, Z, 12)),
   E(energy_orb_wave_t0, ..(0, X, Y, Z, 1.0f, 20)), E(fire_breath_t1, ..(1, X, Y, Z, X + 10, Y, Z, 5, 2, 30)), E(ice_shard_t1, ..(1, X, Y, Z, 1.0f, 30)),
   E(recover_d5, fx_recover(0, X, Y, Z, 5)), E(recover_d15, ..(0, X, Y, Z, 15)), E(recover_t1, ..(1, X, Y, Z, 15)), E(rising_bubble_a30, ..(0, X, Y, Z, 30.0f)).
9. 3ds/tools/azt.d/FXENT3.json and bench.d/FXENT3.json (copy the fxent_a and fxpic_91_drec entries of ENTRYFX.json and GOLDFX1.json; speed turbo, cap 45, deadline 40). L = 1,14,17,20,25,30,31,32,39,44,45,49,58,59,62,64,70,71,76,80,87,98,104,105,107,113,114,115,117,123,127 (the rest effects, FACT pm_fxtest.c order).
   fxr_cap: `warp=mac_01:0 pos=0:0:200 det fxtest=L,128,...,139 loglevel=debug entcap psochk`, until `fxtest: done`. fxr_chk: the same with `psochk` only.
   fxr_drec (azahar_pic, tags of fxpic_91_drec): `warp=mac_01:0 pos=0:0:200 fxtest=L loglevel=debug det drec=412:31:44`, until `drec f1732 draws [1-9]\d* bytes \d+`.
## Group 2 (one commit): capture, apply, gate. Azahar runs: 4 of 8.
1. `bench.py run fxr_cap`; `python 3ds/tools/entcap.py <log> > build3ds/out/fxent3_cap.txt`; B0 = `bad` of the last psochk line of the log.
2. `python 3ds/tools/entapply.py build3ds/out/fxent3_cap.txt > build3ds/out/fxent3_apply.txt`. Each `nd_rec_dl_k(` line left: back to `nd_rec_dl(<arg>);`, delete its fxk lines. `grep -rn "nd_rec_dl_k(" src` returns nothing.
3. nd_host PASS, one build rc 0. `bench.py run fxr_chk`: `bad` of its last psochk line == B0 (one differing count fails).
4. `bench.py base fxr_drec --row FXENT3 --commit $(git merge-base HEAD main)`, `bench.py cmp fxr_drec --row FXENT3 --commit $(git merge-base HEAD main)`: byte-equal.
5. `python 3ds/tools/listcen.py`: recdl before and after. build3ds/out/fxent3_rest.txt: each `nd_rec_dl(` line of src/effects with its entapply class (multi, kmiss, mixed, unseen). Fix rule: a host or build failure in an edit of this page: fix that edit. A failed gate in step 3 or 4: revert that site to `nd_rec_dl(`, 2 tries, then stop.
Stop: group 2 committed, or a tool fault (CLAUDE.md 4). Delete this page in the last commit. Hand-back: build3ds/out/handback_FXENT3.txt, 8 lines:
commit, recdl before/after, entapply last line, B0 and fxr_chk bad, fxr_drec draws and cmp result, gconv tables line before/after, rest count, Azahar runs.
