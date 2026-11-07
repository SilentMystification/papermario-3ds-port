# Audit: warm restarts and picture gates, 2026-11-06 12:00 to the AZT9c merge

Read-only audit. No code, no Azahar runs. Written 2026-11-07.

## Scope and method

- Window: run ts from 2026-11-06 12:00:00 to 23:37:15. The AZT9c commit (e6345d991, 23:36:37) first reaches nds1 in merge 5ec146f06 (pshift, 23:37:15).
- Harness fixes: AZT7 000dda27e (22:38:09, shot output cleared per start), AZT8 a16293b13 (22:47:46, warm restart must boot the lane build), AZT9c e6345d991 (23:36:37, boot check reads the newest boot).
- Data: `3ds/logs/perf/runs.jsonl` has two versions. HEAD has 876 lines, the working copy 848. The audit uses the union of both (949 distinct lines). In-window azahar records: 447. Ids below: `H<n>` = line n of HEAD, `W<n>` = line n of the working copy (records not in HEAD).
- Checks run on every record:
  - C1 build_no null.
  - C2 build_no shared by two commits in the window.
  - C3 warm restart (warm = 1) before AZT8.
  - C4 stale-ROM repeat: a warm run before AZT8 whose commit equals the previous run on the same slot, and whose lane gets a different commit within 15 min after it. Heuristic only. The DB does not store the intended commit, so a repeat can be legitimate.
  - C5 picture pass before AZT7. The shot file is overwritten per bench, so the shot identity cannot be checked after the fact.
  - C6 byte-equal claims in commit messages, checked against a passing pair of records (same build, same lane, both cold or both warm after AZT8).
- Verdict classes: OK; SUSPECT (hard) = a claim has no passing supporting pair, or a compared record has null build or a conflicting build; SUSPECT (soft) = C3, C4 or C5 applies to a record the claim rests on.
- Limits: `dirty` is True on 393 of 447 window records, so the commit field does not prove the tree tested. Azahar logs and shot folders are overwritten per bench, so the boot line of an old run cannot be re-read. The lane 3dsx files on disk show only the last build of each lane.

## Data integrity (not gates, but they change the evidence)

- The working runs.jsonl lacks 101 HEAD lines (21:29 to 23:47 mostly: LOGFIX bisect, AZT9 wcen_pause, PSHIFT, AZT10). It has 73 new lines not in HEAD. The append-only rule is broken in the working copy. Do not commit the working file as is: restore from HEAD and append.
- run_id is not unique: 26 ids on 63 records. Commit messages cite run ids (6c5068036 cites 1004 to 1189, 1226, 1263). Those ids name other records (1226 is an AZT6 pause_spin_grab run, 1263 an MDL run). Cite by line or ts.
- Build number is V0 + (commit count - C0), so two branches can produce the same number. Five window builds map to two commits: 2401, 2418, 2523, 2533, 2536, 2546 (2546 also on MDL d4df7f240). Build number alone does not identify a binary.
- 19 window azahar records have build_no null (C1), e.g. H412, H418, H419, H420, H431, H432, H445, H446, H467, H470, H472, H473, H475, H490, H541, H684, H688, H697, H720.
- 14 window azahar records have a log_path with no lane directory (e.g. build3ds/slot1, build3ds/last_log.txt). They are main-folder runs. They are not in the gate groups below.
- Lane 3dsx on disk: lane-log5 holds 2470 (mtime 19:44), but records name lane-log5 for builds 2491 and 2492 (20:56 to 21:13). Those builds ran from another worktree dir. Check the log_path of those records before using them as lane evidence.

## Harness gates

