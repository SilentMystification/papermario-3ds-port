# ws_GRABFL (Haiku): the grab split flushes the ring words of part 1 (console-only stale vertices)
Start after Merge MENUFL (GRABSEQ and MENUFL edit pm_gpu.c). Worktree build3ds/wt/GRABFL, branch GRABFL. PM_ROW=GRABFL PM_LANE=GRABFL.
Objective (FACT, oracle code read, main 2477863196). Every command list split in pm_gpu.c flushes the vertex ring and the index ring out of the
CPU data cache first (split_frame :300, kick_split :782, sr_split :2456, bind_main through sr_split). The grab split does not (grab_impl :2787-2788).
grab_queue_copy can run the GX queue at once: pm_gx_room at 24 entries (:2627) or the copy_guard drain (:2628, :361). Then the GPU draws part 1 from
stale memory. Azahar has no data cache. The new flush keeps gring_fl and gidx_fl, so every draw keeps its words (byte-equal). Counters: none of C1..C12.
Files: 3ds/src/pm_gpu.c, 3ds/tests/host.d/grabfl.sh (new). Edit no other file. The blocks are in ws_GRABFL_code.md.
## Edit 1 (pm_gpu.c :2759-2760). Before:
`}` (end of grab_raw_quad) / `static int grab_impl(const void* cfb, int y0, int y1, int prev_frame) {`
After: block A between these two lines (the new function grab_ring_flush).
## Edit 2 (pm_gpu.c grab_impl :2787-2788). Before:
`        pm_tex_flush_pend_only();` / `        sh_clear(); ndunit_scan(); rd_sync(); C3D_FrameSplit(GX_CMDLIST_FLUSH); sc_spl++;`
After: block B (one line, `        grab_ring_flush(); /* GRABFL: ... */`) between these two lines.
## Edit 3: block C as 3ds/tests/host.d/grabfl.sh (LF endings).
FACT (oracle dry run of the awk in block C): main prints `grabfl FAIL grab_impl`. Main with Edit 2 prints `grabfl ok splits 5`. The seed prints `grabfl FAIL grab_impl`.
## Loop (1 build, at most 4 Azahar starts; base runs not charged)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/grabfl_host.txt 2>&1`. Pass: `ND HOST TESTS PASS` and a line `grabfl ok splits N`.
   Commit `GRABFL 1: the grab split flushes the ring words of part 1 (console-only stale vertices)`.
2. One build. memcheck margin above 0.
3. Gates, first fxr_drec, then spot_mac01_drec: `python 3ds/tools/bench.py base <bench>`,
   `python 3ds/tools/bench.py run <bench> --row GRABFL --why "GRABFL gate"`,
   `python 3ds/tools/bench.py cmp <bench> --row GRABFL --commit $(git merge-base HEAD main)`. Pass: 1/1 byte-equal (one differing bit fails).
   A cold first fxr_drec cmp: one warm rerun (bench.py:2023 rule).
## Tests that decide
grabfl.sh (seed and real file) and the two byte gates. FACT: no Azahar bench runs the queue mid-frame at a grab. The console proof is the user's check.
## Fix scope
A failure in grabfl.sh: fix only grabfl.sh or Edit 2. A `grabfl FAIL <fn>` for a function other than grab_impl: do not edit that function.
A byte gate that differs: do not change the code. Write the cmp output to build3ds/out/ and send the path to the oracle (SendMessage).
## Stop and hand-back
Stop: host PASS and both gates byte-equal. Final commit `GRABFL 2: gate` with the values. It deletes ws_GRABFL.md and ws_GRABFL_code.md.
Hand-back: build3ds/out/handback_GRABFL.txt (line 1 commit hash, then the 2 gate values, memcheck margin, counters: none).
Any other failure after 2 tries: write the output to build3ds/out/ and send the path to the oracle (SendMessage). Then stop.
