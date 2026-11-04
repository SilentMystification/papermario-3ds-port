# plan_mr1: G_PM_MODEL (one walker op per model draw)
Plan only (Opus). Read at HEAD b4097cd16 (code = LB1b 029a952fd). Rules: CLAUDE.md 2, 5, 7; hw_guidelines.md.
Data: build3ds/last_log.txt (mac01t census view, phase on, window f600 to f899, CF-era build, Azahar).

## 0. Result first
- The plan changed. The design does not use a retained record or a cross-frame key. It uses an inline group header (section 1).
- The 600-op gate cannot be reached on mac01t at HEAD with any MR1 design. Model-attributed ops are at most about 455 per frame (section 4).
- Expected drop: about 330 to 370 ops per frame [E]. Proposed gate: mdlsum tot ops down by 300 or more against the HEAD baseline run.

## 1. Current per-model op sequence (src/model.c appendGfx_model, 1384 to 2057)
Words emitted for a plain model (1CYC/2CYC, no tint, no custom gfx, no panner, no billboard):
| Word | Line | Counted by n_ops today? |
|---|---|---|
| G_RDPPIPESYNC | 1509 | no: g1_skip (pm_gbi.c 2717, 3466) |
| gSPSetLights1: MOVEWORD G_MW_NUMLIGHT + 2 MOVEMEM G_MV_LIGHT (only when groupData->lightingGroup) | 1511-1541 | MOVEWORD yes; MOVEMEM no when equal (g2_same R5B, 2779) |
| G_PM_TEX (or 4 untextured ops, or the make_texture_gfx words for EXTRA_TILE_AUX_INDEPENDENT with a special prop) | 1543-1571 | yes |
| SETCOMBINE (alpha test, fog or tint only) | 1574-1603 | yes unless g2_same |
| G_PM_RM | 1672 / 1736 | yes |
| G_MTX finalMtx (unless MODEL_FLAG_IGNORE_MATRIX) | 1979 / 1990 | yes |
| G_DL shape list (unless MODEL_FLAG_HAS_LOCAL_VERTEX_COPY) | 2041 | yes, 1 op on a dlc hit (pseudo op ff); a dlc miss adds the nested ops and G_ENDDL |
| G_POPMTX | 2053 | yes |
| G_RDPPIPESYNC | 2056 | no |

- Counted ops per model: 5 (no lights), 6 (lights), 7 (lights + SETCOMBINE). mdlsum f899 id rows r6 to r12: ops 7, 7, 7, 7, 7, 6, 6, binds 1, draws 0 (dlc hit).
- mdlsum f899: tot ops 1215 = opsplit ops 1215 = prod sum 1215. Rows: appendGfx_entity_model 363, task f0e2 166, appendGfx_npc 162, task f0e3 8, unattributed 61.
  The model id rows hold the rest: 1215 - 760 = 455 at most (rows below r12 hold 6 ops or less each; some may be fn/task rows).
- mr0 f899: 86 plain models per frame. mtx 197 ops per frame (MTX + POPMTX).

## 2. Design: inline group, not a retained record
- (1) Layout. One 8-byte header word in the main list, then the model's own words, unchanged:
  `w0 = G_PM_MODEL << 24 | n` (n = number of words after the header, 16 bits), `w1 = 0`.
  No separate record. The group is 8 + 8 x n bytes, contiguous: a plain model is 7 to 9 words (56 to 72 bytes, 2 or 3 cache lines), read in order (rule 3).
- (2) Key. None. Each frame the producer writes the words again, so finalMtx values, the texture handle, lights, render mode and fog are in the words. Nothing survives a frame.
  This avoids the address-key failure: plan_dl_architecture.md line 7 (R3, D1, M1 failed) and diag_dlr.md line 39 (pm_pdl_heap_gen changes every frame, hitx 0).
  The existing per-op checks stay as they are: tb_list[idx] == w1 (C1, pm_gbi.c 3512) and rm_apply list validation (2805).
  A retained record would also save producer time (dl phase, 3.3 ms console in total), but it needs invalidation on 9 or more inputs. It gives no extra op drop. Do not do it.
- Why a G_DL to a record does not work: the walker counts every op of a nested list (n_ops++ at 3473, os_op_hook at 3489). The header op runs the words in a noinline runner, which does not count them.

