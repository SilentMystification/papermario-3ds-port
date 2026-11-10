# Row PAUSEFADE: the wide pause gutters are not faded like the centre (open, from PAUSEOPEN)

The centre of the pause picture fades by PM_CC_43 (prim alpha = gPauseBackgroundFade, SHOW_PAUSED in background_gfx.c).
The gutter strips (pm_gpu_pause_gutters, pm_gpu.c) are drawn in class 2 (TEXEL0 x SHADE), white vertex colour, so they show the
scene at full brightness. The class 8 plan path (the walker combine at the payload) drops the texel: flat (40,40,40).
Next: give the strips the centre's fade (prim alpha) in a class that keeps the texel; gate: strip mean darkens like the centre edge.
Run 189453 (pause_gut_dbg_w300): strips show the scene, not flat. Debug lines GUTDBG and GUTW under the gutdbg word.