- AZT7 (pictures): 222 window picture passes before 22:38:09. Each could show a stale shot. This is the only check the records allow. Any gate below that relies on such a pass is marked soft.
- AZT8 (warm, boot build check): 113 window warm restarts before 22:47:46 (C3). Stale ROM is possible for each. The record's build and commit come from the boot line of that run, so a stale ROM would show as the old commit (C4).
- Window between AZT8 and AZT9c (22:47:46 to 23:36:37): 60 warm runs. The AZT8 check read the first boot after the log truncation; AZT9c fixed that. Those warm runs are soft-suspect. For PSHIFT the records match the newest lane build at their time, as far as the commit log shows; no stale ROM is visible.
- AZT9 negative control: lane-nds1 (build 2011) runs H777, H779, H781, H783, H785, H787, H789, H791, H793 all fail, warm and cold alike. The fail is not caused by warm restarts. Not a gate for any merge.
- AZT10 and AZT10b (23:40 and later, after AZT9c): H837 to H849 record warm restarts that match the lane build. These are the post-fix evidence for the warm path.

## Gates

Gate groups are the rows named in the request. Ids are the record lines.

### RECT-1b (merge 56ee96c93, 20:52). Verdict: SUSPECT (soft), with one unsupported claim.
- 62 records (row RECT). 48 picture passes before AZT7 (C5), e.g. H440, H443, H447, H448, H721, H722, H733.
- 21 warm runs before AZT8 (C3): H473, H475, H490, H668, H690, H693, H712, H716, H717, H719, H721, H722, H724, H726 to H733.
- Stale-repeat (C4): H717, H719, H721, H722 (commit 28e05d68c, lane-rect, next run on slot 1 is 08ff8dbba at 20:50). H727 (28e05d68c, slot 2, next 56ee96c93 at 21:02).
- Null build (C1): H467, H470, H473, H475, H490 (rect_regdump runs). H464 and H466 share build 2401 with POND 0e2c6adc1 (C2).
- Claim "mac_01 byte-equal" (merge message): no base commit is recorded in the DB and no passing pair exists. Hard for the claim.
- Rerun: rect_post_mac01 cold on 56ee96c93, with its base recorded.

### OFF (merge e5e7d5de5, 22:06). Verdict: SUSPECT (soft).
- 16 records (rows OFF and AZT6). 13 picture passes before AZT7: H610, H614, H618, H619, H622 to H625, H627, H628, H631, H632, H634.
- Warm before AZT8: H617, H618, H619, H632, W779. H617 and H618 are C4 (70cfc5220 repeated on slot 1, lane rebuilt to fd11da7e5 at 21:29). H619 is a new build (2501) on a warm restart, so it is not stale.
- C2: H624 (spot_mac01_pic, build 2523, commit ce89bb040, cold 21:58) shares build 2523 with MDL 204d529fb. The compare is by build number only, so no OFF claim uses it.
- Rerun (soft): spot_mac01_pic cold on ce89bb040.

### PM5 (program 5 native, PM5-2 and PM5-3). Verdict: SUSPECT (soft).
- 25 records. 13 picture passes before AZT7 (C5). 17 warm before AZT8: H736 to H738, H746 to H752, H754, H756, H759, H768 to H770, W776.
- The pm5 on/off pairs are on the same build 2512 (commit 109169252), same lane-pm5 slot 1 and slot 2: H748 (super_text_pic) with H749 (super_text_pm5off_pic), H750 (rect_post_mac01) with H751 (rect_post_pm5off_mac01). Both members of each pair are warm before AZT8 and both pass. Identity is good (same build). The warm starts are soft (C3 and C4 at H748 to H751).
- Claims "pixel-identical pm5 on and off" and "mac_01 frames 200 to 202 byte-equal" rest on these pairs. Soft.
- Rerun: rect_post_mac01 with pm5 on and pm5off, both cold, on the merged tip.

