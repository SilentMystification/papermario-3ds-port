# ws_GD9A2 (Haiku): status bar census 2: a steady window, duplicate walks, the nd_ds entry hash and the first differing unit (GAMEDRAW row 9)
Oracle NATIVE18 (plan_gamedraw.md 1.6). Worktree build3ds/wt/GD9A2, branch GD9A2, PM_ROW=GD9A2 PM_LANE=GD9A2. Start: main with GD9A merged (8785b78c74).
Why Haiku: counters, a frame span and log lines in 3ds/src/nd_ublk.c, all given below. No draw change. Counter: none moves; the lines decide GD9B.
FACT (GD9A Azahar, window f390): `ublk n 339 key 73 st 59 w 54 bad 5 spl 0 words 399 draws 8`. That window holds the scene start: the bar is hidden,
then slides in after 240 idle steps (inventory.c:737-746), so most keys change by design. n 339 for 300 steps: some streams are walked twice (OPEN: the
key frame walk, pm_nusys.c:158-169). bad 5: key and core 0 state equal, words differ: the key lacks an input. Candidate (code read): the snaps the
bracket writes (nd_snap_pso, nd_snap.c:124-131) copy all of nd_ds, the core 1 state before the bracket, which the key does not hold.
## Edits (3ds/src/nd_ublk.c unless named)
1. Span: launch word `ublkf=<f0>:<f1>` (pm_debug_value, read once): core 0 counts only blocks walked in frames f0 to f1 (pm_walk_frame); without it, as now.
2. Duplicates: keep the frame of the last counted block per site; a second block of the same frame counts `dup` and is not compared.
3. nd_ds entry hash: in nd_ublk_begin, before recording, `r[2] = FNV over the bytes of nd_ds` (extern NdDs nd_ds, nd_ds.h). Core 0 keeps it per site.
   New counts: ds (key and ds equal), bad2 (key, ds and core 0 state equal, words differ).
4. First differing unit (core 1): keep the per-unit hash list of the last bracket (up to 512 entries, FNV of each nd_ublk_unit and nd_ublk_napi
   contribution) and the unit words. Inside the span, when the key differs from the last bracket and both have the same unit count, log the first
   differing index: `ublk f%u kdiff u%u w0 %08x w1 %08x was %08x %08x` (at most 8 lines per run). Different unit counts: `ublk f%u kn %u was %u`.
5. ds detail (core 1): keep a copy of the last nd_ds (heap, sizeof nd_ds). Inside the span, when the key is equal and the ds hash differs, log the first
   differing byte offset: `ublk f%u dsdiff off %u` (at most 8 lines). Name the field from the offset with offsetof in the hand-back (nd_ds.h).
6. The prof line gets the new counts: `prof f%u w%u ublk n %u key %u st %u w %u bad %u ds %u bad2 %u dup %u spl %u words %u draws %u c0 %u.%02u`.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=GD9A2 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `bench.py base hudfb_drec --commit $(git merge-base HEAD main)`; run and cmp `--row GD9A2`: byte-equal (drec and gst).
3. `python 3ds/tools/capture_check.py --row GD9A2 --proof "ublk n" --profile prof fps60 uishow=bar ublkf=300:390 warp=mac_01:0 pos=0:0:200 profdisk=1`.
   uishow=bar opens the bar at frame 200 (pm_warp.c:2038); it stands from about f215 until showTimer (210) runs out. Never add phase=on.
   Copy the ublk line and every kdiff, kn and dsdiff line into the hand-back.
Fix rights: compile and link errors. A DIFF, a base fault, no ublk line: stop, send the output path to oracle NATIVE18.
Stop: commit `GD9A2 1: gate. <host> <c0gate> <cmp line> <ublk line>` deletes this page. Hand-back: one SubagentHandback: commit hash and all ublk lines.
