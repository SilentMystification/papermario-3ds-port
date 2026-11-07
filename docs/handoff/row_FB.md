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

## Rules
One agent per FB row, own worktree (fb1, fb2, fb3, fb4) from nds1; copy assets/us and ver/us/build. Code first (CLAUDE.md rule 5). Forcing switches are debug switches for the gate; FB-4 deletes them with the CPU path. Byte-equal against the pre-row build on the forced scene. Commit on your branch, no attribution lines, never commit 3ds/src/map_restore_table.c, keep Azahar open. Rebase on nds1 before each commit; check merge markers per commit. FB-4 waits for FB-1..3, TRI-4 and MDL-3. Delete this file in the FB-4 commit.
