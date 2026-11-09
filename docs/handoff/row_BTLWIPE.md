# Row BTLWIPE: hitch while the battle star wipe closes

## User facts
User (2026-11-09, O3DS XL): "Entering into battle also hitches sometimes as the star transition goes in."

## Start state
Worktree build3ds/wt/BTLWIPE, branch BTLWIPE from main (hash in the first commit). PM_ROW=BTLWIPE, PM_LANE=BTLWIPE. One Opus agent.

## Cause (oracle, code trace)
pm_xfer does not cover battle entry (pm_map.c:592, rooms.c:230; pm_xfer_skip XF_MAP only, pm_xfer.c:112). About step 11 of the wipe
(star about 70%), bgm_update_music_control (main_loop.c:110, logic thread) reaches PLAY_NEXT: snd_song_load (bgm_control.c:148) ->
au_load_song_files (engine.c:710): BGM read (:744), up to 3 aux banks via au_load_BK_to_bank (engine.c:1132): pm_au2_sync(AU2_SYNC_BANK)
(:1134, waits for every committed render job: pm_au2.c:146-152, paced 5.75 ms per block) plus 8 KB nuPiReadRom chunks (au_read_rom
engine.c:1423, each drains the walk on the logic thread and posts pm_tex_invalidate). Steps 12-14: new voices -> cache_get
(pm_dspaud.c:75-140) VADPCM decode on the render thread, which preempts the logic thread on core 1 (O3DS). Sometimes: cold decode cache
(first battle), backlog phase, no song push in some encounters, N3DS logic on core 2. The wipe capture stays on the GPU (not the cause).

## Plan (one commit each, measure before the next)
F1 measure: XF_BTL kind (begin at PRE_BATTLE_INIT, encounter.c:1275; name <map>->btl:<id>; load step from a wrap of load_battle_section;
   end 30 steps after the first drawn battle step) plus `xfer3 battle wmax <ms> wo34 <n> song <ms> bks <n>/<ms> dec <n>/<ms>/<maxms>
   dr <n>/<ms> capcpu <n>`, cold. Benches+scenarios: xfer_battle_enc (Azahar, warp=kmr_09:0 story=ch0_toad_town encwarp=120 loglevel=debug,
   until `xfer battle \S+`, cap 30 s), console xfer_battle_prof (prof=30) and xfer_battle_walk_prof (padscript=right*200).
F2 audio reads from the resident image: au_read_rom (TARGET_3DS) memcpy from pm_rom_ptr, fallback nuPiReadRom; confirm no swap16 range or
   the asset table overlaps SBN/BK.
F3 narrow the aux bank sync: track per voice the aux bank of its Instrument in au2_put; BANK_SET_AUX waits only for that bank's last job;
   full sync for other bank sets and when a voice on that bank is still active; audit render-side Instrument reads first.
F4 pre-decode on the core-1 prefetch thread (pm_prefetch.c, prio 0x3F) at PRE_BATTLE_INIT: parse the battle song's aux BKs from
   pm_rom_ptr into heap scratch, swap, decode unique samples into the DSP cache (shared decode body with cache_get, insert under a lock,
   decode outside it, no eviction of entries used in the last 2 s, never nuPiReadRom on that thread).
F5 battle stage prefetch (MAPX-4 pattern): post stage shape and hit at PRE_BATTLE_INIT; decode_yay0 take serves it.
F6 battle warm frames: pm_xfer_skip accepts XF_BTL after the load (at most 64 model misses per covered frame).
Not here: per-read drains in the covered battle load (report dr; plan_core2.md 3b).
hw_guidelines; cold counters; no per-frame malloc; memcheck margin >= 256 KB; no toggles; no stubs.

## Done criteria
nd_host PASS; battle_start_f182_cold byte-equal across two cold runs and vs base; btrans_stand_enc/btrans_walk_enc caplog cpu 0,
cpu-after-split 0, xfer battle line; aud_battle det audiohash near-equal; memcheck OK; xfer_battle_enc before/after (song, bks, dr down,
wmax not up). Console (user): xfer_battle_prof and xfer_battle_walk_prof on the O3DS: wo34 0, wmax < 34 ms; 5 walk-in encounters.

## Rules
CLAUDE.md rules 5, 7, 8, 9, 11. No git stash, no push, no attribution. Never commit 3ds/src/map_restore_table.c. Merge main before each step.

## At the end
Delete this row file in the completing commit. SendMessage main "BTLWIPE ready <hash>".
