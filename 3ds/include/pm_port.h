#ifndef PM_PORT_H
#define PM_PORT_H

/* Plain C seam. Game files include ultra64; GPU files include 3ds.h.
 * Neither side includes the other. */

/* L02: levels 0 error, 1 warn, 2 info, 3 debug. pm_log is info. Switch loglevel=<0..3>, default warn. log=off stops the write to log.txt. */
void pm_log(const char* fmt, ...);
void pm_log_e(const char* fmt, ...);
void pm_log_w(const char* fmt, ...);
void pm_log_i(const char* fmt, ...);
void pm_log_d(const char* fmt, ...);
void pm_log_hw(const char* fmt, ...); /* HWM1: debug level measurement line, kept longest by the queue */
int pm_log_level(void);
int pm_log_file(void);
void pm_log_set_level(int lv);
void pm_log_set_file(int on);
void pm_log_init(int live);
void pm_log_poll(void);
void pm_con_lock(void);
int pm_con_trylock(void);
void pm_con_unlock(void);
void pm_log_start(void);
void pm_log_stop(void);
void pm_log_console_ready(void); /* CPULIM-UI: the bottom console exists now: print the lines that were logged before it did */
extern volatile int pm_cpu_game_value; /* app core 1 time limit the game set (HUD) */
extern volatile int pm_cpu_state;      /* 0 = ok, -1 = the last APT_SetAppCpuTimeLimit call failed */
void pm_cpu_limit_exit(void);
int pm_log_writer_core(void);
unsigned pm_log_writer_hb(void);
unsigned pm_log_dropped(void);
void pm_log_stat(float avg_ms, unsigned frames, float secs);
void pm_log_top(void);
void pm_log_set_switches(const char* s);
void pm_crash_line(const char* s, int n);
/* LB4: crash exit and hang watchdog (src/pm_wdog.c). The phase marker is one store to a global. */
extern volatile unsigned pm_wd_prog, pm_wd_phase, pm_wd_hold, pm_wd_armed, pm_wd_crash_tick;
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
void pm_ph_init(void);
void pm_ph_push(int id);
void pm_ph_pop(void);
void pm_ph_frame_end(void);
/* HWM1: scene tag for the phs lines, GPU numbers of the last frame, command buffer fill */
void pm_ph_scene_mode(int mode);
void pm_ph_scene_map(int area, int map);
void pm_ph_scene_get(int* mode, int* area, int* map);
extern unsigned pm_ph_gpu_us[2]; /* previous frame: citro3d GPU drawing time and citro3d processing time (us), read after C3D_FrameBegin returns */
extern unsigned pm_ph_cmd_pm;    /* command buffer fill of this frame in 1/1000 */
void pm_ph_gpu_note(void);       /* call right after C3D_FrameBegin returns */
#define PHF_PUSH(id) do { if (pm_ph_fine) pm_ph_push(id); } while (0)
#define PHF_POP() do { if (pm_ph_fine) pm_ph_pop(); } while (0)
#define PH_PUSH(id) do { if (pm_ph_on) pm_ph_push(id); } while (0)
#define PH_POP() do { if (pm_ph_on) pm_ph_pop(); } while (0)
#define PH_ADD(c, n) do { if (pm_ph_on) pm_ph_cnt[c] += (unsigned)(n); } while (0)
/* RSP split counters (one rsplit line per drawn frame with phase): crossings the phase line does not name. */
enum { RS_TILE, RS_COMB, RS_XDUP, RS_CLIP, RS_PCOMP, RS_DLC, RS_N };
extern unsigned pm_rs_cnt[RS_N];
#define RS_ADD(c, n) do { if (pm_ph_on) pm_rs_cnt[c] += (unsigned)(n); } while (0)
enum { CS_PLAN, CS_MODE, CS_PRIM, CS_ENV, CS_LAY, CS_CON, CS_N };
extern unsigned pm_cs_cnt[CS_N];
#define CS_ADD(c, n) do { if (pm_ph_on) pm_cs_cnt[c] += (unsigned)(n); } while (0)

void pm_pad_scan(void);
void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger);

