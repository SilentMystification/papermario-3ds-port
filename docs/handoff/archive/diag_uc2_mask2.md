# diag_uc2_mask2: first unmasked differing word on yard after the 0x227 mask

Investigation only. Base HEAD 950f4680a. No source edit, no Azahar start, no git checkout. Read: CLAUDE.md rule 3, agent_uc2.md, diag_uc2_mismatch.md, uc2_mask_B.patch (scratchpad), 3ds/src/pm_gpu.c 1030-1066 and 2255-2340, 3ds/src/pm_gbi.c 3370-3420, plan_uniform_cut.md.
Tags: [C] CONFIRMED (code or logged counter). [I] INFERRED.

## 1. Counters (CONFIRMED, from agent_uc2.md)
- yard, no mask (UC2F commit A): f348 hit 2375, bad 4800, miss 0.
- yard, 0x227 mask (run B): f349 hit 2375, bad 4222, miss 0.
- Drop: 578 words over 2375 compares, about 0.24 per compare. Remaining: about 1.78 bad words per compare.
- mac01t_rec f614 with mask: hit 5000, bad 9700, miss 300.

## 2. Result
- The first unmasked differing word is NOT identified by the code or the counters. [C]
- The walk and the compare do not name the word. The rec line keeps sums only (pm_gpu.c 3440, pm_gbi.c rec_report). [C]
- No dump of one record and one frame group exists in the logs. [C]

## 3. What the compare masks (CONFIRMED)
- Uniform data: each 0x2C1 vec4 (split: off, off+2, off+3, off+4; else off..off+3). pm_gpu.c 2300 to 2302, 2327 to 2331.
- Index buffer address (0x227 param, bit 31 clear): pm_gpu.c 2298 is NOT in HEAD. It exists only in uc2_mask_B.patch. The mask is reverted. [C]
- Not masked, compared as plain words: 0x2C0 config word, 0x2C1 header words, the 0x280 to 0x2DF program registers (pk), 0x25E, 0x25F, 0x228 count, 0x22A, 0x245, 0x22F, the cull word 0x40, scissor, blend, texture registers. [C from pm_gpu.c 2289 to 2296 and 1040 to 1066]

## 4. Candidates for the remaining words (ranked)
1. Program registers 0x280 to 0x2DF (pk words, not 0x2C0 and 0x2C1). Compared as plain words. Per frame change not seen in code. [I]
2. Cull word 0x40 (sh_cull, g_cullm, pm_gpu.c 1040). Changes only on a gv_wind flip (plan_uniform_cut u6). A flip is rare. About 1.78 bad per compare on yard is too many for a rare flip. Unlikely. [I]
3. Count 0x228 (gni). Key-pure per model batch (diag_uc2_mismatch section 4). Unlikely. [I]
- Index address 0x227 and the uniform slots are not candidates. Both are masked or patched. [C]

## 5. Single change (diagnostic, log only)
- In rec_check (pm_gbi.c 3401), on the first compare with a differing word: log the index j, a[j], b[j], and the register and header of the command that holds j (from pm_gpu_rec_walk over a). Log once per build.
- Expected: one line names the register and offset. The next change masks or patches that word.
- Picture unchanged (the log does not touch the draw). [C]
- Cost: one Azahar start on yard_rec, one justification line in 3ds/logs/perf/status_UC2.txt. Not run here.

## 6. Open items
- The word that gives bad 4222 on yard. Needs the log in section 5.
- mac01t_rec miss 300 (pal count or light shape per draw). Separate from bad. [I]
