#ifndef PM_PORT_H
#define PM_PORT_H

/* Plain C seam. Game files include ultra64; GPU files include 3ds.h.
 * Neither side includes the other. */

/* CF5: hot op-loop scalars share one .data.pmhot block (ld/3dsx.ld.in puts it first, 32-byte aligned). */
#define PM_HOTD __attribute__((section(".data.pmhot")))

/* L02: levels 0 error, 1 warn, 2 info, 3 debug. pm_log is info. Switch loglevel=<0..3>, default warn. log=off stops the write to log.txt. */
void pm_log(const char* fmt, ...);
void pm_log_e(const char* fmt, ...);
void pm_log_w(const char* fmt, ...);
void pm_log_i(const char* fmt, ...);
void pm_log_d(const char* fmt, ...);
void pm_log_hw(const char* fmt, ...); /* HWM1: debug level measurement line, kept longest by the queue */
void pm_log_evt(const char* fmt, ...); /* SIZE-0: harness test-event line (shot, until-lines); passes the gate in every mode */
int pm_log_level(void); /* the higher of the live and disk level */
int pm_log_conlevel(void); /* CONLV: console level 0..4, 4 = off */
int pm_log_file(void); /* 1 when the queue takes lines: logging or Profiling on */
int pm_log_live_level(void); int pm_log_disk_level(void); int pm_log_con_level(void); /* LOG5: -1 off, 0 error .. 3 debug */
int pm_log_logging(void); /* LOG5: the logging gate (pm_opt.log), 0 while Profiling runs */
void pm_log_set_levels(int on, int live, int disk, int con);
void pm_log_set_prof_disk(int on); int pm_log_get_prof_disk(void); /* U2 */ /* LOG5: the logging gate and the three levels (options panel, options.ini) */
void pm_log_header(char* con, int nc, char* live, int nl); /* U3: bottom screen header texts */
int pm_log_header_key(void); /* U3: changes when the header text changes */
void pm_log_apply_switch_levels(void); /* LOG3: re-reads loglevel, livelevel, disklevel, conlevel, log from the switches */
void pm_log_set_file(int on);
void pm_log_init(int live);
void pm_log_poll(void);
void pm_log_frame_tick(void); /* LOG4: once per drawn frame after pm_gpu_end */
void pm_con_lock(void);
int pm_con_trylock(void);
void pm_con_unlock(void);
void pm_log_start(void);
void pm_log_live_status(void); /* LIVEMSG: the pm_log_init result, written after pm_log_start */
void pm_log_stop(void);
void pm_log_console_ready(void); /* CPULIM-UI: the bottom console exists now: print the lines that were logged before it did */
extern volatile int pm_cpu_game_value; /* app core 1 time limit the game set (HUD) */
extern volatile int pm_cpu_state;      /* 0 = ok, -1 = the last APT_SetAppCpuTimeLimit call failed */
void pm_cpu_limit_exit(void);
int pm_log_writer_core(void);
unsigned pm_log_writer_hb(void);
unsigned pm_log_dropped(void);
void pm_log_stat(float avg_ms, unsigned frames, float secs, unsigned spikes, float spike_max);
void pm_log_lvtest(unsigned frame); /* CONCOL: test aid, switch lvtest: four lines at frame 100, the marker at frame 125 */
void pm_log_bench(void); /* LOG1: switch logbench, boot-time producer cost per line kind (pm_log.c) */
void pm_log_set_switches(const char* s);
void pm_crash_line(const char* s, int n);
/* LB4: crash exit and hang watchdog (src/pm_wdog.c). The phase marker is one store to a global. */
extern volatile unsigned pm_wd_prog, pm_wd_phase, pm_wd_hold, pm_wd_armed, pm_wd_crash_tick;
/* SLP2: sleep state from the APT hooks (pm_log.c). asleep 1 between sleep enter and wake; sleep_tick is the system tick (ms, low word)
 * at the enter. wake_frames: frames after a wake that begin without the GPU sync wait (pm_gpu.c). */
extern volatile unsigned pm_wd_quiet; /* SLEEP1: HOME suspend, watchdog notes stay in RAM */
void pm_wd_flush_notes(void); /* SLEEP1: RAM notes to crash.txt (bounded step only) */
extern volatile unsigned pm_wd_asleep, pm_wd_sleep_tick, pm_wd_wake_frames, pm_wd_slept; /* slept: 1 after the first sleep, stays 1 */
#define PM_WD_PHASE(n) (pm_wd_phase = (n))
void pm_wd_start(void);
void pm_exit_process(void);
void pm_log_crash_close(void);
unsigned pm_frame_count(void);
void pm_fatal_stop(const char* msg); /* R01: log FATAL, flush, [CRASH] line, svcBreak */
void pm_frame_loop(void);

