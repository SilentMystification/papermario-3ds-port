# Row BOTHUD: ON HOLD. The first cause is disproved. No producer found by code read: the trace below, a user question

(Not for a coder. Commit 47da274ce on branch BOTHUD holds only the probe (pm_hud_draws, hudchk) and the seed: keep the branch for
the probe, do not merge the sink timeout.)
## User facts (2026-11-12, console pushes)
After some pushes the bottom screen is empty, also when the settings are toggled. The HUD stays missing after a retoggle.
"Sometimes": a state that differs between pushes.
## What the seeded run proved (run_seed.txt, 47da274ce)
prof push form (prof=30, log off) on Azahar: `hudchk f300 draws 92`. The log sink wakes about 23 times per second also with
logging off: gate_changed (pm_log.c:376-380) signals ev_sink on every gate change. So "the sink never wakes with logging off"
(the first cause) is false. The sink timeout edit fixes nothing.
## Code read (ORACLE8, main a3ffbbe8b): what draws the bottom screen and what can blank it
- The HUD rows: pm_main.c:843 copies them when pm_opt.dbg_hud (the HUD option, live, pm_options.c:314-319). pm_gpu_hud
  (pm_gpu_ui.c:111) returns at once while menu_on. pm_gpu_hud_flush (pm_gpu_ui.c:156-162) draws only when the console exists
  and menu_on is 0. con_flush (pm_log.c:857) returns while con_up is 0 (set once by pm_log_console_ready, pm_nusys.c:31).
- The console is made once at GPU init (pm_gpu.c:1892 consoleInit(GFX_BOTTOM)). The HUD option and the levels are stored apart:
  options.ini keeps the saved levels (sv_*, pm_options.c:109-110, :225), boot values apply once (pm_options.c:229-241). No
  push value is written back. A HUD toggle reaches pm_opt.dbg_hud at once. So no persisted value blanks it: not proven by read.
- Paths that blank the screen and stay (each by code, not proven as the user's case):
  P1 menu_on stays 1: it goes to 0 only when the menu draw gets an empty text (pm_gpu_ui.c:61-73). A close that skips that draw
     leaves the HUD and the log off for good (pm_gpu_hud and the flush both return).
  P2 the console writes into the bottom frame buffer of libctru (single buffered by consoleInit). A later gfx call that turns
     double buffering on for the bottom screen, or a swap of it, shows the other buffer: empty. grep gfxSetDoubleBuffering and
     gfxScreenSwapBuffers(GFX_BOTTOM ...) in 3ds/src and libctru use is the next read.
  P3 after the HOME menu or sleep the system may overwrite the bottom frame buffer: the console redraws only changed rows.
## Question for the user (a fact no read gives)
When the bottom screen was empty, what happened just before: the push itself (first boot), the Select menu (open and close), the
HOME menu, sleep (lid), or a return to title? Was the bottom screen empty from the boot, or did it go empty later?
