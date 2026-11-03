# agent_uc3b: handoff (row UC3, branch uc2c). Read this first.

## 1. State
- Branch uc2c. HEAD b728404af (code), docs commit follows.
- UC3M (per-class count) and UC3b step one (staging refusal) are committed in b728404af.
- mac01t_rec_chk gate for UC3b PASSES (START 6, lane UC3, build 02:30:56):
  window f307 bad 0, miss 0, hit 2872 (was 2878), adr 0 (was 184), stg 62 captures refused.
  Lane shots 0000_1_2 and 0001_1_2 byte-equal to base (C:\Users\Andrew\AppData\Local\Temp\claude\uc2c\base_mac01t_rec).
- yard_rec gate for UC3b: NOT measured. Starts 7, 8, 9 (binary b728404af) exited after shot 0 at frame 203.
  The base build (345d9c427 state) also exits: base runs gave 2 fails and 1 pass in 3 (START 1 to 3, row UC3base).
  The pass gave rec f349 hit 2003 bad 0 miss 0 (same as the UC2 yard gate). Acceptance: the coordinator counts the yard gate as not regressed.
  Investigation of the early exits: agent AZFLAKY. Do not spend starts on it.
- The working tree 3ds/src/pm_gpu.c, pm_gbi.c, pm_tex.c, 3ds/include/pm_tex.h equal b728404af.
- 3ds/src/map_restore_table.c is the user file: never commit, never discard. Current copy is the saved one.

## 2. Measured table (rec lines, lane UC3)
| Build | Scene | Window | hit | bad | miss | adr | stg | Top shot |
|---|---|---|---|---|---|---|---|---|
| UC2 HEAD (UC2 mask) | mac01t_rec_chk | f307 | 2878 | 0 | 0 | 184 (per UC3M) | - | byte-equal |
| UC3M (b728404af mask unchanged) | mac01t_rec_chk | f307 | 2878 | 0 | 0 | 184 | 0 | byte-equal (START 5) |
| UC3b (b728404af) | mac01t_rec_chk | f307 | 2872 | 0 | 0 | 0 | 62 | byte-equal (START 6) |
| UC3b (b728404af) | yard_rec | f348 | not read | - | - | - | - | 0000_0_11 byte-equal (START 8) |
| base | yard_rec | f349 | 2003 | 0 | 0 | - | - | not compared |

UC3M split at mac01t f307 (CONFIRMED, START 5): oth 1384, adr 184, pal 0, uni 0.
- adr = differing words at texture address registers 0x085, 0x095, 0x09D. The UC3a hypothesis that uniform (non-palette) words differ is REFUTED for mac01t f307.
- oth is not split. It holds the 0x227 index address words and other masked words. Not measured per register.

## 3. Code changed (b728404af)
- pm_gpu.c pm_gpu_rec_same: rec_cl class array (0 other, 1 address, 2 palette, 3 uniform); counts all differing words (rec_cc); pm_gpu_rec_cls() accessor resets them.
- pm_gbi.c rec_report: the rec line now ends with `stg <n> cls oth <n> adr <n> pal <n> uni <n>`.
- pm_gbi.c rec_done: after the emp check, `if (pm_tex_cur_vram_pending() || pm_tex_aux_vram_pending()) { k->state = 0; rec_stg++; return; }`.
- pm_tex.c / pm_tex.h: new pm_tex_aux_vram_pending().
- Compare mask (UC2) unchanged.

## 4. Open items
- yard_rec flaky on this machine (base too). AZFLAKY owns the cause.
- UC3a retry (pal-only mask with the staging refusal) NOT started. Plan: plan_uc3.md section 4, row UC3a. Mask change: pm_gpu_rec_same. The adr words are now the only remaining known diff at mac01t f307. Expect bad 0 under a pal-only mask only if adr stays 0 (it is 0 with the refusal) and oth is equal. Gate: mac01t f307 bad 0, miss 0, hit > 0, top shots byte-equal.
- UC3c (index arena) NOT started. Plan: plan_uc3.md section 4, row UC3c.
- Budget: UC3b 4 of 8 starts used. UC3base 3 of 8. UC3M 1 of 3.
- Azahar: a shared mutex Global\pm_azahar_run in 3ds/run_azahar.ps1 serialises runs. Call azt.ps1 and wait. Only exit 3 (LOCK TIMEOUT) means retry once.
- The azt marker check reads the shared sdmc log. When a run looks marker-missing, read build3ds/lane-UC3/last_log.txt for the rec lines and shot files instead.

## 5. Next steps
1. UC3a retry: edit pm_gpu_rec_same mask to pal-only (patch slots with idx in [u4_pal, u4_pal + 4 * GPAL) plus 0x227 p0). Keep the cls counters. Build with PM_LANE=UC3. Gate mac01t_rec_chk f307.
2. If bad 0: commit. Then UC3c per plan_uc3.md section 2a and section 4.
3. If bad > 0: read the cls counts. Revert the mask edit (git checkout -- 3ds/src/pm_gpu.c) before the next stage.

## 6. Commands
- Build: `python 3ds/tools/gen_map_restore.py`, `docker rm -f pm3ds-build-UC3` (if a stale container exists), `PM_LANE=UC3 sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from C:\Users\Andrew\AppData\Local\Temp\claude\uc3\map_restore_table.c.saved.
- Start: `PM_ROW=<row> PM_LANE=UC3 timeout 900 powershell -NoProfile -File 3ds/tools/azt.ps1 <scenario> -Row <row> -Build UC3 -Try`.
- Justification line: append to 3ds/logs/perf/status_UC3.txt before each start.

## 7. Acronyms
UC3 = replay row. UC2 = reccheck row. UC3M = per-class count row. UC3b = staging refusal row. adr = address class (texture address words). pal = palette uniform class. uni = other uniform class. oth = other differing words. stg = captures refused for a staged slot. stg/staged = slot in its load frame, draws its linear copy. hit = compares. bad = compared words that differ. miss = patch tables that differ. VRAM = video RAM. FCRAM = main RAM. sdmc = Azahar virtual SD folder. AZFLAKY = investigation of the early Azahar exit.
