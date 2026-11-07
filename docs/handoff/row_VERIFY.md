# Row VERIFY: recheck the suspect gates on today's nds1 (cold, post-AZT10b harness)

## Why
docs/handoff/audit_warm_runs.md: gates run before AZT7/AZT8/AZT9c may have used a stale ROM or a stale shot. Hard suspects: CORE-2/3, WK-1, TX-1 (byte-equal claims with no base pair in the DB), MDL-1/1b (battle_start and iwa_10 pairs failed), RECT-1b (no base pair). The merges are cumulative, so one check of today's nds1 covers them: pure moves must keep the picture equal to a pre-move base; native paths must equal their walker reference switch on the same binary.

## Checks (all cold starts: the harness boot check proves the build; record build_no and commit)
1. Pure moves (CORE-2/3, WK-1, TX-1, GM-1): spot_mac01_pic and spot_mac00_pic frames 200-205 on nds1 HEAD vs bench build-at 5b2ee3c48 (before CORE-3, after PM5). Expected byte-equal (no change since then alters these frames; name any commit that should, by code).
2. Native vs walker on one binary (nds1 HEAD): battle_start_f182 vs the same build with r13off and with pm5off; iwa_10 3 shots vs r13off; rect_post_mac01 vs rect_nr_plain_mac01 (norectraw). Expected byte-equal.
3. Census: wcen_mac01, wcen_pause, wcen_battle_pd on nds1 HEAD: wd 0.
Write a table into docs/handoff/audit_warm_runs.md (section "## Recheck on nds1 <hash>", replace any older recheck section): check, runs (line numbers), result. For any failure: name the first differing frame and bisect by merge commits with build-at (cold), code first for the cause, and stop there (report; no fix in this row).

## Rules
No code changes. Own worktree not needed (bench build-at and lanes); run from the main folder. Commit only the doc (pathspec commit), no attribution lines. Cold starts: close nothing by hand; if the harness gives a warm start, the boot check must show the right build id, else rerun. Keep Azahar open. Delete this file in the commit with the table.