## 3. What the op does
Walker case (gbi_run_body switch, beside G_PM_RM 3516):
```
case G_PM_MODEL: {
    const Gfx* nx = NULL;
    unsigned n = w0 & 0xffffu;
    if (end && sp == 0 && pc + n > end) n = (unsigned)(end - pc);
    pc = (Gfx*)mg_run(pc, n, &nx);
    if (nx && sp < 12) { stack[sp++] = pc; pc = (Gfx*)nx; }
    break;
}
```
mg_run(g, n, &nx) (static noinline, placed just before gbi_run_body at 3431):
1. If dd_on(), dls_on, gp_on() or pm_ph_fine: return g. (The walker walks the words with its debug hooks. pm_ph_on alone does not stop the runner: the gate census needs it.)
2. For each word g[k], k < n, in order (prefetch g + 4 when g is 32-byte aligned):
   - op in g1_skip: next word. g2_same(op, w0, w1): next word.
   - pm_gbi_cur_op = op. gv_epoch++ when op is not G_MTX, G_POPMTX or G_DL (the rule at 3486).
   - G_MOVEMEM: gbi_movemem(w0, w1). G_MOVEWORD: gbi_moveword(w0, w1).
   - G_TEXTURE, G_SETOTHERMODE_H/L, G_SETCOMBINE, G_SETPRIMCOLOR, G_SETTIMG, G_SETTILE, G_LOADBLOCK, G_LOADTILE, G_LOADTLUT, G_SETTILESIZE: tex_op(w0, w1).
   - G_SETFOGCOLOR: fogc = w1. G_SETBLENDCOLOR: blendc = w1.
   - G_PM_TEX: if w1 && tb_list[w0 & 0x7f] == w1, tb_run(w1). Else stop at this word.
   - G_PM_RM: if rm_apply(w1), next word. Else stop at this word (rm_apply returns 0 with no effect, so the walker can run it again).
   - G_MTX: if is_host(ptr_of(w1)), load_mtx(w0, w1). Else stop at this word.
   - G_POPMTX: gbi_popmtx(w1).
   - G_DL: next = ptr_of(w1). If next is NULL, not host, or the type is G_DL_NOPUSH: stop at this word. Prefetch next.
     If dlc_on && dlc_try(next): next word. Else: *nx = next, return g + k + 1 (the walker pushes; the rest of the group runs as ordinary ops after G_ENDDL).
   - Any other op: stop at this word.
3. "Stop at this word" = return g + k. The walker runs that word and the words after it as ordinary ops, with their own fallbacks.
   This is the fallback for an invalid word. It is not a switch: the words are the original list.
4. Return g + n.
- Byte-equal reason: the same handlers run in the same order, with the same g1/g2 skips. Only walker bookkeeping (counters, census and debug hooks) is not done for the inner words.
  The header op itself does gv_epoch++ once. gv_epoch is compared for equality only (pm_gbi.c 1475, 1508), so the next triangle makes the same decision again.
- Phase census lines change meaning: mdlsum binds and the mr0 bind/dlc split no longer see the inner words. mops counts drop. Use mdlsum tot ops and opsplit ops only.

## 4. Op-count rule and gate
- Rule: the header is 1 op (n_ops++, os_op_hook, mdc_op all run once for it, class oth). Words run by mg_run are 0 ops. Words after a stop are counted as today.
- Per dlc-hit model: 5 to 7 ops become 1. A model whose shape list misses dlc keeps its nested ops, plus POPMTX after the return.
- Gate counter: mac01t line `mdlsum f899 tot ops N` (equal to opsplit ops and prod sum). Do not use `prod f.. <appendGfx_model> ops`:
  that line holds 1105, which includes appendGfx_entity_model and appendGfx_npc ops (mdlsum lists them apart). Cause not checked [I: PK_MARKS 256 overflow].
- Reachable drop: model id ops (at most 455) minus one header per model draw (86 to 96) = at most about 370. 600 is not reachable.
  The 600 figure came from plan_toad_33 when the console had 2145 ops per frame, before R1, R2B and R5B removed the light and combine ops.
