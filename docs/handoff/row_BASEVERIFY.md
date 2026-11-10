# Row BASEVERIFY: a stored base must be stable (Haiku)

## Facts (oracle batch 6): HC3 control fault: the HC3 build is correct, the stored base is bad

**The HC3 build matches the other bases.** md5 per shot:
- HC3 -O2 run (build 3796) = basecache a7212c47e = basecache 5327ea59f, for all 6 shots (ccee85c5, 31cc6e57, 57d20b29, ...).
- basecache 2e4de46da (fab1a23f, ...) matches neither.
- 2e4de46da to a7212c47e changes only one line in nd_op.c (the HC4e1 typed passthrough), so that line does not change the picture.
- The -Os run gave identical per-shot counts, so -Os also renders the same picture.
- So none of the suggested causes apply: no stale ELF (the boot line shows build 3796-HC3_150337, git a749ac004), no stale lane volume, no other lane's shots folder (they are fresh, 15:05), and no cmp path fault.

**How the bad base was stored.** HC3 re-stored it at 14:34:27 (meta row HC3, build 3795) after 2e4de46da had dropped out of the store (BASE_KEEP=5). The STEREOFX1 6/6 at about 13:50 was against the older, good copy.
- The run was a cold start after two failed warm restarts in slot1 (pool.log lines 8948-8957: PAUSEFADE, then HC3 "warm fail emu 0 B", kill, cold pid 9260).
- It was stored before the base lock existed (1be3c4459, 14:53).
- Its game logic equals the good runs (same audiohash and seamfix lines), but the pixels differ by ±1-12 over textured areas.
- I could not name the exact cause by code read. The cold start alone does not cause it: the a7212c47e and 5327ea59f bases were also cold and are good.

**The tool gap.** base_run stores whatever one run gives, and nothing checks the result.

**Tool fix (bench.py, Haiku):**
1. In `base_run`, run the base scenario twice in the same azt list and store only when every shot is byte-equal between the two runs. Otherwise print "base X: two runs differ, not stored" and return 1. This costs 1 extra start per merge-base, once.
2. Add `bench.py base <bench> --drop [--commit C]`, which removes `basecache/<C>/<bench>` and its index.json entry.
3. In `cmd_cmp`, on any DIFF, also hash each row shot against every stored base of the same bench and print "row shots equal stored base <short>". That would have shown this fault at once.

**Valid gate for HC3:**
1. `bench.py base spot_mac01_det --drop --commit 2e4de46da`
2. `bench.py base spot_mac01_det` (rebuilds 2e4de46da, with the two-run check)
3. Restore the -Os line on a749ac004 and run `bench.py run spot_mac01_det --row HC3`
4. `bench.py cmp spot_mac01_det --row HC3` should give 6/6.

Without the tool row, `bench.py cmp spot_mac01_det --row HC3 --commit a7212c47e` on a fresh -Os run is a valid proxy, because the -O2 control already equals that base byte for byte.

**Side finding:** `run_id` values repeat across worktree runs.jsonl files (191535 and 193617 each appear for two rows).
- Also fix: run_id values repeat across worktree runs.jsonl files (191535 and 193617 each appear for two rows).
