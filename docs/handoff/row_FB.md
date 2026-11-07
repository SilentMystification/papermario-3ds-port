# Row FB: the last counted fallbacks go native, then the CPU batch path is deleted

plan_walker_end.md D1: done = wd 0 AND the CPU paths deleted. RECT-3/PM5-4 (branch del) found every CPU branch still reached through a counted fallback (agent_s6.md RECT line). This row makes them native, then deletes the CPU path.

## Oracle plan FALLBACK
Rule: a CPU branch is deleted when its caller has a native form, or when the caller is only a debug switch (the switch is deleted with it). No fallback survives as a counted CPU path. Per caller:
- Debug or capability switches, deleted together with their branch: nozt (pm_gpu.c nozt_on is only `pm_debug_has("nozt")`), norectraw (rr_why 6), pm5off and gv5 off, notex1nat, noraw, nogrange. gpu_ok, gpu_vtx_on and gv4_on are false only when C3D or shader init failed; then nothing draws, so the CPU path is not needed either: the frame shows the bottom-screen error.
- Ring full (rr_why 5, rect_raw_ok pm_gpu.c:3831; the same test in gtrip and grange): add a cold `ring_wrap()`. It closes the open batch, makes a C3D_FrameSplit, waits for that GPU part (the pm_gpu_begin_wait form), then resets gring_n and gidx_n to 0 and continues natively. Size GRING_VTX and GRING_IDX from the logged high mark (rect ring hi, gv high) times 2, so the wrap stays cold.
- sc above 127 (rr_why 7, tex_raw_rect): store s - s_base and t - t_base in the shorts, with uvso.zw = base. The span then only has to fit 127 normalized units. If the span itself exceeds it, split the rect along that axis into equal parts (the same density). Both stay native.
- zmark rects (rr_why 4) and zmark/raw (nraw4): the two-pass template that TRI-1 added to nd_raw_native (pass 1 marks, pass 3 writes depth). rect_raw_ok drops its r_zmark refusal, and the rect close goes through the same nd_raw_native two-pass branch.
- grab (rr_why 3, plus the grab_impl triangles at pm_gpu.c ~2398): the grab quad becomes a native raw batch. Four vertices go into the ring with the rotated uv; unit 0 is grab_tex (the OFF-2 grab_native_unit0 form); RECT_GM palette; combine 3, replace. grab_bind then only marks the unit and never refuses.
- pm5 with tex1 (nraw6 pm5) and pm5 with noise: extend pm5.v.pica with the tc1 output (`mul/add uvx` exactly as pm4) and the uvx uniform at a free register. This amends D7: the same program layout plus one output. Noise for pm5 takes the same unit 2 path as below.
- Noise unit 2 (nraw7): nd_emit writes the unit 2 block (the fixed noise texture, address and type registers 0xA0+) and the nzo uniform. NdTmplKey gets noise (the tev for the noise source exists in pm_comb). Drop noise_bound.
- aux off-screen unit 1 (nraw6 aux): pm_tex_native_unit1 returns the off-screen slot's image with the OFF-3 snapshot rule, as unit 0 does.
- cmdbuf full (nraw1): a forced split (kick_check's split, without the draw threshold) when the remaining command buffer is below ND_EMIT_MAX + 1 KB, then emit natively.
- Template refusal (nraw9): nd_tmpl_build returns -1 when the table is full (`tmpl_n >= ND_TMPL_N`, nd_tmpl.c:388). Make the table an LRU with eviction (it is rebuilt from the key on a miss) and log the high mark. A pm_comb_build failure keeps the refusal, but the game never sets combine 8 for a plan that does not build, so it is unreachable: count it, and treat it as a hard error.
- flush_draw_cpu's 7 callers (non-raw vertex batches): TRI-4 owns them. Sprite class 5 (spr_build raw refusal): MDL-3 owns it. Once both have landed, every refusal reason in them is one of the native forms above.
- WIDE partly-edge ranges, ent_dynlist/dynvtx, reflect, USE_IMAGE: these run walker words that draw through the ring and close natively. They are walker interpretation (an S10 matter), not CPU batch users. Remove them from the RECT line list.
Rows (each gate: the counters 0 with a forcing switch where the default run never hits the case; byte-equal against the pre-row build on the forced scene; mac_01 byte-equal):
FB-1 RECT/GRAB: ring_wrap, the uvso base and split for sc, zmark rects through the two-pass close, the grab quad native.
- Forcing switches: `ringsmall=<n>` (a tiny ring), `scforce` (adds 200 to s in a sky rect plus the base, which must give the same picture), and pause_spin_grab for grab.
- Gate: rr_why 3/4/5/7 = 0; shots byte-equal on mac_00 sky, pause_spin_grab, and a zmark scene (battle_start).
FB-2 OFF/NOISE/TMPL: unit 2 noise in nd_emit plus the key; aux off-screen unit 1; forced cmdbuf split; template LRU.
- Forcing switches: `tmplsmall=<n>` and `cmdsmall`.
- Scenes: nok11_splash (noise), intro_native (aux off), a forced cmdbuf scene.
- Gate: nraw 1/6/7/9 = 0; byte-equal to the pre-row build.
FB-3 PM5: pm5.v.pica tc1 output plus uvx, the noise unit for pm5, refusals removed.
- Forcing switch: `pm5all` (every lit model goes through pm5).
- Gate: pm5 tex1 and noise counts 0; battle_start and the fog/lit maps byte-equal to the pre-row build.
FB-4 DELETE (after FB-1..3, TRI-4, MDL-3; same commit as GM-3's CPU-only geometry deletes, or right after it):
- Delete flush_draw_cpu, put4, tri_raw, pm_gpu_tri, pm_gpu_tri3d, pm_gpu_vert_clip, the Vert batch, vert_base and its linearAlloc.
- Delete the g_use5 citro3d block of flush_draw with sh5_*, u5_* and prog5's CPU uniform writes (prog5 stays for the native draw), and sh_*/sh_ok, tev_same and nd_leave_native (D6).
- Delete the debug switches named above.
- Proof: `grep -nE "flush_draw_cpu|C3D_DrawArrays|C3D_DrawElements|vert_base|put4\(" 3ds/src` is empty.
- Gate: wd 0 in all CEN scenes; mac_01, battle_start, pause and iwa_10 byte-equal to the pre-row build; linear_kb lower by the vert_base size (the boot_mem bench).

## Oracle plan FB-1 ring
Most likely cause: the short encoding of s,t now depends on the batch. The new base form (0539f47cc) stores round((s - s_base) * sc), with s_base taken per batch, from the batch's first rect. A rect merged into a batch uses that rect's base; the same rect in its own batch (ringsmall=64, or a flush per rect) uses its own base. When s_base is not a multiple of 1/sc, round((s - base) * sc) + base*sc ≠ round(s * sc), so the encoded coordinate moves by up to half a step. The sky is point-sampled (G_TF_POINT, unit param nearest), so a texel boundary near a sample flips to the neighbour texel. That gives a thin strip of small differences (max 13 in a smooth sky gradient), and it is stable and repeatable: the strip you see. The other candidates do not change the encoded vertices: grab rows (the grab quad was refuted), depth or blend (COPY / NOOP mode, z 0), shared-edge rasterisation (the same triangles give the same pixels whatever the draw call).
One measurement: frame N of fb_sky_mac00, two runs (unforced merged, and a forced flush per rect). Per rect, log s0, t0, s1, t1 (floats), sc, s_base, t_base and the 4 encoded shorts read back from gring. The floats are equal in both runs, so if the shorts or the base differ for the rects covering game rows 0-45, the cause is confirmed.
Fix (tex_raw_rect / rect_raw_open, pm_gpu.c): quantize the base to whole normalized units, `s_base = floorf(min(s0, s1))` (the same for t), and pick sc from the span after subtracting the base. Because sc is a power of two and the base is an integer, round((s - k) * sc) = round(s * sc) - k * sc exactly, so the encoding no longer depends on batching. Keep the merge key as it is (sc and base equal); merging still occurs, with no picture effect.
Gate:
- fb_sky_mac00 (3 shots) byte-equal to the pre-row sky in all three modes: merged, forced flush per rect, and ringsmall=64.
- scforce (the s + 200 sky test) byte-equal.
- mac_01 and pause sky byte-equal.

## Rules
One agent per FB row, own worktree (fb1, fb2, fb3, fb4) from nds1; copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). Forcing switches are debug switches for the gate; FB-4 deletes them with the CPU path. Byte-equal against the pre-row build on the forced scene. Commit on your branch, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Rebase on nds1 before each commit; check merge markers per commit. FB-4 waits for FB-1..3, TRI-4 and MDL-3. Delete this file in the FB-4 commit.

## Oracle review FB-1 ring and grab (2026-11-07, replaces the sc base premise, refuted by measurement: base 0 on every sky rect)
State at handback: fb1 62d1cb2c8 (grab WIP). Zmark merged in nds1 28e1b03b3.
Ring trace: a merged rect batch draws at close (flush_impl_body rect branch, nd_raw_native). Unit 0 words come from pm_tex_native_unit0 at close time (live tile and filter state). Only combine, rect_2d, rect_z are restored from the batch (rect_ec, rect_zb). Filter, scissor (stage_scissor), prim/env/cconst and the TEV key are read live at close. Best fit: texture filter. background.c sets G_TF_POINT for the sky; cam_main.c render_frame sets G_TF_BILERP for the world. If op_setothermode (filter) does not flush an open rect batch, the merged sky draws bilinear. Small gradient diff (max 13) fits.
Measurement (cold, frames 199-201): log the unit 0 param word (filter, wrap) and scissor at each rect queue (rect_raw_open/quad) and at each rect batch close. Close value != queue value in merged mode names the field; the op logged between names the setter.
Fix: rect_raw_open stores unit words (pm_tex_native_unit0 at queue time) and scissor in the batch, the merge key includes them, the nd_raw_native rect branch uses the stored words. Every setter of filter, scissor, prim/env/cconst flushes when rect_open (nverts > 0 || gni > 0 || rect_open).
Gate (pictures after sprfix is in nds1): fb_sky_mac00 byte-equal to the pre-row sky in merged, flush-per-rect and ringsmall=64 modes; mac_01 and pause sky byte-equal; cold runs.
Grab (after sprfix): scene nok11_splash only (21 grab_impl calls; pause_spin_grab never calls grab_impl, drop it as a grab test). First CPU-vs-CPU check: two cold nok11_splash runs with native grab off. If they differ, find the noise first (first differing frame, pm_tex slot census, frame heap length). Log at grab_impl and at the native draws that sample the grab: grab_tex dim, param, lod, addr, fmt, the u0 words, quad uv, frame, call index. Gate: CPU deterministic, native byte-equal to CPU, logged words equal (grab slot addr and dim, param nearest/clamp).

## Oracle decision FB-3 gates (2026-11-07)
Part 1 (fb3 4bd620b45): gate = pm5all vs pm4 same build 0 px, mac_01 0 px, plus battle_start x185 y23-98 by eye (the lit two-texture draw textured, not texel (0,0)) and the ndmcheck/ndref line of that draw with u1 and uvx set. Byte-equal to pre is not valid there (pre pm5.v.pica `mov outtc1.xy, zeros`, no uvx for pm5).
Part 2: after FB-2 (unit 2 noise in nd_emit and the key) merges: pm5 noise (outtc2 = ndc * nzo in pm5.v.pica as pm4), nraw7 and nd_model why 6 noise removal. Gate: pm5 noise counts 0, byte gate on nok11_splash.
battle_start_f182: keep it. Make it deterministic: cold start, fixedseed and fs1off in the scenario, a build with sprfix. Base vs base byte-equal twice. Only if it still differs: by-eye gate and name the remaining source (FS1 frame skips in pm_main.c fs_skip use wall time; RNG seed; warm restart).
