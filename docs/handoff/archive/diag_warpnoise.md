# Diag WARPNOISE (2026-10-29)
Tags: [M] measured, [C] code, [E] estimate. No source edits. No Azahar starts.
## 0. Input [M]
- build3ds/hw_live_log_170.txt: 629 lines, ids #3099 to #3969. No build line. No #5989. One gap, #3173 to #3416 (244 ids). Not 3 gaps or 1397 ids.
- No map load in the file. The 17 "warp:" lines are scene heartbeats.
- build3ds/ftp_170/hw_o3ds_log.txt (pre-LOGCLEAN) has #5989 and one real warp, #44449 "kmr_11 entry 0".
## 1. Lines per warp [M]
- Window: 20 frames before to 300 after each anchor. 11 full live-log windows: 113 to 139 lines, mean 127.6, 12964 bytes.
- Real warp in ftp_170 (#44449): 2724 lines, 208612 bytes. Mostly per-frame split kinds, removed by LOGCLEAN [C].
## 2. Top 10 (mean per window: lines, bytes, source, rate)
- spike 72.5, 5144 B, pm_phase.c:41, frame over 50 ms.
- pace 15.6, 3221 B, pm_main.c:305, per 1 s.
- b7 10.9, 1353 B, pm_gbi.c:2318, per 30 frames.
- V09F5 6.8, 484 B, src/audio/bgm_player.c:1738, per BGM use-instrument command.
- warp scene 3.5, 459 B, pm_warp.c:741, per 100 scene frames.
- audio 2.3, 346 B, pm_audio.c:171 and pm_dspaud.c:488, per 300 kicks.
- logtop 2.2, 167 B, pm_log.c:279, per 1200 frames, 8 lines.
- logstat 2.0, 247 B, pm_log.c:264 and :266, per 300 frames.
- mem 1.4, 128 B, pm_mem.c and pm_main.c:523, per 3600 frames.
- dcsum 1.0, 204 B, pm_gpu.c:2308, per 300 frames.
- Also 1.0 each per 300 frames: dlc (pm_dlc.inc.c:533), dlc2 (:535), pdl (:1419), dr1 (pm_gpu.c:1839), gv1pal (pm_gbi.c:2334), gv1 (:2336), gp1 (:2339), gp3 (:2341), logstat2 (pm_log.c:268). [C]
## 3. What a warp logs [C]
- pm_warp.c:220 "warp: %s entry": once per warp. Marker of 9 or more scenarios.
- pm_warp.c:741 "warp: scene f%u": every 100 scene frames, only in a warp_on scene. Not in the default build.
- A warp logs 2 lines. The push line in CLAUDE.md has no warp switch. This session had one.
## 4. Use
- Keep (scenario markers): "warp: <map> entry N" (:220). "warp: scene f300 " (:741, kept by f % 300 == 0). "logstat frames 300 work ... wall 3x." (pm_log.c:266, yard_pace). "dlc2 f599 tri ... static grp" (pm_dlc.inc.c:535, mim_05).
- Noise: V09F5 (no docs, no tool). b7 (all zero here: grabs 0, splits 0, F03 0 x 0). [M]
- Spike near 300-frame boundaries: 11 of 349 (about 13 expected). No excess at report frames. [M]
## 5. Delete list (per window, before to after)
- Delete b7 print (pm_gbi.c:2318): 10.9 to 0.
- Delete V09F5 print (bgm_player.c:1738): 6.8 to 0.
- pm_warp.c:740 to f % 300 == 0 or f == 5: 3.5 to 1.1.
- Delete gv1pal, gv1, gp1, gp3 prints (pm_gbi.c:2334 to 2341): 4.0 to 0.
- Delete dr1, dlc, pdl prints (pm_gpu.c:1839, pm_dlc.inc.c:533 and :1419): 3.0 to 0.
- Delete logtop call (pm_main.c:532): 2.2 to 0.
- Delete duplicate logstat hw line (pm_log.c:264): 2.0 to 1.0.
- Subtotal: 127.6 to 97.4.
- Decision: remove spike (pm_phase.c:41): 72.5 to 0.
- Decision: pace 1 per 5 s (pm_main.c:303): 15.6 to 3.1.
- Total: 127.6 to about 12.5. Under 15 needs both decisions. Without spike: about 85.
## 6. Cost and gaps
- Writer: fbuf waits 0 and dropped 0 in every logstat line. The game thread did not wait. [M]
- Sink thread: file write max 99 to 437 ms, console 274 to 602 ms. Not the game thread. [M]
- Socket send max: 156161 us at #3483, 231682 us at #3618. [M]
- Gap: game total 3481 at #3482, game queue dropped 0. The loss is after the game queue, likely socket or PC side. [E]
- Frame: logstat work 49.3 to 49.7 ms per frame. Log 40 bytes per frame [M]. Formatting 0.05 to 0.1 ms per frame [E]. Load time: not measured.