int pm_debug_has(const char* word);
extern unsigned pm_dbg_gen;
/* O6: a switch word cached until the switch string changes (pm_log_switch, options panel) */
#define PMDL(word) ({ static int v_; static unsigned g_; if (g_ != pm_dbg_gen) { v_ = pm_debug_has(word); g_ = pm_dbg_gen; } v_; })
/* F06-1: frame interpolation record (pm_interp.c). pm_interp_on is -1 until the first start. */
extern int pm_interp_on;
void pm_interp_start(void);
void pm_interp_stop(void);
void pm_interp_open(unsigned a, unsigned b);
void pm_interp_close(void);
void pm_interp_lerp(float t); /* F06-2 */
const float (*pm_interp_find(const void* m))[4];
extern int pm_interp_skip;
extern void pm_interp_skip_count(void);
extern int pm_interp_repl; /* 1 during the t = 0.5 pass */
void pm_interp_record(float mf[4][4], void* m);
const char* pm_debug_text(void);
int pm_debug_value(const char* key, char* out, unsigned n);
unsigned pm_ticks(void);

/* PT01: per-frame phase timer. Switch `phase` turns it on (pm_ph_on). Exclusive time: a nested phase takes its time from
 * the phase that was running. Counters are per drawn frame. All macros are one load and one branch when the switch is off. */
enum { PH_OTHER, PH_LOGIC, PH_DL, PH_GBI, PH_GPU, PH_TEX, PH_AUDIO, PH_ROM, PH_YAY0, PH_FBEGIN, PH_FEND, PH_IDLE, PH_GCACHE, PH_GDRAW, PH_GVTX, PH_GTRI, PH_GMTX, PH_GOP, PH_FBREAD, PH_GTEV, PH_GBIND, PH_GST, PH_N }; /* HWM1: PH_GTEV, PH_GBIND, PH_GST split the draw state cost, timed only with phasegbi */
enum { C_TRI, C_TRIOUT, C_VTXLOAD, C_VTXSLOT, C_DRAW, C_FLUSH, C_TEXCALL, C_TEXMISS, C_TEXBYTE, C_ROMN, C_ROMB, C_YAYN, C_YAYB, C_DBG, C_ATICK, C_CMD, C_AJOB, C_GVLOAD, C_GVTRI, C_GVREJ, C_GVFB, C_GVXF, C_FLCALL, C_FLKB, C_RAWD, C_RAWCALL, C_N };
extern int pm_ph_on;
extern int pm_ph_fine; /* switch phasegbi: time G_VTX, triangle and G_MTX ops on their own (two tick reads per op, so it inflates the frame) */
extern unsigned pm_ph_cnt[C_N];
unsigned pm_ph_prev(int k); /* counter k of the last finished frame */
void pm_ph_init(void);
int pm_au_tk(void); /* audio tick reads on: switch audiostat or phase (pm_os.c). 0 in the default build */
extern int pm_dcs_on; /* switch dcsum (with the log on): the sampled draw and flush timers run without phase */
void pm_ph_push(int id);
void pm_ph_pop(void);
void pm_ph_frame_end(void);
extern int pm_pf_on; /* PF1: switch prof (clean-frame profile), see pm_phase.c */
extern int pm_pf_arm; /* PF2: prof samples frames (prof on, phase off) */
extern int pm_ph_cen; /* PERF1: phase mode on (not a prof sample): the per-op census hooks of the walk (W1, MDL1, WCEN2) */
extern unsigned pm_walk_dbg; /* PROF2-5: the debug word of the walker (nd.h WD_*), nd_walk_run loads it once per list; 0 in a clean frame */
void pm_walk_dbg_set(void); /* PROF2-5 (nd_walk.c): set pm_walk_dbg again after a change of pm_ph_on, pm_ph_cen, pm_ph_fine, nd_c2_on or the orbdbg list */
extern int pm_cen_lines; /* PROF2-3: the census lines (ndm, wide, rect, sprnat, spr5, entnat, nrawwhy, gdl, mdlw) print: phase mode or the census word, logging on. Never in prof mode */
void pm_pf_frame(unsigned long long t0, unsigned long long t1); /* main loop, once per drawn frame: step start and work end ticks */
/* HWM1: scene tag for the phs lines, GPU numbers of the last frame, command buffer fill */
void pm_ph_scene_mode(int mode);
void pm_ph_ui(const char* what, int v); /* UI: menu event line, phase gate */
void pm_ph_scene_map(int area, int map);
void pm_ph_scene_get(int* mode, int* area, int* map);
extern unsigned pm_ph_gpu_us[2]; /* previous frame: citro3d GPU drawing time and citro3d processing time (us), read after C3D_FrameBegin returns */
extern unsigned pm_ph_cmd_pm;    /* command buffer fill of this frame in 1/1000 */
void pm_ph_gpu_note(void);       /* call right after C3D_FrameBegin returns */
#define PHF_PUSH(id) do { if (pm_ph_fine) pm_ph_push(id); } while (0)
#define PHF_POP() do { if (pm_ph_fine) pm_ph_pop(); } while (0)
/* CORE2-3a: the walker side (nd_wq.c). */
extern int pm_walk_side, pm_c2own;
void pm_wq_post(void (*fn)(void*), void* arg, unsigned size);
void pm_wq_call(void (*fn)(void*), void* arg, unsigned size);
void pm_walk_drain(const char* why);
/* CORE2C-2: one frame for the walker (nd_wq.c queue, depth 1). A task is the head of its frame stream (DIRECT, nd_cmd.h) and the byte
 * count of its list (log only). bg: the background task (none: ptr NULL); main: the swap task (none: ptr NULL). cfb: the colour buffer the game built
 * the frame for (the walker reads pm_walk_cfb, not nuGfxCfb_ptr). */
