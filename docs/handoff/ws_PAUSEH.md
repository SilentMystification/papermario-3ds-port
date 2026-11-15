# ws_PAUSEH: the pause keeps the copy of its first frame (Haiku). Start after `Merge HWFIX` (HWFIX owns 3ds/src/pm_gpu.c).
Objective: the pause background shows the frozen frame, not black. Counters C1..C12: no change (FACT: no list code is edited).
Cause (FACT, code read): every SHOW_PAUSED frame asks a new copy of the last drawn frame (src/background_gfx.c:343, pm_gpu_want_prev).
pm_gpu_begin copies the target colour into grab_tex (3ds/src/pm_gpu.c:2927). The last drawn frame is the previous pause frame. That frame
is already dimmed by PM_CC_43 with prim (40,40,40,fade) (background_gfx.c:363-364). So each frame dims the last one. The picture goes to
(40,40,40) in about 10 frames (FACT: fade 128 of 255 per frame, :346-347). The Select menu grey (pm_gpu.c:1947-1966) has no black path.
Files: 3ds/src/pm_gpu.c, src/background_gfx.c. Order: pm_gpu.c, then background_gfx.c. One group, one commit.
## Edit 1, pm_gpu.c:264. Before:
    static int prev_want, prev_pre; /* PT12: see pm_gpu_grab_prev */
After:
    static int prev_want, prev_pre; /* PT12: see pm_gpu_grab_prev */
    static int prev_hold; static unsigned hold_n, hold_rst; /* PAUSEH: the pause keeps the grab_tex copy of its first frame (pm_gpu_hold_prev) */
## Edit 2, pm_gpu.c:2716-2722. Add these lines after line 2722 (the same post form as pm_gpu_want_prev):
    /* PAUSEH: a pause frame keeps the copy in grab_tex for the next 2 drawn frames: pm_gpu_begin makes no new copy (src/background_gfx.c SHOW_PAUSED). */
    static void pm_gpu_hold_prev_w(void);
    typedef struct { int unused; } Wq_pm_gpu_hold_prev;
    static void wq_pm_gpu_hold_prev(void* a) { (void)a; pm_gpu_hold_prev_w(); }
    void pm_gpu_hold_prev(void) { Wq_pm_gpu_hold_prev q = { 0 }; pm_wq_post(wq_pm_gpu_hold_prev, &q, sizeof q); }
    static void pm_gpu_hold_prev_w(void) { prev_hold = 2; }
## Edit 3, pm_gpu.c:2927-2928. Before:
    if (prev_want && grab_tex_init()) { grab_queue_copy(); prev_pre = 1; kc_skip_n = ((gxCmdQueue_s*)&__C3D_Context)->numEntries; } /* PT12: ... */
    if (prev_want) prev_want--;
After (keep the PT12 comment text on the else line):
    if (prev_hold) { prev_pre = grab_tex_init(); if (++hold_n == 30u) pm_log("pauseh f%u held 30 resets %u", pm_frame_count(), hold_rst); } /* PAUSEH: no new copy */
    else if (prev_want && grab_tex_init()) { hold_n = 0; grab_queue_copy(); prev_pre = 1; kc_skip_n = ((gxCmdQueue_s*)&__C3D_Context)->numEntries; } /* PT12: ... */
    if (prev_want) prev_want--;
    if (prev_hold) prev_hold--;
## Edit 4, pm_gpu.c:2783. Before:
    if (!prev_frame) grab_queue_copy(); /* PT12: the old frame was copied at pm_gpu_begin */
After:
    if (!prev_frame) { grab_queue_copy(); if (prev_hold) { prev_hold = 0; hold_rst++; } } /* PT12; PAUSEH: a grab overwrote grab_tex: the next frame copies again */
## Edit 5, src/background_gfx.c:252 and :343. Add after line 252:
    extern void pm_gpu_hold_prev(void); /* PAUSEH: pm_gpu.c, the pause keeps the copy of its first frame */
Line 343 before: `pm_gpu_want_prev(); /* PAUSEOPEN: each drawn frame of the pause asks for its copy (the request holds 2 drawn frames) */`
Line 343 after:  `pm_gpu_hold_prev(); /* PAUSEH: each drawn pause frame keeps the copy of the first one (a new copy is the dimmed frame) */`
Do not change line 333 (BEGIN_PAUSED asks the one copy) or src/encounter.c:1275.
## Loop and gates (1 build, 2 Azahar runs)
1. `sh 3ds/tools/nd_host_test.sh` > build3ds/out/pauseh_host.txt. Pass: PASS.
2. One build (build.sh). Pass: rc 0, memcheck OK.
3. `python 3ds/tools/bench.py run pause_gut_dbg_w300 --row PAUSEH --why "pause hold line"` (FACT: PAUSEFADE gate bench, b824521854).
   Pass: run.log has one line `pauseh f<n> held 30 resets 0`. Seeded fault: the base source has no string `pauseh` (grep 3ds/src on
   $(git merge-base HEAD main)), so the base cannot pass this check. GUESS: the bench stays paused 30 or more drawn frames.
4. `python 3ds/tools/bench.py cmp spot_mac01_drec --row PAUSEH --commit $(git merge-base HEAD main)`. Pass: byte-equal (no pause there).
Fix a failing test: edit only the lines of Edits 1-5. No line in step 3: send build3ds/out/pauseh_run.txt (last 40 lines of run.log) to the oracle.
Stop: steps 1-4 pass, one commit. Hand-back: build3ds/out/handback_PAUSEH.txt (commit, host, build rc, pauseh line, cmp result). Delete this page in the commit.