- Proposed gate (orchestrator or user sets it): mdlsum tot ops down by 300 or more against the HEAD baseline run of the same scenario; model id rows r4 to r12 show ops 1 or 2; top shots byte-equal.
- Time: Azahar mac01t_fsplit gbi ms shows direction only. Estimate 1 to 2 ms console [E] (prologue of 4 to 6 ops x 86 models). The console push at the epic end measures it.
- The mac01t expected range (1175 to 1495) fails after MR1. The MR1 commit sets the new range (measured +/- 12 %) in azt_scenarios.json and test_catalog.md.

## 5. Edits
| File | Line | Change |
|---|---|---|
| include/gbi_custom.h | after 218 | `#define G_PM_MODEL 0x0C` (free: port ops are 0x09 to 0x0B) and a comment: header of n model words, run by mg_run. |
| src/model.c | before 1509 | `#ifdef TARGET_3DS Gfx* mgHdr = (*gfxPos)++; mgHdr->words.w0 = (u32)G_PM_MODEL << 24; mgHdr->words.w1 = 0; #endif` (declare mgHdr at the top of the function under TARGET_3DS). |
| src/model.c | after 2056 | `#ifdef TARGET_3DS mgHdr->words.w0 = ((u32)G_PM_MODEL << 24) \| (u32)((*gfxPos) - mgHdr - 1); #endif` |
| src/model.c | 1807 | no change. The shroud early return leaves n = 0, so the walker runs the words as today. The depth and remap tint words (1480-1491) stay before the header. |
| 3ds/src/pm_gbi.c | 3671-3685, 3686-3692, 3762-3777 | Move the G_MOVEMEM, G_POPMTX and G_MOVEWORD case bodies, unchanged, into `static noinline` gbi_movemem, gbi_popmtx, gbi_moveword before gbi_run_body (after gbi_viewport 3400, which gbi_movemem calls). The walker cases call them. This makes the hot loop smaller (rules 1, 10). |
| 3ds/src/pm_gbi.c | before 3431 | mg_run (section 3), about 45 lines. |
| 3ds/src/pm_gbi.c | beside 3516 | case G_PM_MODEL (section 3), 7 lines. |
| 3ds/tools/azt_scenarios.json, docs/handoff/test_catalog.md | mac01t | new tot ops range after the MR1 run. |
- About 80 changed lines. The row touches shared draw code (pm_gbi.c, src/model.c): byte-equal check on the scenes in section 6.
- hw_guidelines: the runner is noinline and calls the existing handlers (no inlined handler, rule 10). Sequential reads with prefetch (rule 3). No new division or double.
  The moved light handlers keep their existing `* 8 / 24` (not new; out of scope).

## 6. Test (rule 5)
- Build: `/c/Python314/python 3ds/tools/gen_map_restore.py`, `PM_LANE=mr1 PM_ROW=MR1 sh 3ds/build.sh`, restore 3ds/src/map_restore_table.c from the user copy.
- Baseline: lane conlb1b2 holds HEAD code (b4097cd16 changed docs only). Run each scenario once on it for the shots and the mac01t tot ops baseline.
- Order: mac01t first (gate quantity and 2 top shots). Continue only when tot ops fell by the gate amount and the shots are byte-equal. Then the 6 maps: yard, forest_mim, sewer, ruins, sun_sbk, volcano (1 top shot each, byte-equal).
  Note: azt_scenarios.json has two entries named forest_mim. Use the one the catalog marks.
- Command: `PM_ROW=MR1 powershell -File 3ds/tools/azt.ps1 <scenario> -Row MR1 -Build mr1 -Try`. Compare: `python 3ds/tools/shot_cmp.py <baseline dir> <mr1 dir>` (copy each shot folder first).
- Starts: 7 baseline + 7 MR1 = 14. That is above the budget of 8. The orchestrator resets the budget at the cap. The row is INCOMPLETE until all 14 run.
- Stop: a changed top shot, or tot ops not down by the gate amount: revert all MR1 files.
- Commit: model.c, gbi_custom.h, pm_gbi.c, azt_scenarios.json, test_catalog.md. Never map_restore_table.c.

## 7. Option outside MR1 (not planned)
- The same header in appendGfx_entity_model (363 ops) and appendGfx_npc (162 ops) could give more drop, but those rows hold vtx/tri words (draws 30 and 27), which stop the runner. A census per word type is needed before any row.