typedef struct { void* ptr; unsigned n; } PmTask;
typedef struct { PmTask bg, main; void* cfb; unsigned flags; } PmFrame;
#define PMF_REPL 1u /* F06-2: the main task walks with the t = 0.5 matrix replacements (pm_interp_repl) */
void pm_wq_frame(const PmFrame* f); /* the game side: queue one frame (waits while one is queued) */
void pm_frame_walk(const PmFrame* f); /* pm_nusys.c, walker side: walk the tasks of one frame */
typedef struct { PmTask t[4]; } PmKey; /* the tasks of the key frame of one step (MAX_KEY 4 in pm_nusys.c) */
int pm_interp_key_take(PmKey* k); /* the key frame tasks the last step recorded (clears the list); 0 when none */
void pm_key_walk(const PmKey* k); /* pm_nusys.c, walker side */
void pm_wq_key(const PmKey* k); /* queue the key frame (nd_wq.c) */
void pm_wq_frame_slot(void); /* logic: wait until the frame and key frame before are walked (the rtab and display context rule) */
extern void* pm_walk_cfb; /* the colour buffer of the frame the walker draws (pm_main.c) */
void pm_walk_own(const char* fn);
int pm_wq_on_logic(void); /* nd_wq.c: the caller runs on the logic thread */
#define PM_WALK_OWN() do { if (pm_c2own > 0 && (!pm_walk_side || pm_wq_on_logic())) pm_walk_own(__func__); } while (0)
/* CORE2C-3: the logic thread (pm_logic.c). The frame loop hands one command per pass: LG_IDLE (the retrace call of an idle pass),
 * LG_STEP (a drawn step: frame begin, step_game_loop and the list build, audio, frame end, the step tail), LG_SKIP (an FS1 skipped step).
 * pad: the pad state of the pass (pm_input.c pm_pad_latch), read by the game once per step. frame: the frame number after the step. */
