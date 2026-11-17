# ws_OVLIMG (Haiku): D_80156910 becomes the 160 x 120 RGBA16 image; D_8014EA48 moves; screen_overlays.c:264 to ND_SELF (C1 -1, C2 -1, C5 -40, C6 -1)
Designer: oracle DESIGN2 (SendMessage). Worktree build3ds/wt/OVLIMG, branch OVLIMG from main, PM_ROW=OVLIMG PM_LANE=OVLIMG.
Main at release: 3ad3164cdd. Start: now, parallel with ws_FLTGT (no shared file). Model: Haiku. The patch is complete (no design step).
Edit only the 4 files of docs/handoff/ws_OVLIMG.patch: src/screen_overlays.c, 3ds/tools/gfxin/src/screen_overlays.c,
new 3ds/tools/gset.d/OVLIMG.txt, new 3ds/tests/host.d/ovlimg.sh.
## Objective
- FACT (screen_overlays.c:21): `BSS s32 D_80156910;` is 4 bytes. FACT (:71-:91): 11 gsDPLoadTextureTile(&D_80156910, RGBA, 16b,
  width 160) read rows 0 to 119. The image is 160 x 120 x 2 = 38400 bytes (0x9600). The list read 38396 bytes past the variable.
- FACT (grep src, include, 3ds/src): no code writes D_80156910. Decision (user said decide): a zero image of 0x9600 bytes, 8-byte aligned.
- FACT (grep src): no caller sets OVERLAY_TYPE_2 (callers screen_overlays.c:580 :593 :595 :660, state_intro.c:92 :262,
  ToadHouse.inc.c:22 with types 3 and 0 from ToadHouse.data.inc.c:86 :101). The draw at :264 runs in no scene. The host test is its proof.
- The initializer lets the RAW rule (gen_gfx_syms.py:21) map D_80156910 as GS_IMG. D_8014EA48 then gets an itab and is gset ok.
  The patch holds the output of `python 3ds/tools/gset.py 3ds/tools/gset.d/OVLIMG.txt build3ds/out/gset_ok.txt` plus :264.
- FACT (oracle host run, main + patch): `ovlimg ok`, `D_8014EA48: itab (unseeded table, ...)`, `gconv lists 1939 ... bad 0`.
- Counters FACT (oracle listcen, main + patch): build 324 -> 323, gfxpos 22 -> 21, data 323 -> 283, data_arrays 9 -> 8, gfxtype 420 -> 419.
- Memory: BSS +38396 bytes (FACT: 0x9600 - 4). The build memcheck prints the margin.
## Edit shapes (src/screen_overlays.c)
- :21 before `BSS s32 D_80156910;` after 2 lines: `extern unsigned char D_80156910[] ALIGNED(8); /* ... */` and
  `unsigned char D_80156910[0x9600] = { 0 }; /* ... */` (the RAW rule needs `^unsigned char NAME[word] =`).
- :264 before `            gSPDisplayList(gMainGfxPos++, D_8014EA48);` after `            nd_draw_list(NDN(D_8014EA48), ND_SELF);`
## Loop (write each output to build3ds/out/, read the last 20 lines)
1. `tr -d '\r' < docs/handoff/ws_OVLIMG.patch | git apply -`. A reject: stop. Send the output to the oracle.
2. Commit the 4 files: `OVLIMG 1: D_80156910 is the 160 x 120 RGBA16 image (ovlimg.sh); D_8014EA48 moves (gset.d OVLIMG); :264 ND_SELF`.
3. `python 3ds/tools/bench.py base spot_mac01_drec --row OVLIMG > build3ds/out/ovlimg_base.txt 2>&1` (base runs, not charged).
4. `sh 3ds/tools/rowloop.sh --row OVLIMG --bench spot_mac01_drec --why "OVLIMG gate" > build3ds/out/ovlimg_loop.txt 2>&1`.
## Tests that decide
- `LOOP PASS`: host PASS, build rc 0 (memcheck margin above 0), the cmp line byte-equal (base = merge-base of HEAD and main).
- `grep -h "ovlimg ok" build3ds/out/loop_OVLIMG_host.txt` prints 1 line.
- `python 3ds/tools/listcen.py | grep "^listcen total"`: build 323, gfxpos 21, data 283, data_arrays 8, gfxtype 419.
## What you may edit to fix a fail
Only the 4 files. Host log line `FAIL hostsel store (read trace)` with `WinError 5` (hostsel.py:123) and every unit ok: run step 4
once more (FACT: it came once in the oracle dry run). A second time: stop, send the host log path to the oracle.
A build error on D_80156910, a memcheck fail or a DIFF: stop. Send the log path in build3ds/out/ to the oracle.
Budget: 2 builds, 4 Azahar starts.
## Stop and hand-back
Last commit `OVLIMG 2: gate. spot_mac01_drec byte-equal; build 323 gfxpos 21 data 283` deletes ws_OVLIMG.md and ws_OVLIMG.patch.
One SubagentHandback: commit hashes, LOOP lines, listcen totals, memcheck margin.