### CORE-2/3 (merge 2ca8df902, 23:25). Verdict: SUSPECT (hard).
- Only 2 records in the DB: H709 (spot_mac01_pic, build 2479, commit dc7f1c438, cold, 20:40) and H714 (spot_mac01_pic, build 2480, commit 1f5e0db04, warm before AZT8, 20:45). Both are CORE-2 intermediate commits, not the CORE-3 tip.
- Claim "spot_mac01_pic and spot_mac00_pic frames 200 to 205 byte-equal to 5b2ee3c48" (CORE-3, commit 138a83ef6, merge 2ca8df902): base 5b2ee3c48 and every CORE-2b base (35c7d89c6, 0288713da, 1ee0924dd, 28df033e1, 6475a87d6, 9b5e3d20b) have zero records. spot_mac00_pic has no CORE record at all. The claim has no supporting record. Hard.
- Rerun: spot_mac01_pic and spot_mac00_pic frames 200 to 205, cold, CORE-3 tip vs 5b2ee3c48, both recorded.

### WK-1 (merge 5e5456e11, 23:46) and TX-1 (merge e278e6837, 00:10). Verdict: SUSPECT (hard, no record).
- No window records with row WK or TX. The claims ("spot_mac01_pic byte-equal" for WK-1 and "3 of 3 byte-equal to the TX-1 base" for TX-1) have no base commit and no compare pair in the DB.
- Both merges come after the AZT9c merge, so the stale-ROM bug does not apply. The gap is evidence, not harness.
- Rerun: spot_mac01_pic cold on the WK-1 and TX-1 tips, each with its base, recorded.

### GM-1 (merge ffe23e735, 00:20). Verdict: OUT OF WINDOW (not audited here).
- GM runs start at 00:04 (commit 43a478f53, build 2570, warm, after AZT9c). Post-AZT9c, so the boot check applied. Claim base 43a478f53 is in the DB.

### BTRANS (merge 6a4b32b68, 23:59). Verdict: OUT OF WINDOW.
- No BTRANS rows in the window. The merge follows AZT9c. No base or compare pair in the DB for the claimed btrans benches.

### MDL-1/1b (row MDL, commits 1b9b9d119 to f8d571baf). Verdict: SUSPECT (hard).
- 60 records (row MDL). 19 warm before AZT8 (C3): H761, H765, H766, H774, H775, W777, W778, W783, W784, W786 to W795. Among them C4 soft: H761, H765, H766, H774, H775, W778, W784, W786, W787, W789, W793, W794, W795.
- Warm in the AZT8 to AZT9c window (soft): 21 MDL records, e.g. W809, W811, W812, W813, W816.
- C2 (number shared by two commits): 2523 (204d529fb vs OFF ce89bb040), 2533 (437c502dd vs PM5 6f46dff75 and AZT9 1ff6780a7), 2536 (39e4378d6 vs 63529db43), 2546 (d4df7f240 vs PSHIFT eaee325ad). Picture comparisons by build number are invalid for these.
- Claim "mac_01 f200 to 202 byte-equal to r13off on the same binary": supported. W814 (spot_mac01_pic, build 2568, commit ad8f5a961, cold, pass, 23:20) and W815 (spot_mac01_r13off_pic, same build and commit, cold, pass, 23:21). OK.
- Claim "battle_start f182 (132 shots) byte-equal to r13off": no passing pair. W812 (battle_start_pic) and W813 (battle_start_r13off_pic) are both fail:1, both warm inside the AZT8 to AZT9c window. Hard.
- Claim "iwa_10 3 shots byte-equal to r13off": no passing pair. W816 (mdl_iwa10_modebad_pic, warm, fail:1) and W817 (mdl_iwa10_r13off_pic, cold, fail:1). Hard.
- Rerun: battle_start f182 pair and iwa_10 pair, cold, on ad8f5a961 or the MDL-1b tip, with a passing run.