typedef struct { unsigned short held, down, press; signed char x, y; } PmPad;
enum { LG_IDLE, LG_STEP, LG_SKIP };
#define LGF_ISKIP 1u /* the step is behind: draw the key frame now (pm_interp_skip), no lerp frame */
#define LGF_NOKEY 2u /* no key frame after this step (frame advance hold or step) */
typedef struct { unsigned kind, frame; void* cfb; PmPad pad; unsigned flags; } LgCmd;
void pm_lg_start(void); /* frame loop start: the logic thread (none: serial only) */
void pm_lg_stop(void); /* frame loop end */
unsigned pm_lg_show_keys(void); /* game thread: walk the key frames of the steps shown; the count not yet counted as drawn */
void pm_lg_hand(const LgCmd* c, int serial); /* run the command: inline (serial) or on the logic thread */
int pm_lg_draw_flag(void); /* the retrace flag (D_80073E0A) after the last handed command; waits for its retrace to start */
void pm_lg_show(void); /* walk the queue through the end of every finished step */
void pm_lg_quiesce(void); /* game thread: the logic thread is idle and the queue is empty */
void pm_lg_sleep_ns(long long ns); /* game thread: sleep, running the queue for a blocked logic thread */
void pm_lg_publish_d(int d); /* pm_nusys.c: the retrace flag after this retrace call */
void pm_step_run(const LgCmd* c); /* pm_main.c: the body of a command */
void pm_pad_latch(PmPad* p); /* pm_input.c, frame loop: the pad state since the last latch */
void pm_pad_step(const PmPad* p); /* pm_input.c, logic: the pad state the game reads in this step */
#define PH_PUSH(id) do { if (pm_ph_on) pm_ph_push(id); } while (0)
#define PH_POP() do { if (pm_ph_on) pm_ph_pop(); } while (0)
#define PH_ADD(c, n) do { if (pm_ph_on) pm_ph_cnt[c] += (unsigned)(n); } while (0)
/* FS2: switch fsplit. Coarse split of a drawn frame, one tick read per mark (12 per drawn frame). Each mark closes the open part and opens the next. */
enum { FS_REST, FS_LOGIC, FS_DL, FS_GBI, FS_FLUSH, FS_GWAIT, FS_SWAP, FS_AUDIO, FS_N };
extern int pm_fs_on;
extern int pm_fs_q; /* 1 during a key frame draw: no mark */
extern unsigned pm_fs_tasks; /* WMODEL: gbi tasks walked in this frame (counted only with fsplit) */
void pm_fs_mark(int cat);
#define FS_MARK(c) do { if (pm_fs_on) pm_fs_mark(c); } while (0)
extern unsigned pm_gbi_sync_ticks; /* ticks in sync_combine_work this drawn frame (phase on) */
extern unsigned pm_gbi_gv_ticks; /* ticks in G_TRI1/G_TRI2 streaks this drawn frame (phase on) */
extern unsigned pm_gbi_tex_ticks; /* ticks in texture-command streaks this drawn frame (phase on) */
extern unsigned pm_gbi_cur_op; /* PI2 */
int pm_gbi_flush_class(void); /* PI2 */
extern unsigned pm_fl_n[6], pm_fl_e[6], pm_fl_us[6];
/* DC1: plain counts per drawn frame (closes, raw, palette uploads, matrix multiplies, TEV rebuilds, real binds per unit, staged binds) */
enum { DC_CL, DC_RAW, DC_PALUP, DC_PMUL, DC_TEVR, DC_TB0, DC_TB1, DC_TB2, DC_TBS, DC_SHM, DC_PRG, DC_BUF, DC_PKEQ, DC_N }; /* DC6: DC_SHM = shadow misses (a setter wrote the GPU) */
/* DC1: sample sums in us over the sampled closes (one close in 8 with phase on); the log line multiplies them by 8 */
enum { DCU_PAL, DCU_BIND, DCU_TEV, DCU_ST, DCU_DRAW, DCU_KICK, DCU_N };
extern unsigned pm_dc_c[DC_N], pm_dc_us[DCU_N];
/* DS1: flush a CPU-written range for the GPU with svcFlushProcessDataCache (no GSP IPC). A failed svc falls back to GSPGPU_FlushDataCache, counted in pm_fl_gsp_n. */
void pm_fl_dc(const void* p, unsigned n);
extern unsigned pm_fl_gsp_n;
extern unsigned pm_svc_n, pm_ec_n, pm_ec_svc; /* EC2: svc reads, empty closes, svc reads inside empty closes (phase on, per drawn frame) */ /* PI2: batch closes, empty closes, us per class, us of the empty closes */
enum {
    CS_PLAN, CS_MODE, CS_PRIM, CS_ENV, CS_LAY, CS_CON,
    CS_PF_N, CS_PF_G, CS_PF_CNAME, CS_PF_BUFNAME, CS_PF_VTXNAME,
    CS_PF_UTEX, CS_PF_USHA, CS_PF_UT1, CS_PF_UNOISE, CS_PF_KS, CS_PF_FOLD,
    CS_N
};
extern unsigned pm_cs_cnt[CS_N];
#define CS_ADD(c, n) do { if (pm_ph_on) pm_cs_cnt[c] += (unsigned)(n); } while (0)

