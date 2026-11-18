#!/bin/sh
# BOTSCR: the HUD, the footer and the menu close wake the log sink (pm_log_sink_kick), so the bottom screen draws with logging off.
# Prints `botscr ok`. Seeded fault: pm_gpu_ui.c without the HUD kick (the line after `hud_new = 1;`) must give 2 kick calls.
n=$(grep -c 'pm_log_sink_kick();' 3ds/src/pm_gpu_ui.c)
[ "$n" = 3 ] || { echo "botscr FAIL kick count $n"; exit 1; }
grep -q 'void pm_log_sink_kick(void) { if (con_up) LightEvent_Signal(&ev_sink); }' 3ds/src/pm_log.c || { echo "botscr FAIL def"; exit 1; }
m=$(sed '/^    hud_new = 1;/{n;/pm_log_sink_kick/d}' 3ds/src/pm_gpu_ui.c | grep -c 'pm_log_sink_kick();')
[ "$m" = 2 ] || { echo "botscr seeded fault not seen"; exit 1; }
echo "botscr ok"