### PSHIFT (merge 5ec146f06, 23:37). Verdict: OK on records; citation wrong.
- 9 records (H828 to H836). Two cold fails (H828, H829), then passes H830 to H836. Warm ones (H830 to H836) are inside the AZT8 to AZT9c window; each record's commit equals the newest lane build at its time (eaee325ad 23:12 to 23:17, e67d13582 23:25, a3f1cb9a7 23:26:45 onward). No stale ROM visible.
- Byte-equal pair: H834 (spot_mac01_pic, commit a3f1cb9a7, build 2549, warm, pass, 23:29) vs H835 (spot_mac01_pic, base e35813e85, build 2548, build-at, pass, 23:34). Different builds on purpose (base vs fix). OK.
- Citation: 6c5068036 cites run ids 1004 to 1189, 1226 and 1263 for these runs. Those ids name other records. Fix the commit reference (no rewrite: cite the lines in the next run file).

### LOG5 (LOG rows, logbench pairs, LOGFIX bisect). Verdict: SUSPECT (soft).
- LOG rows: 28 records; 11 warm before AZT8 (C3): H584, H588, H591, H592, H595, H604, H608, H609, H741, H742, H744. H595 is C4 (log5_menu, build 2491 d7a3181ed, next 4227e2904 at 21:12).
- LOGFIX bisect (H853 to H872): 10 warm before AZT8, all fail:1 (battle_start_f182, a known run-to-run timing box per b5b95e04c). Not a stale-ROM signal.
- 3 picture passes before AZT7: H597, H599, H600.
- Lane note: lane-log5 3dsx on disk is 2470 (see data integrity). Lane identity of the 2491 and 2492 runs needs a check.
- Rerun: log5_menu cold on d7a3181ed, and one logbench pair cold on the same build.

### AZT9c (the harness gate itself). Verdict: SUSPECT (soft), the post-fix evidence is in AZT10.
- The AZT9 wcen_pause runs (H776 to H827 and W796 to W826) include 32 warm runs in the AZT8 to AZT9c window, checked by a boot check that AZT9c later fixed.
- Post-fix: H837 to H849 (AZT10, AZT10b, after 23:37) are the evidence. They are outside the window.

## Gates to rerun

1. CORE-3 (merge 2ca8df902): spot_mac01_pic and spot_mac00_pic frames 200 to 205, cold, against 5b2ee3c48. Hard.
2. WK-1 (5e5456e11) and TX-1 (e278e6837): spot_mac01_pic cold with base. Hard (no record).
3. MDL-1b battle_start f182 and iwa_10 pairs: cold, passing, same build. Hard.
4. RECT-1b (56ee96c93): rect_post_mac01 cold, with base. Soft, and the mac_01 claim is hard.
5. PM5 (109169252 pairs): rect_post_mac01 pm5 on and off, cold. Soft.
6. LOG5 (d7a3181ed): log5_menu cold. Soft.
7. OFF (ce89bb040): spot_mac01_pic cold. Soft.
8. Pictures before AZT7 (22:38:09) that a later gate relies on: rerun any of them as a cold start after AZT9c.

Not on the list: MDL mac_01 pair (W814, W815), PSHIFT pair (H834, H835), the AZT9 and AZT10 harness runs.

## Summary of verdicts

| Gate | Verdict | Hard reason | Soft reason |
|---|---|---|---|
| RECT-1b | SUSPECT | mac_01 byte-equal claim has no pair | 21 warm before AZT8, 48 pictures before AZT7 |
| OFF | SUSPECT | none | 5 warm before AZT8, 13 pictures before AZT7 |
| PM5 | SUSPECT | none | 17 warm before AZT8 (on/off pairs warm) |
| CORE-2/3 | SUSPECT | byte-equal base and tip have no record | 1 warm before AZT8 |
| WK-1 | SUSPECT | no record | |
| TX-1 | SUSPECT | no record | |
| GM-1 | OUT OF WINDOW | | |
| BTRANS | OUT OF WINDOW | | |
| MDL-1/1b | SUSPECT | battle_start and iwa_10 claims have no passing pair | 19 warm before AZT8, build collisions |
| PSHIFT | OK | | citation of run ids wrong |
| LOG5 | SUSPECT | | 11 warm before AZT8 |
| AZT9c | SUSPECT | | 32 warm in AZT8 to AZT9c window |