void pm_pad_scan(void);
void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger);

void pm_game_bind(void);
int pm_rom_init(void);
/* BOOT-1: boot timing marks (pm_main.c). pm_boot_mark stores the svc tick of point id once; arg is the ROM byte count for PMB_ROM_READ. */
enum { PMB_MAIN, PMB_THREAD, PMB_LOG, PMB_OPT, PMB_ROM_OPEN, PMB_ROM_READ, PMB_GFX, PMB_AUDIO, PMB_DRAW, PMB_TITLE, PMB_N };
void pm_boot_mark(int id, unsigned arg);
unsigned pm_rom_order_n(void); /* BOOT step 0: 128 KB ROM chunks touched so far (switch romorder, pm_os.c) */
void pm_rom_drop_stat(void);
void pm_heaphi_line(unsigned frame); /* MEM-B: pm_heaphi.c */
void pm_fatal_screen(const char* msg);
void pm_crash_install(void* stack, unsigned size);
void pm_stack_register(int id, const char* name, unsigned size);
unsigned pm_stack_used(int id, unsigned* size);
void pm_fpscr_setup(const char* who);
int pm_mem_crash_line(char* buf, int cap);
void pm_gpu_log_stats(const char* tag);
void* pm_soc_buffer(unsigned* size);
int pm_started_by_netload(void);
void pm_soc_free(void);
void pm_audio_shutdown(void);
void pm_log_soc_exit(void);
const unsigned char* pm_rom_ptr(unsigned addr, unsigned len);
void pm_flash_mark_all(void);
void pm_flash_put(unsigned off, const void* src, unsigned len);
void pm_flash_fill(unsigned off, unsigned len);
void pm_flash_attach(unsigned char* mem, unsigned size);
void pm_flash_flush(const char* why);
void pm_flash_crash_flush(void);
extern volatile int pm_flash_warn; /* SAVESAFE: the last save write failed */
unsigned pm_soc_alloc_rc(void);
unsigned pm_heap_bytes(void);
void pm_mem_boot_line(unsigned need, unsigned leave, unsigned cap); /* MEMGATE: the boot memory line (pm_mem.c) */
unsigned pm_linear_bytes(void);
void pm_mem_log(const char* tag);
void pm_gfx_retrace(void);
void pm_gfx_retrace_skip(void); /* FS1 */
void pm_audio_init(void);
int pm_cheat_moon_get(void); void pm_cheat_moon_set(int on); void pm_cheat_tick(void); int pm_cheat_get(int k); void pm_cheat_set(int k, int on); /* CH6, CH7 */
int pm_warp_story_n(void); const char* pm_warp_story_label(int i); void pm_warp_story_apply(int i); /* CH8 */
const char* pm_dbgg_ch8_text(int k); void pm_dbgg_ch8_step(int k, int dir); void pm_dbgg_ch8_act(int k); /* CH8 */ /* CH6 Moon Jump, pm_dbg_game.c */
void pm_audio_mute(int on); /* PAUSE AUDIO: the Select menu mutes the output */
void pm_audio_post(const void* pcm, unsigned bytes, unsigned rate);
void pm_au_thread_start(void (*entry)(void*), void* arg);
int pm_au_thread_started(void);
int pm_au_thread_yield(void);
void pm_audio_kick(unsigned n);
void pm_audio_join_at(int site); /* 0 kick, 1 pm_gfx_retrace, 2 step_game_loop, 3 main loop */
#define pm_audio_join() pm_audio_join_at(3)
int pm_on_audio(void);
int pm_audio_serve(void);
void pm_audio_retrace_send(void);
void pm_audio_tick_end(void);
unsigned pm_audio_retraces(void);
unsigned pm_audio_pending(void);
void pm_audio_hash_log(void);
void pm_audio_drain(int reason); /* AU2: join the control job, then wait for the render thread (reason: AU2_SYNC_* in pm_au2.h) */
int pm_audio_join_timed(int site, unsigned long long ns); /* sleep hook: 0 = idle, -1 = deadline */
int pm_au2_sync_timed(int reason, unsigned long long ns); /* sleep hook: 0 = render queue idle, -1 = deadline */
void pm_dspa_rearm(void); /* after sleep: channels set up again (pm_dspaud.c) */
/* sleep and watchdog exit: run fn on a worker thread (slot PM_STEP_*), wait at most ms. 1 = fn done, 0 = deadline, -1 = skipped (slot busy or no thread). */
#define PM_STEP_FLASH 0
#define PM_STEP_EXIT 1
int pm_step_timed(int slot, void (*fn)(void*), void* arg, unsigned ms);
void pm_audio_step(void); /* AU1b: once per drawn step */
/* B7-0: swirl capture (pm_cfb_capture_prev) and F03 depth query timing, read and cleared by the b7 line in the former pm_gbi.c. ticks of the ARM11 clock */
typedef struct PmB7 { unsigned cap_n, cap_ticks, cap_max, dq_n, dq_ticks, dq_max; } PmB7;
extern PmB7 pm_b7;

