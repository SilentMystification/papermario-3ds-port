# Row AUDDROP: sounds and voices missing on the console (any type)

## User facts
User (2026-11-09): sound effects and a voice go missing, consistently, on many occasions: message text blips, Mario's voice, battle
sounds, field sounds; "Most if not all things might fall victim." Earlier AUDV (git log --grep AUDV) showed census 0 on Azahar only.

## Start state
Worktree build3ds/wt/AUDDROP, branch AUDDROP from main (hash in the first commit). PM_ROW=AUDDROP, PM_LANE=AUDDROP.

## Causes (oracle, ranked; the census decides)
1. Render catch-up burst: pm_dspaud.c:447-451 pm_dspa_pace runs up to 6 blocks (34.5 ms) back to back when late. PM_AU_LEAD 1416
   (pm_audio.c:13). Steps over about 50 ms (O3DS) drain the render; the next job's first blocks burst. Starts land in tick 1
   (syn_driver.c:355 -> engine.c:430); a stop in the same burst (voice.c:4-17 -> dsp_voice.c:142 ndspChnWaveBufClear) clears the wave
   buffer before the DSP plays it (under 1 ndsp frame, about 4.9 ms).
2. Same ndsp channel stopped and restarted in one ndsp frame: dsp_voice.c:200 ctz lowest-free reuses a channel freed this pass;
   retrigger 213-215 + pm_dspaud.c:176; stale status -> pm_dspa_voice_ended (219-227) ends the voice silent.
3. Sample cache: pm_dspaud.c:88 malloc(N*2) temp PCM fails on the O3DS for long samples; 106-107 linearMemAlign failure drops without
   evict; 100-101 guard; give-up after 16 retries (dsp_voice.c:223).
4. Zero-tick steps merge requests (pm_main.c:298, pm_audio.c:196); sfx queue wrap sfx_player.c:598 (pending mod 16, the 16th request
   loses the whole queue) and start+stop merged in one batch (690-714).
5. Post-sleep bounded join pm_audio.c:236-240 (au_busy=0 while the job runs) races sfxQueueWritePos/ReadPos. Only after sleep.
6. Type-specific (low): loop cap pm_dspaud.c:193, non-ADPCM dsp_voice.c:146, bus>=2 dry 181, fx_dropped 309, no rom data pm_os.c:413.

## Plan
A. Census (switch audiostat, cold noinline, event paths only, about 1.6 KB .bss, no heap): voice tag au_vowner[24] (client<<16|id) set
   where sfx/bgm/ambience claim a voice, copied at engine.c:492 into au_vtag[24][16] by au2 vgen (accessor in syn_driver.c). Render
   reasons per DspV (t_start, b_start): UNHEARD (stop/restart while wb[0].status == NDSP_WBUF_QUEUED, pm_dspa_voice_heard),
   SQUEEZED (stop wall_us < blocks*5750/2), EARLYEND (end under half the expected length), NOCHAN, GIVEUP+sub, LOOPCAP, NONADPCM.
   Control reasons: QOVF (sfx_player.c:598), MERGED (start+stop same batch), NOPLAYER, SFXOFF, STOLEN (engine.c:529; report only).
   Pace: catchup blocks and max run, resyncs, max late us, decode max us, zero-tick steps. Storage: SPSC ring 64 {tag, reason, sub, ms}
   (render), 32-entry table (control). Output at pm_log level each window:
   `audio drops wN: unheard a squeezed b early c nochan d giveup e loopcap f nonadpcm g qovf h merged i | engine noplayer j stolen k |
   catchup blocks l maxrun m late_us_max n decode_max_us o zero_tick_steps p | ids: <top 12 id reason count>`, plus the first 48
   (id, reason) pairs as single lines with pm_frame_n and the map name. Debug switch aulag=<ms> (logic step sleep) to reproduce O3DS
   step lengths on Azahar. Scenario+bench aud_talk (preset=toad pos=-66:20:-518:0 padscript=wait300,A,wait150,A,wait150 audiostat,
   like talk_off; until `audio drops w2:`) and aud_*_lag variants (aulag=30). Commit the census alone first.
B. Cause 1: pm_dspa_pace: `if (!next_due || now >= next_due) { if (next_due && now > next_due + BLK_TICKS/4) st_late++; next_due = now; }
   else svcSleepThread(...); next_due += BLK_TICKS;` Safety valve: au2_blk_backlog() > 24 runs without sleep, counted SQUEEZED_CAP.
   Delete the 6-block rule.
C. Cause 2 (only if UNHEARD/EARLYEND with catchup 0): ch_freed_now mask per au_dsp_block, pick ctz(fr & ~ch_freed_now) with a rotating
   cursor; a retrigger moves to another free channel.
D. Cause 3 (if GIVEUP heap/linear): allocate linear first with evict-and-retry, decode straight into it; delete the temp heap buffer.
E. Cause 4 (if MERGED/QOVF): guard `pending < SFX_QUEUE_SIZE - 1`; n == 0 with a non-empty sfx queue uses n = 1 with a debt counter.
F. Cause 5: only if the user reports drops after sleep.

## Done criteria
nd_host PASS. memcheck margin >= 256 KB. Azahar audiostat: aud_mac01, aud_battle, aud_intro, aud_talk: all port reasons 0 in w2.
Before/after on aud_talk_lag and aud_mac01_lag (aulag=30): before B squeezed+unheard > 0 and catchup > 0, after 0. Shots show the subject
(speech box, map). Console bench for the user: push o and push n with audiostat, normal play, pull logs; pass = port reasons 0.
Affected scenarios (no others): the aud_* ones.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles (old pace rule deleted), no stubs. Commit after each passed step. Never commit
3ds/src/map_restore_table.c. Padscripts staged in build3ds/azslots/slot*/user/sdmc/3ds/PaperMario/.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "AUDDROP ready <hash>".
