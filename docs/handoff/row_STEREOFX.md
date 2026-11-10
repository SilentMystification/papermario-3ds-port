# Row STEREOFX: crashes with the 3D slider on (Watt in a dark room, Merlee talk), O3DS XL

(Queue: oracle first, then Haiku rows. One agent at a time.)

## User facts (2026-11-10, O3DS XL .131)
1. Shy Guy's Toy Box (omo), a dark room, Watt is the partner. The user moves the 3D slider up from 0. The game crashes at the
   slider move (user answer).
2. The user talks to Merlee. The game crashes right before the price screen shows (user answer). 3D was on. The user does not know if 3D is part of the cause.
XL build: the last push to .131 is 8085ef1d0 (build 3700, 2026-11-09 17:23). It has LINRACE. It does not have CRASHALL (crash page),
PAUSEOPEN or later merges. XL FTP pull at 2026-11-10: no connection (no new dump or log). Push of main for the crash page: user said not now.

## Common factor (orchestrator, to check)
Both scenes draw a full-screen darken or light effect: the Watt light in a dark room, the Merlee spell darken
(npc_merlee.c card_setup_gfx, merlee_spell). STEREO S2 (99014ae5e): the right eye replays the recorded GPU queue of the left
pass (fills, copies, command lists). Suspect: a full-screen effect entry (fill, copy or a buffer sized for one eye) that the
replay or the slider change at run time (mono to stereo switch with the effect live) handles wrong.

## Step 1 (oracle)
Trace the slider-change path (mono to stereo while a frame draws) and the replay of full-screen effect entries for both scenes.
Name the code point and the value, and one Azahar check (stereo forced, the scene, the slider change). Then split into Haiku rows.

## Similar crashes to check (same style)
Other full-screen effects with 3D on: Star Spirit cutscenes, Star powers (Up and Away dim), pause open, Return to Title.