/* MS01: static data of a map is restored when the map is entered again (pm_mapdata.c, map_restore_table.c is generated). */
typedef struct PmMapRst {
    const char* id;          /* map id, or machi / dgb for the files shared by the maps of an area */
    unsigned char *d0, *d1;  /* block of the .data symbols that change at run time (pristine copy in save) */
    unsigned char *b0, *b1;  /* block of all .bss of the map (zeroed) */
    unsigned char* save;
} PmMapRst;
void pm_mapdata_enter(const char* id);

/* F11-1: port settings (pm_options.c). The file is options.ini in the working folder (options.txt is read once as a fallback). */
typedef struct PmOpt {
    int fps60, wide, dbg_hud, dbg_stats, dbg_nofog, dbg_nolight;
    int o3ds_z; /* C-O3DS: 0 = X is the Z key on the O3DS, 1 = Y (options.ini o3dsz) */
    int log, livelevel, disklevel, conlevel, prof_disk; /* U2: prof_disk 1 writes profiling lines to log.txt too (options.ini prof_disk, switch profdisk=0|1, default 0). LOG5: log = logging on (default 1, options.ini log_on). livelevel, disklevel, conlevel: -1 off, 0 error, 1 warn (default), 2 info, 3 debug (options.ini live_level, disk_level, con_level) */
    int fixedseed; /* F11-7: 1 = same random start seed every boot (as the switch fixedseed), read at boot */
    int dpad_mode; /* UI1c: 0 Multifunction, 1 D-PAD to C-Stick, 2 D-PAD to Analog (N3DS only). options.ini dpad_mode=multi|cstick|analog */
    int dz_pad, dz_cs; /* PT61: dead zone in percent of full travel, circle pad and C-Stick (N3DS). 0 = as before (no port dead zone) */
    int rep_delay, rep_rate; /* PT60: menu key repeat in ms, the first step is at once, the next after rep_delay, then every rep_rate */
    int fskip; /* FS1: 1 = frame skip on (default, a late step skips its draw), 0 = off: every game step is drawn. options.ini fskip=0|1, panel row 9 */
    int hud_ms; /* HUDINT: refresh interval of the bottom screen memory rows in ms: 250, 500, 1000 (default), 2000, 5000. options.ini hudint=<ms> */
    int pos_hud; /* POSHUD: 1 = the bottom screen HUD has a Mario position row, drawn every frame. options.ini poshud=0|1, Debug tab row 8 */
    int poslog; /* MN7 rest: 1 = the L+R+Y+B combo logs a position line (pm_input.c). options.ini poslog=0|1, Debug row Position log combo (option row 12) */
} PmOpt;
void pm_fskip_set(int on); /* pm_main.c: applies fskip at once */
extern int pm_new3ds; /* 1 on a New 3DS, set in pm_main.c */
extern PmOpt pm_opt;
void pm_pos_text(char* out); /* POSHUD: pm_route.c, writes the Mario row text (40 bytes) */
int pm_map_hud_step(unsigned step, unsigned hold, char (*rows)[40]); /* MAPHUD: pm_route.c, fills the 3 map rows (HUD rows 4 to 6) always; returns 1 during the hold after a change */
const char* pm_map_now(void); /* BOTLAYOUT: pm_route.c, the footer map row text */
void pm_gpu_footer(const char* map, const char* sys); /* BOTLAYOUT: footer rows 28 (map) and 29 (apt, model, build), drawn by the sink thread */
void pm_options_init(void);
void pm_options_write(void);
/* F11-3: the options panel (src/filemenu/filemenu_options.c). Rows 0 to 5 are fps60 wide hud stats nofog nolight, row 8 is Fixed seed, row 9 is Frame skip, row 10 is Position HUD, row 12 is Position log combo. LOG5: the logging rows (Logging, Live log, Log to disk, Console log) are the Logging page, not option rows. */
int pm_options_get(int row);
int pm_optui_grey(int tab, int row); /* LOG5: the Logging rows that grey while logging is off (cursor skip and colour) */
void pm_options_flip(int row);
void pm_options_close(void);
const unsigned char* pm_options_build_msg(void); /* BN1 footer text */
const unsigned char* pm_options_msg(const char* text, int slot); /* UI1a: text as game message, slot 0 to 7 */
/* UI1a, PT59: tab 0 General, 1 Controls, 2 Debug. mode 0 = file select panel, mode 1 = Select menu (port menu): the same rows, plus Return to Title and Exit to Homebrew */
int pm_optui_rows(int tab, int mode);
const char* pm_optui_label(int tab, int row, int mode);
const char* pm_optui_value(int tab, int row, int mode);
int pm_optui_step(int tab, int row, int dir, int mode); /* left or right on a row, 1 when the value changed */
int pm_optui_act(int tab, int row, int mode);
int pm_optui_warn(void); void pm_optui_warn_confirm(void); int pm_optui_sub_pop(int row); int pm_optui_sub_restore(void); int pm_optui_sub_enter(void); int pm_optui_sub_level(void); /* MN6 */ /* 8 open an area page (depth 2) */ /* 0 done, 1 close (Back), 3 exit to Homebrew, 4 return to title, 5 warp started, 6 open the Warp area submenu (tab index PM_OPT_SUB) */
#define PM_OPT_SUB 4 /* the folder page (Warp area or Boss rows): a page index after General, Controls, Debug and the reserved tab 3 */
void pm_optui_disarm(void);
int pm_optui_alert(int tab, int row, int mode); /* 1 when the row asks for the second press (Exit to Homebrew, A again): drawn in red */
/* PT60: hold repeat of both menus. dir is -1, 0 or 1. tps is the call rate (30 game updates, 60 pad scans). Returns 1 when a step fires. n counts the steps of this hold. */
typedef struct { int dir, t, n; } PmRepeat;
int pm_repeat(PmRepeat* r, int dir, int tps);
int pm_pad_take_tab(void); /* UI1a: -1 for L, +1 for R (3DS keys) since the last call */
void pm_log_switch(const char* word, int on);
int pm_pad_take_select(void);
/* F11-5: rebinding. 17 3DS inputs, each gives one N64 button mask (0 = none). SELECT opens the port menu and is not in the table. */
#define PM_NIN 17
enum { PM_IN_A, PM_IN_B, PM_IN_X, PM_IN_Y, PM_IN_L, PM_IN_R, PM_IN_ZL, PM_IN_ZR, PM_IN_START, PM_IN_DUP, PM_IN_DDOWN, PM_IN_DLEFT, PM_IN_DRIGHT, PM_IN_CSUP, PM_IN_CSDOWN, PM_IN_CSLEFT, PM_IN_CSRIGHT };
extern unsigned short pm_map[PM_NIN];
extern const char* const pm_in_name[PM_NIN];
const char* pm_n64_name(unsigned short mask);
unsigned short pm_n64_next(unsigned short mask); /* the next N64 button in the cycle NONE A B Z START L R CUP CDOWN CLEFT CRIGHT */
void pm_map_reset(void);
int pm_free_overworld(void); /* UI1c: 1 when the player has free control in the world (pm_dpad.c) */
const char* pm_dpad_mode_name(void); /* UI1c: name of the mode for the device */
void pm_dpad_mode_next(void);
int pm_warp_preset_count(void);
const char* pm_warp_preset_name(int i); /* UI1a */
const char* pm_warp_preset_map(int i);
int pm_warp_area_count(void); /* MN6 rest: the Warp area folder, one row per area that has a non-test map (gAreas order) */
const char* pm_warp_area_name(int a); /* the English area name (pm_warp.c area_name) */
int pm_warp_area_size(int a); /* maps of area a */
int pm_warp_area_map(int a, int j); /* the flat map index k of map j of area a */
int pm_warp_map_entries(int k); /* entry count of map k (at least 1) */
int pm_warp_entry_get(int k); /* the entry chosen for map k on the area page (RAM) */
void pm_warp_entry_set(int k, int e);
extern int pm_warp_pending; /* UI1a: set by pm_warp_now, read by filemenu_get_exit_mode */
void pm_warp_now(int choice, int entry); /* UI1a, PT01: warp from the file select to a choice (preset or map) */
int pm_warp_map_count(void);
const char* pm_warp_map_name(int k);
int pm_warp_map_index(const char* name);
int pm_warp_is_test_map(const char* id); /* PT01b */
void pm_menu_msg(const char* fmt, ...); /* PT76: a message in the 2 row feedback area of the Select menu (and a log warning) */
int pm_boss_count(void); const char* pm_boss_label(int i); int pm_boss_launch(int i); /* Boss tab of the Select menu: warp to mac_00 and start the battle, the player fights */
int pm_warp_live(int choice, int entry); /* PT63: warp at once from the Select menu (no file select). 1 = started */
void pm_title_return(void); /* PT66: soft reset to the title, as the game own reset does */
void pm_options_touch(void); /* mark the settings changed, saved at pm_options_close */
/* F11-6: the port menu on the bottom screen (3ds/src/pm_options_ui.c). Select, when no game code takes it, opens it in any game mode. */
extern volatile int pm_menu_open;
void pm_menu_set(int on);
void pm_menu_key(unsigned hid_keys_down);
void pm_menu_hold(int dx, int dy); /* PT60 */
void pm_gpu_menu_draw(const char* text);
void pm_gpu_shot_bottom(const char* name); /* MN0: writes sdmc:/3ds/PaperMario/shots/<name>.ppm */
void pm_gpu_menu_present(void); /* MN1: shows a menu change while the game is frozen */
void pm_gpu_menu_grey(void); /* MN2: the top screen grey while the menu is open */

