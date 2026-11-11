# Row LIBTHR: crash handlers on the libctru threads (APT, GSP event, ndsp)

(Model: Haiku. Base: main 4db2dfadf. Edits pm_log.c cpu_hook, pm_gpu.c pm_gpu_init (not the FXCEN1 functions nd_native_submit and
pm_gpu_cnt_now), pm_audio.c the ndsp init function. Disjoint from CRASHEXIT2, PULLCRASH, MAPROMFS. Reads only this file and CLAUDE.md.)

## Cause (oracle code read, main 4db2dfadf)
- A crash handler is installed per thread (threadOnException) only by pm_thread_create (pm_thread.c th_entry :39) and
  pm_thread_adopt (:67, used once: pm_main.c:944 "main"). Every grep hit of threadCreate in 3ds/src is pm_thread.c.
- libctru makes its own threads: the APT event thread (aptInit), the GSP event thread (gfxInitDefault -> gspInit, pm_gpu.c:1543)
  and the ndsp thread (ndspInit, pm_audio.c:55). soc, romfs and hid make none. A fault on those three threads has no handler:
  no crash.txt line and no crash page.
- Each runs our code in a callback, so it can adopt itself once: APT runs the aptHook callbacks (cpu_hook, pm_log.c:1203, hooked at
  :1263; some hooks may run on the main thread: adopt only a thread with no name), GSP runs gspSetEventCallback callbacks, ndsp runs
  the ndspSetCallback callback (not used by the game today: grep ndspSetCallback finds none). pm_thread_adopt takes a pool slot
  (PM_TH_MAX 19, pm_thread.c:11; about 15 in use) with its own 4 KB handler stack and installs the handler.

## Start
`sh 3ds/tools/wt_new.sh LIBTHR`. PM_ROW=LIBTHR PM_LANE=LIBTHR on every build and bench call.

## Exact edits
1. pm_log.c cpu_hook: first statement `if (!pm_thread_name_current()) pm_thread_adopt("apt"); /* LIBTHR: a hook on the APT thread gets a crash handler */`
   (include pm_thread.h if the two functions are not declared in pm_log.c).
2. pm_gpu.c pm_gpu_init, after `gfxInitDefault();` (:1543): `gspSetEventCallback(GSPGPU_EVENT_VBlank0, gsp_adopt, NULL, true); /* LIBTHR: one shot, on the GSP event thread */`
   and above pm_gpu_init: `static void gsp_adopt(void* d) { (void)d; if (!pm_thread_name_current()) pm_thread_adopt("gsp"); }`.
3. pm_audio.c, after `ndspSetOutputMode(NDSP_OUTPUT_STEREO);`: `ndspSetCallback(ndsp_adopt, NULL); /* LIBTHR: the ndsp thread adopts itself at its first frame */`
   and above the function: `static void ndsp_adopt(void* d) { static int done; (void)d; if (done) return; done = 1; if (!pm_thread_name_current()) pm_thread_adopt("ndsp"); }`.
4. pm_thread.c pm_thread_adopt: when slot_take fails, `pm_log_w("thread %s: no crash slot (%d), not adopted", name, PM_TH_MAX);`.

## Gate (CLAUDE.md 2; Azahar budget 2 of 8)
- Build, memcheck OK; `bench.py run nd_host --row LIBTHR --why "adopt hooks"`: PASS.
- `bench.py base spot_mac01_det`, `bench.py run spot_mac01_det --row LIBTHR --why "no draw change; no crash slot line"`,
  `bench.py cmp spot_mac01_det --row LIBTHR`: 6/6 byte-equal; the LOG: copy has no `no crash slot` line.

## Stop and end
A failed gate, an edit that does not fit, or a tool fault: commit, hand back the exact output (CLAUDE.md 4-5). Delete this row file
in the completing commit. Hand back the hash, the gates, the memcheck line.