void pm_game_bind(void);
int pm_rom_init(void);
void pm_rom_frame_end(void); /* SD stream: one line when a drawn frame spends over 500 ms in pi_read */
void pm_fatal_screen(const char* msg);
void pm_crash_selftest(void);
void pm_crash_install(void* stack, unsigned size);
void pm_stack_register(int id, const char* name, unsigned size);
unsigned pm_stack_used(int id, unsigned* size);
void pm_fpscr_setup(const char* who);
int pm_mem_crash_line(char* buf, int cap);
void pm_gpu_log_stats(const char* tag);
void* pm_soc_buffer(unsigned* size);
int pm_started_by_netload(void);
void pm_soc_free(void);
void pm_exit_mark(const char* what);
void pm_audio_shutdown(void);
void pm_log_soc_exit(void);
const unsigned char* pm_rom_ptr(unsigned addr, unsigned len);
void pm_flash_mark(unsigned off, unsigned len);
void pm_flash_attach(unsigned char* mem, unsigned size);
void pm_flash_flush(const char* why);
unsigned pm_soc_alloc_rc(void);
unsigned pm_heap_bytes(void);
unsigned pm_linear_bytes(void);
void pm_mem_log(const char* tag);
void pm_gfx_retrace(void);
void pm_gfx_retrace_skip(void); /* FS1 */
void pm_audio_init(void);
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
int pm_audio_lock(void);
void pm_audio_hash_log(void);
void pm_audio_drain(int reason); /* AU2: join the control job, then wait for the render thread (reason: AU2_SYNC_* in pm_au2.h) */
void pm_audio_step(void); /* AU1b: once per drawn step */
/* B7-0: swirl capture (pm_cfb_capture_prev) and F03 depth query timing, read and cleared by the b7 line in pm_gbi.c. ticks of the ARM11 clock */
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
    int log, loglevel; /* L02, F11-8: log file on or off (default 1), level 0 error, 1 warn (default), 2 info, 3 debug */
    int fixedseed; /* F11-7: 1 = same random start seed every boot (as the switch fixedseed), read at boot */
    int dpad_mode; /* UI1c: 0 Multifunction, 1 D-PAD to C-Stick, 2 D-PAD to Analog (N3DS only). options.ini dpad_mode=multi|cstick|analog */
    int warp; /* UI1a, PT01: 0 = off, 1 to P = warp preset, P+1 and up = map k of gAreas (options.ini warp=<preset> or warp=<map>:<entry>) */
    int warp_entry; /* PT01: entry of a map choice */
    int dz_pad, dz_cs; /* PT61: dead zone in percent of full travel, circle pad and C-Stick (N3DS). 0 = as before (no port dead zone) */
    int rep_delay, rep_rate; /* PT60: menu key repeat in ms, the first step is at once, the next after rep_delay, then every rep_rate */
} PmOpt;
extern int pm_new3ds; /* 1 on a New 3DS, set in pm_main.c */
extern PmOpt pm_opt;
void pm_options_init(void);
void pm_options_write(void);
/* F11-3: the options panel (src/filemenu/filemenu_options.c). Rows 0 to 5 are fps60 wide hud stats nofog nolight, row 6 is Logging, row 7 is Log level, row 8 is Fixed seed. */
int pm_options_get(int row);
void pm_options_flip(int row);
void pm_options_close(void);
const unsigned char* pm_options_build_msg(void); /* BN1 footer text */
const unsigned char* pm_options_msg(const char* text, int slot); /* UI1a: text as game message, slot 0 to 7 */
/* UI1a, PT59: tab 0 General, 1 Controls, 2 Debug. mode 0 = file select panel, mode 1 = Select menu (port menu): the same rows, plus Return to Title and Exit to Homebrew */
int pm_optui_rows(int tab, int mode);
const char* pm_optui_label(int tab, int row, int mode);
const char* pm_optui_value(int tab, int row, int mode);
int pm_optui_step(int tab, int row, int dir, int mode); /* left or right on a row, 1 when the value changed */
int pm_optui_act(int tab, int row, int mode); /* 0 done, 1 close (Back), 2 leave the file select (Warp now), 3 exit to Homebrew, 4 return to title, 5 warp started */
void pm_optui_disarm(void);
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
extern int pm_warp_pending; /* UI1a: set by pm_warp_now, read by filemenu_get_exit_mode */
void pm_warp_now(int choice, int entry); /* UI1a, PT01: warp from the file select to a choice (preset or map) */
int pm_warp_map_count(void);
const char* pm_warp_map_name(int k);
int pm_warp_map_index(const char* name);
int pm_warp_is_test_map(const char* id); /* PT01b */
int pm_warp_choice_count(void);
const char* pm_warp_choice_text(int choice, int entry);
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

#endif
