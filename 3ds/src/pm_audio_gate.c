/* CORE2-1: audio one step ahead. The audio job of a step (kicked at the main nuGfxTaskStart, pm_audio.c) runs on core 1 until the game
 * thread touches the sound engine again, not only until the next step begins. The only way game code reaches the engine state is the
 * snd_* interface of src/audio/snd_interface.c (bgm_control.c, sfx_control.c, ambience.c, evt audio_api.c and the map scripts call
 * it; no game file reads gSoundManager, gBGMPlayer*, gAuAmbienceManager or gSoundGlobals: docs/handoff/plan_core2.md). Each snd_*
 * function is wrapped by the linker (3ds/CMakeLists.txt, --wrap) and joins the job first (pm_audio_gate). So every engine read or write
 * of the game sees the engine after the whole job, as with the join at the step start: same order, same state, the job only overlaps
 * game code that does not touch the engine. Calls inside snd_interface.c are not wrapped (one object); calls from the audio thread
 * itself pass (pm_audio_gate is a no-op there). Generated from snd_interface.c: a new non-static snd_* function needs a line here and
 * its --wrap flag. */
#include "common.h"
#include "audio.h"
#include "pm_port.h"

void pm_audio_gate(void);

void __real_snd_notify_engine_ready(ALHeap* heap);
void __wrap_snd_notify_engine_ready(ALHeap* heap) { pm_audio_gate(); __real_snd_notify_engine_ready(heap); }
void __real_snd_legacy_sound_dispatch(u32 id);
void __wrap_snd_legacy_sound_dispatch(u32 id) { pm_audio_gate(); __real_snd_legacy_sound_dispatch(id); }
void __real_snd_bgm_clear_legacy_commands(BGMPlayer* player);
void __wrap_snd_bgm_clear_legacy_commands(BGMPlayer* player) { pm_audio_gate(); __real_snd_bgm_clear_legacy_commands(player); }
void __real_snd_bgm_enqueue_legacy_command(u32 cmd);
void __wrap_snd_bgm_enqueue_legacy_command(u32 cmd) { pm_audio_gate(); __real_snd_bgm_enqueue_legacy_command(cmd); }
void __real_snd_start_sound(s32 soundID, u8 volume, u8 pan);
void __wrap_snd_start_sound(s32 soundID, u8 volume, u8 pan) { pm_audio_gate(); __real_snd_start_sound(soundID, volume, pan); }
void __real_snd_start_sound_with_shift(s32 soundID, u8 volume, u8 pan, s16 pitchShift);
void __wrap_snd_start_sound_with_shift(s32 soundID, u8 volume, u8 pan, s16 pitchShift) { pm_audio_gate(); __real_snd_start_sound_with_shift(soundID, volume, pan, pitchShift); }
void __real_snd_adjust_sound(s32 soundID, u8 volume, u8 pan);
void __wrap_snd_adjust_sound(s32 soundID, u8 volume, u8 pan) { pm_audio_gate(); __real_snd_adjust_sound(soundID, volume, pan); }
void __real_snd_adjust_sound_with_shift(s32 soundID, u8 volume, u8 pan, s16 pitchShift);
void __wrap_snd_adjust_sound_with_shift(s32 soundID, u8 volume, u8 pan, s16 pitchShift) { pm_audio_gate(); __real_snd_adjust_sound_with_shift(soundID, volume, pan, pitchShift); }
void __real_snd_stop_sound(s32 soundID);
void __wrap_snd_stop_sound(s32 soundID) { pm_audio_gate(); __real_snd_stop_sound(soundID); }
void __real_snd_reset(void);
void __wrap_snd_reset(void) { pm_audio_gate(); __real_snd_reset(); }
void __real_snd_start_sound_raw(s32 soundID, s16 volume, s16 pitchShift, s32 pan);
void __wrap_snd_start_sound_raw(s32 soundID, s16 volume, s16 pitchShift, s32 pan) { pm_audio_gate(); __real_snd_start_sound_raw(soundID, volume, pitchShift, pan); }
AuResult __real_snd_load_ambient(s32 ambSoundID);
AuResult __wrap_snd_load_ambient(s32 ambSoundID) { pm_audio_gate(); return __real_snd_load_ambient(ambSoundID); }
AuResult __real_snd_ambient_play(s32 index, s32 fadeInTime);
AuResult __wrap_snd_ambient_play(s32 index, s32 fadeInTime) { pm_audio_gate(); return __real_snd_ambient_play(index, fadeInTime); }
AuResult __real_snd_ambient_stop_quick(s32 index);
AuResult __wrap_snd_ambient_stop_quick(s32 index) { pm_audio_gate(); return __real_snd_ambient_stop_quick(index); }
AuResult __real_snd_ambient_stop_slow(s32 index, s32 fadeOutTime);
AuResult __wrap_snd_ambient_stop_slow(s32 index, s32 fadeOutTime) { pm_audio_gate(); return __real_snd_ambient_stop_slow(index, fadeOutTime); }
AuResult __real_snd_ambient_pause(s32 index, s32 fadeOutTime);
AuResult __wrap_snd_ambient_pause(s32 index, s32 fadeOutTime) { pm_audio_gate(); return __real_snd_ambient_pause(index, fadeOutTime); }
AuResult __real_snd_ambient_resume(s32 index, s32 fadeInTime);
AuResult __wrap_snd_ambient_resume(s32 index, s32 fadeInTime) { pm_audio_gate(); return __real_snd_ambient_resume(index, fadeInTime); }
AuResult __real_snd_ambient_is_stopped(s32 index);
AuResult __wrap_snd_ambient_is_stopped(s32 index) { pm_audio_gate(); return __real_snd_ambient_is_stopped(index); }
AuResult __real_snd_ambient_fade_out(s32 index, s32 arg1);
AuResult __wrap_snd_ambient_fade_out(s32 index, s32 arg1) { pm_audio_gate(); return __real_snd_ambient_fade_out(index, arg1); }
AuResult __real_snd_ambient_set_volume(s32 index, s32 time, s32 volume);
AuResult __wrap_snd_ambient_set_volume(s32 index, s32 time, s32 volume) { pm_audio_gate(); return __real_snd_ambient_set_volume(index, time, volume); }
AuResult __real_snd_ambient_disable(s32 index);
AuResult __wrap_snd_ambient_disable(s32 index) { pm_audio_gate(); return __real_snd_ambient_disable(index); }
AuResult __real_snd_ambient_enable(s32 index);
AuResult __wrap_snd_ambient_enable(s32 index) { pm_audio_gate(); return __real_snd_ambient_enable(index); }
void __real_snd_ambient_radio_setup(s32 index);
void __wrap_snd_ambient_radio_setup(s32 index) { pm_audio_gate(); __real_snd_ambient_radio_setup(index); }
AuResult __real_snd_ambient_radio_stop(s32 time);
AuResult __wrap_snd_ambient_radio_stop(s32 time) { pm_audio_gate(); return __real_snd_ambient_radio_stop(time); }
AuResult __real_snd_ambient_radio_select(s32 index);
AuResult __wrap_snd_ambient_radio_select(s32 index) { pm_audio_gate(); return __real_snd_ambient_radio_select(index); }
AuResult __real_snd_song_load(s32 songID, s32 playerIndex);
AuResult __wrap_snd_song_load(s32 songID, s32 playerIndex) { /* BTLWIPE: the song stage of the xfer line (gate wait and load) */
    unsigned t0 = pm_xf.on ? pm_ticks() : 0u;
    pm_audio_gate();
    AuResult r = __real_snd_song_load(songID, playerIndex);
    if (pm_xf.on) pm_xf.song_t += pm_ticks() - t0;
    return r;
}
AuResult __real_snd_song_request_play_default(s32 songName);
AuResult __wrap_snd_song_request_play_default(s32 songName) { pm_audio_gate(); return __real_snd_song_request_play_default(songName); }
AuResult __real_snd_song_request_play(s32 songName, s32 variation);
AuResult __wrap_snd_song_request_play(s32 songName, s32 variation) { pm_audio_gate(); return __real_snd_song_request_play(songName, variation); }
AuResult __real_snd_song_stop(s32 songName);
AuResult __wrap_snd_song_stop(s32 songName) { pm_audio_gate(); return __real_snd_song_stop(songName); }
void __real_snd_song_stop_all(void);
void __wrap_snd_song_stop_all(void) { pm_audio_gate(); __real_snd_song_stop_all(); }
AuResult __real_snd_song_is_playing(s32 songName);
AuResult __wrap_snd_song_is_playing(s32 songName) { pm_audio_gate(); return __real_snd_song_is_playing(songName); }
AuResult __real_snd_song_request_fade_in(s32 songName, s32 variation, s32 fadeInTime, s32 startVolume, s32 endVolume);
AuResult __wrap_snd_song_request_fade_in(s32 songName, s32 variation, s32 fadeInTime, s32 startVolume, s32 endVolume) { pm_audio_gate(); return __real_snd_song_request_fade_in(songName, variation, fadeInTime, startVolume, endVolume); }
AuResult __real_snd_song_request_fade_in_default(s32 songName, s32 fadeInTime, s32 startVolume, s32 endVolume);
AuResult __wrap_snd_song_request_fade_in_default(s32 songName, s32 fadeInTime, s32 startVolume, s32 endVolume) { pm_audio_gate(); return __real_snd_song_request_fade_in_default(songName, fadeInTime, startVolume, endVolume); }
AuResult __real_snd_song_request_fade_out(s32 songName, s32 fadeTime, AuCallback callback);
AuResult __wrap_snd_song_request_fade_out(s32 songName, s32 fadeTime, AuCallback callback) { pm_audio_gate(); return __real_snd_song_request_fade_out(songName, fadeTime, callback); }
AuResult __real_snd_song_push_stop(s32 songName);
AuResult __wrap_snd_song_push_stop(s32 songName) { pm_audio_gate(); return __real_snd_song_push_stop(songName); }
AuResult __real_snd_song_request_pop(s32 songName);
AuResult __wrap_snd_song_request_pop(s32 songName) { pm_audio_gate(); return __real_snd_song_request_pop(songName); }
AuResult __real_snd_song_request_snapshot(s32 songName);
AuResult __wrap_snd_song_request_snapshot(s32 songName) { pm_audio_gate(); return __real_snd_song_request_snapshot(songName); }
AuResult __real_snd_song_request_push_fade_out(s32 songName, s32 fadeTime);
AuResult __wrap_snd_song_request_push_fade_out(s32 songName, s32 fadeTime) { pm_audio_gate(); return __real_snd_song_request_push_fade_out(songName, fadeTime); }
AuResult __real_snd_song_request_pause(s32 songName);
AuResult __wrap_snd_song_request_pause(s32 songName) { pm_audio_gate(); return __real_snd_song_request_pause(songName); }
AuResult __real_snd_song_request_unpause(s32 songName);
AuResult __wrap_snd_song_request_unpause(s32 songName) { pm_audio_gate(); return __real_snd_song_request_unpause(songName); }
AuResult __real_snd_song_set_volume_quiet(s32 songName);
AuResult __wrap_snd_song_set_volume_quiet(s32 songName) { pm_audio_gate(); return __real_snd_song_set_volume_quiet(songName); }
AuResult __real_snd_song_set_volume_full(s32 songName);
AuResult __wrap_snd_song_set_volume_full(s32 songName) { pm_audio_gate(); return __real_snd_song_set_volume_full(songName); }
AuResult __real_snd_song_set_linked_mode(s32 songName, bool mode);
AuResult __wrap_snd_song_set_linked_mode(s32 songName, bool mode) { pm_audio_gate(); return __real_snd_song_set_linked_mode(songName, mode); }
AuResult __real_snd_song_get_playing_info(s32 songName, BGMHeader** outFile, BGMPlayer** outPlayer);
AuResult __wrap_snd_song_get_playing_info(s32 songName, BGMHeader** outFile, BGMPlayer** outPlayer) { pm_audio_gate(); return __real_snd_song_get_playing_info(songName, outFile, outPlayer); }
AuResult __real_snd_song_set_playback_rate(s32 songName, f32 rate);
AuResult __wrap_snd_song_set_playback_rate(s32 songName, f32 rate) { pm_audio_gate(); return __real_snd_song_set_playback_rate(songName, rate); }
AuResult __real_snd_song_set_detune(s32 songName, s32 detune);
AuResult __wrap_snd_song_set_detune(s32 songName, s32 detune) { pm_audio_gate(); return __real_snd_song_set_detune(songName, detune); }
AuResult __real_snd_song_set_track_volumes(s32 songName, MusicTrackVols trackVolSet);
AuResult __wrap_snd_song_set_track_volumes(s32 songName, MusicTrackVols trackVolSet) { pm_audio_gate(); return __real_snd_song_set_track_volumes(songName, trackVolSet); }
AuResult __real_snd_song_clear_track_volumes(s32 songName, MusicTrackVols trackVolSet);
AuResult __wrap_snd_song_clear_track_volumes(s32 songName, MusicTrackVols trackVolSet) { pm_audio_gate(); return __real_snd_song_clear_track_volumes(songName, trackVolSet); }
AuResult __real_snd_song_set_track_vol_mute(s32 songName, s32 trackIdx);
AuResult __wrap_snd_song_set_track_vol_mute(s32 songName, s32 trackIdx) { pm_audio_gate(); return __real_snd_song_set_track_vol_mute(songName, trackIdx); }
AuResult __real_snd_song_set_track_vol_quiet(s32 songName, s32 trackIdx);
AuResult __wrap_snd_song_set_track_vol_quiet(s32 songName, s32 trackIdx) { pm_audio_gate(); return __real_snd_song_set_track_vol_quiet(songName, trackIdx); }
AuResult __real_snd_song_set_track_vol_full(s32 songName, s32 trackIdx);
AuResult __wrap_snd_song_set_track_vol_full(s32 songName, s32 trackIdx) { pm_audio_gate(); return __real_snd_song_set_track_vol_full(songName, trackIdx); }
void __real_snd_song_set_proximity_mix_far(s32 songName, s32 mix);
void __wrap_snd_song_set_proximity_mix_far(s32 songName, s32 mix) { pm_audio_gate(); __real_snd_song_set_proximity_mix_far(songName, mix); }
void __real_snd_song_set_proximity_mix_near(s32 songName, s32 mix);
void __wrap_snd_song_set_proximity_mix_near(s32 songName, s32 mix) { pm_audio_gate(); __real_snd_song_set_proximity_mix_near(songName, mix); }
void __real_snd_song_set_proximity_mix_full(s32 songName, s32 mix);
void __wrap_snd_song_set_proximity_mix_full(s32 songName, s32 mix) { pm_audio_gate(); __real_snd_song_set_proximity_mix_full(songName, mix); }
void __real_snd_song_poll_music_events(MusicEventTrigger** musicEvents, s32* count);
void __wrap_snd_song_poll_music_events(MusicEventTrigger** musicEvents, s32* count) { pm_audio_gate(); __real_snd_song_poll_music_events(musicEvents, count); }
void __real_snd_song_flush_music_events(void);
void __wrap_snd_song_flush_music_events(void) { pm_audio_gate(); __real_snd_song_flush_music_events(); }
void __real_snd_song_trigger_music_event(s32 playerID, s32 trackIndex, s32 eventInfo);
void __wrap_snd_song_trigger_music_event(s32 playerID, s32 trackIndex, s32 eventInfo) { pm_audio_gate(); __real_snd_song_trigger_music_event(playerID, trackIndex, eventInfo); }
void __real_snd_song_clear_music_events(void);
void __wrap_snd_song_clear_music_events(void) { pm_audio_gate(); __real_snd_song_clear_music_events(); }
void __real_snd_register_callback(AuCallback func, s32 index);
void __wrap_snd_register_callback(AuCallback func, s32 index) { pm_audio_gate(); __real_snd_register_callback(func, index); }
void __real_snd_set_stereo(void);
void __wrap_snd_set_stereo(void) { pm_audio_gate(); __real_snd_set_stereo(); }
void __real_snd_set_mono(void);
void __wrap_snd_set_mono(void) { pm_audio_gate(); __real_snd_set_mono(); }
void __real_snd_set_bgm_volume(VolumeLevels volume);
void __wrap_snd_set_bgm_volume(VolumeLevels volume) { pm_audio_gate(); __real_snd_set_bgm_volume(volume); }
void __real_snd_set_sfx_volume(VolumeLevels volume);
void __wrap_snd_set_sfx_volume(VolumeLevels volume) { pm_audio_gate(); __real_snd_set_sfx_volume(volume); }
void __real_snd_set_sfx_reverb_type(s32 reverbType);
void __wrap_snd_set_sfx_reverb_type(s32 reverbType) { pm_audio_gate(); __real_snd_set_sfx_reverb_type(reverbType); }
void __real_snd_enable_sfx(void);
void __wrap_snd_enable_sfx(void) { pm_audio_gate(); __real_snd_enable_sfx(); }
void __real_snd_disable_sfx(void);
void __wrap_snd_disable_sfx(void) { pm_audio_gate(); __real_snd_disable_sfx(); }
