# ws_NTABFIND (Haiku): nd_rec_ndev records a copy of the NdNDev, not its address (crash ent_compile nd_ent.c:278)
Designer: the oracle (SendMessage). Worktree build3ds/wt/NTABFIND, PM_ROW=NTABFIND PM_LANE=NTABFIND. Main at release: e73be922a1.
Files: 3ds/include/nd_gtab.h, 3ds/src/nd_gtab.c, 3ds/src/nd_walk.c, new 3ds/tools/nd_ndev_rec_test.c, new 3ds/tests/host.d/ntabfind.sh.
## Objective
- FACT (ENTCAPE lane slot1/last_unmapped.txt): 8192 unmapped reads, 0x0 to 0x7FFC. That is ent_flat(NULL) over 4096 units.
  So ent_compile (nd_ent.c:278-279) gets l0 = NULL. No ndn_dev_idx entry is bad.
- FACT (code): nd_gtab.c:138 records the address of the entry. imgfx.c:1768 passes &state->gfxDev[ctx]. imgfx.c:476, 481,
  491, 1271, 1278 set gfxDev[i].list = NULL. Core 0 runs the record later (nd_wq.c depth-1 frame push). Then gt_run
  (nd_walk.c:630, ntri != 0 from imgfx.c:1240) calls ent_gdl(NULL) (nd_walk.c:631).
- FACT: merge NIMGM c7ac535507 brought it in. wscmb_pause_drec crashes on main 31fb4b4d9 (ENTSELF out/entself_wscmb_base.txt).
- Counters: C1..C12 do not change (no list call site changes).
## Design (all in docs/handoff/ws_NTABFIND.patch; edit only these 5 files)
nd_gtab.h: nd_ndev_pack and nd_ndev_unpack (rec[2] site, rec[4..8] the NdNDev copy, 5 units). nd_walk.c: nd_ndev_rec runs
the copy through gt_run. nd_gtab.h uses __builtin_memcpy, not <string.h> (FACT: <string.h> breaks 43F0.c with include/gcc/memory.h).
FACT: `git apply --check` OK on main de02cb1eb4. FACT: ENTCAPE 7c8391e8df+f8a9b52284 ran entcape_cap to the end, rc 0, no unmapped read.
## Edit shape (nd_gtab.c:138)
Before: `void nd_rec_ndev(const NdNDev* e) { const uint32_t rec[2] = { (uint32_t)(uintptr_t)e, 0u }; nd_rec_napi(nd_ntab_rec, rec, 1u); }`
After: `void nd_rec_ndev(const NdNDev* e) { uint32_t rec[2u * ND_NDEV_REC_UNITS]; nd_ndev_pack(rec, e, ...); nd_rec_napi(nd_ndev_rec, rec, ND_NDEV_REC_UNITS); }`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_NTABFIND.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 5 files: `NTABFIND 1: nd_rec_ndev records a copy of the NdNDev (crash ent_compile nd_ent.c:278, from NIMGM)`.
3. `sh 3ds/tools/rowloop.sh --row NTABFIND --bench nimgm_anim_drec,spot_mac01_drec --why "NTABFIND gate" > build3ds/out/ntabfind_loop.txt 2>&1`
4. `python 3ds/tools/bench.py run wscmb_pause_drec --row NTABFIND --why "NTABFIND crash gone" > build3ds/out/ntabfind_pause.txt 2>&1`
## Tests that decide
- Host: `ntabfind ok` and `ND HOST TESTS PASS`. ntabfind.sh holds the seeded fault: the record stores the address. It must
  print `ndev FAIL case 0`. The script fails when the seed passes.
- `LOOP PASS`: nimgm_anim_drec and spot_mac01_drec byte-equal against `git merge-base HEAD main`.
- Step 4: rc 0, the until-line `drec f141 draws` appears, no `crash symbols` line. Do not cmp it: the base crashes (FACT).
## What you may edit to fix a fail
Only the 5 files. A DIFF, a crash or a failed base run: stop. Send the output path to the oracle. Budget: 1 build, 3 Azahar runs.
## Stop and hand-back
Last commit `NTABFIND 2: gate. nimgm_anim_drec, spot_mac01_drec byte-equal; wscmb_pause_drec rc 0 no crash; host ntabfind ok`
deletes ws_NTABFIND.md and the patch. One SubagentHandback: commit hashes, the LOOP lines, the step 4 rc and until-line.