/* MAPX-1: the xfer line (pm_xfer.c). One line per door or map transition: frames, step ms, and the cost of each load stage. The counters
 * below are cumulative inside one transition (reset at its start); the tick sums run only while a transition is open (pm_xf.on). */
typedef struct {
    int on;                       /* a transition is open: the stage ticks run */
    unsigned rom_n, rom_b;        /* nuPiReadRom on the game thread: calls, bytes */
    unsigned yay_n, yay_b, yay_t; /* decode_yay0: calls, output bytes, ticks */
    unsigned yay_pre;             /* MAPX-4: decode_yay0 calls served by the core-1 prefetch */
    unsigned inv_n;               /* pm_tex_invalidate calls */
    unsigned au_t, load_t;        /* map load: pm_audio_drain ticks, whole load_map_by_IDs ticks */
    unsigned dlc_n, dlc_t;        /* dlc_build calls, ticks */
    unsigned mdl_miss;            /* model cache misses (nd_recs.c rec_run) */
    unsigned ent_n;               /* ent_compile calls */
    unsigned tex_n, tex_b, tex_t; /* texture cache misses (decode and upload): count, bytes, ticks */
    int load_step;                /* the step in progress ran load_map_by_IDs */
    unsigned warm_f, warm_skip;   /* MAPX-3: covered frames that built under a budget, model record misses they skipped */
    int diag;                     /* switch xferdiag: each model cache miss is classed by the first key word that differs (nd_mc_xdiag) */
    unsigned wh[93];              /* xferdiag: misses per first differing key word (92: no stored entry of the same model) */
    unsigned mdl_rng;             /* DOORHITCH: key hits refused by the group mask or the matrix version (pm_gpu_model_hit), rebuilt and not in mdl_miss */
    unsigned rdw_t;               /* DOORHITCH: ticks of the walk drain before a ROM read (nuPiReadRom) */
    unsigned hid_d;               /* DOORHITCH: hidden models of the new map given their compiled list before a first draw (covered frames) */
    int step_smp;                 /* DOORHITCH: the step in progress is a prof sample (phase timers on); set by pm_main before pm_xfer_step */
} PmXf;
extern PmXf pm_xf;
void pm_xfer_map_begin(void);                     /* set_game_mode(GAME_MODE_CHANGE_MAP) */
void pm_xfer_map_load(int area, int map);         /* __wrap_load_map_by_IDs: the target */
void pm_xfer_door(int state);                     /* rooms.c SetDoorState: 1 entering, 2 exiting, 0 idle */
void pm_xfer_step(unsigned long long t0, unsigned long long t1); /* pm_main.c: once per game step, its work ticks */
void pm_xfer_hidden(void);                        /* DOORHITCH: logic thread, a drawn step after its list build (pm_step_run) */
int pm_xfer_skip(int kind); /* MAPX-3: 1 when a cache build of this kind (0 model record miss) is over the budget of a covered frame (skip its draw) */

#endif
