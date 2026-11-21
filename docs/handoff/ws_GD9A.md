# ws_GD9A (Haiku): status bar block census: is the bar's core 0 command block the same each step (key, state, words)? (GAMEDRAW row 9, step A)
Oracle NATIVE18 (plan_gamedraw.md 1.6). Worktree build3ds/wt/GD9A, branch GD9A, PM_ROW=GD9A PM_LANE=GD9A. Start: main 00f4cdcb57 or later; grep the quoted text.
Why Haiku: a bracket, 3 one-line hooks, a hash and counters, all given below. No draw change. Counter: none moves; the line decides GD9B (the replay).
FACT (NATIVE20, live_play2_170.txt): the shown bar adds on core 0 gbi +4.15 ms, about 26 draws per frame (95 -> 121), gx +2.14 ms; ft +1.23 ms.
## Edits (CRLF files: use the Edit tool)
1. src/state_world.c:22. Before: `    draw_status_ui();`  After: `    nd_ublk_begin(1); draw_status_ui(); nd_ublk_end(); /* GD9A */` (declare both in 3ds/include/nd_rec.h).
2. New 3ds/src/nd_ublk.c. Core 1: begin records `nd_rec_napi(nd_ublk_run, rec, 2)` with rec words {1 (begin), site, 0, 0}, then sets nd_ub_on = pm_pf_on and
   ub_key = 2166136261u. end clears nd_ub_on, then records {2 (end), site, ub_key, ub_n} the same way. The key (FNV-1a over 32-bit words):
       void nd_ublk_unit(uint32_t w0, uint32_t w1) { /* GD9A: one unit; the data behind a matrix or vertex pointer, not the pointer (it alternates per context) */
           const unsigned id = w0 >> 24; ub_w(w0); ub_n++;
           if (id == ND_FS_MTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 64u);
           else if (id == ND_FS_VTX && is_host(w1)) ub_bytes((const void*)(uintptr_t)w1, 16u * ((w0 >> 12) & 0xffu));
           else ub_w(w1); }
   nd_ublk_napi(run, rec, units): ub_w(run); when units == 1, the pair through nd_ublk_unit (an nd_xf record is one MTX or POPMTX unit,
   nd_xf.c:31; nd_xf_run is static); else ub_bytes(rec, 8 x units).
   (is_host: nd_walk.c:421, a pure range test.)
3. 3ds/src/nd_rec.c, one line at the entry of each (after the opening brace): nd_rec_nr (:604) `if (__builtin_expect(nd_ub_on, 0)) nd_ublk_unit(w0, w1);`;
   nd_rec_napi (:449) and nd_rec_napi_s (:460) `if (__builtin_expect(nd_ub_on, 0)) nd_ublk_napi(run, rec, units);`; nd_rec_alt (:635) the same with
   (run, rec, units) (its au units pass nd_rec_nr and hash there). Define `int nd_ub_on;` in nd_rec.c and weak no-op nd_ublk_unit and nd_ublk_napi
   (as nd_geo.c:36), so the host units that link nd_rec.c still link.
4. Core 0, nd_ublk_run(rec) in nd_ublk.c. begin: when pm_pf_on, keep t0 = svcGetSystemTick, coff (GPUCMD_GetBuffer), s0 = pm_gpu_cmd_split_n(),
   d0 = pm_gpu_draws_frame(), sh = pm_gpu_state_hash(). end: ticks, draws and words (coff delta) of the block; when the split count moved, count spl and
   stop. wh = pm_gpu_word_hash(coff0, coff1). Per site keep the last (key, sh, wh). Count: n, key (key equal last), st (key and sh equal), w (key, sh, wh equal),
   bad (key and sh equal, wh not). Sum ticks, words, draws.
5. 3ds/src/pm_gpu.c accessors: pm_gpu_draws_frame (draws_frame :103); pm_gpu_state_hash = FNV over the bytes of ndn_sh (:170) and the word at
   __C3D_Context + 32; pm_gpu_word_hash(from, to) = FNV over the command words of the current buffer, PICA format (nd_emit.c:8-15), the values of
   registers with gst_skip(reg) (:615) taken as 0.
6. pm_phase.c window print: `prof f%u w%u ublk n %u key %u st %u w %u bad %u spl %u words %u draws %u c0 %u.%02u` (counts per window; words and draws
   per block; c0 = ms per block x100). nd_ublk_take(...) copies and zeroes the counters (core 0 writes them; the print runs on core 0).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=GD9A sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base hudfb_drec --commit $(git merge-base HEAD main)`; run and cmp `--row GD9A`: byte-equal (drec and gst).
3. `python 3ds/tools/capture_check.py --row GD9A --proof "ublk n" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`: CAPTURE CHECK OK.
   Never add phase=on. Note the ublk line (Azahar: counts are valid, c0 is not).
Fix rights: compile, link and placement errors. A DIFF (the bracket records split a state run: send the first DIFF frame), a base fault: stop, send the
output path to oracle NATIVE18.
Stop: commit `GD9A 1: gate. <host> <c0gate> <cmp line> <ublk line>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
