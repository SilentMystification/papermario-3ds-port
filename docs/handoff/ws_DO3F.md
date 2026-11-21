# ws_DO3F (Haiku): DO3 uni_native also with the fps60 key patch on (it returns early on the console today)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.5). Worktree build3ds/wt/DO3F, branch DO3F. PM_ROW=DO3F PM_LANE=DO3F. Start: main 353b31c792 or later; grep the quoted text.
Why Haiku: one loop gets the patch call that kp_scan already makes, given below.
FACT: with fps60 every L frame sets nd_kp_on = 1 (3ds/src/pm_gpu.c:198 pm_gpu_twin_begin). uni_native returns at `if (__builtin_expect(nd_kp_on, 0)) return;`
(:1329), so the dirty float uniforms stay dirty, c3d_upd_needed says yes for reason fv (111 per frame, live_cap2_170.txt c0s line) and C3Di_UpdateContext
runs per draw (c0s upd 1.05 ms). The DO3 gates ran with fps60 off, so they did not see this.
Objective: c0s fv about 0 and upd about 0.1 ms with fps60 on; the K frame words stay equal. Counter: console c0s upd and fv; core 0 GUESS -0.6 to -0.9 ms.
## Edit (3ds/src/pm_gpu.c uni_native :1324-1342; CRLF: use the Edit tool)
1. Delete `if (__builtin_expect(nd_kp_on, 0)) return;`.
2. In the run loop, keep a pointer to the first value word of the data command and patch each staged row, as kp_scan does (:1272) and as nd_emit.c:88 does
   (the base is the first word of the data command, the index is 4 x row offset). Before:
        *o++ = ((const u32*)&C3D_FVUnif[GPU_VERTEX_SHADER][i])[0]; *o++ = 0x2C1u | 0xFu << 16 | (nw - 1u) << 20;
        for (unsigned k = 1; k < nw; k++) *o++ = ((const u32*)&C3D_FVUnif[GPU_VERTEX_SHADER][i])[k];
        *o++ = 0; /* nw is even: the pad */
   After:
        u32* const dc = o; /* DO3F: the data command (its first value word), the base of nd_kp_vec */
        *o++ = ((const u32*)&C3D_FVUnif[GPU_VERTEX_SHADER][i])[0]; *o++ = 0x2C1u | 0xFu << 16 | (nw - 1u) << 20;
        for (unsigned k = 1; k < nw; k++) *o++ = ((const u32*)&C3D_FVUnif[GPU_VERTEX_SHADER][i])[k];
        *o++ = 0; /* nw is even: the pad */
        if (__builtin_expect(nd_kp_on, 0)) for (unsigned k = i; k < e; k++) if (u5kpv[k]) { nd_kp_vec((uint32_t*)dc, 4u * (k - i), u5kp[k]); u5kpv[k] = 0; } /* DO3F: the K value of a staged row, as kp_scan */
3. Move the static declarations of u5kp and u5kpv (:170-171) above uni_native only if the compiler needs it (they are at :170, before :1324: no move).
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=DO3F sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `python 3ds/tools/bench.py base spot_mac01_fps60_stereo_det --commit $(git merge-base HEAD main)`; run and cmp with `--row DO3F`: byte-equal
   (fps60, key frames and stereo: the K patch and the eye rows). Then the same 3 commands for spot_mac01_fps60_det: byte-equal.
Fix rights: compile errors. A DIFF or a base fault: stop, send the output path to oracle NATIVE18 (the first DIFF frame names the patch row).
Stop: commit `DO3F 1: gate. <host> <c0gate> <2 cmp lines>` deletes this page. Hand-back: one SubagentHandback: commit hash, the results.
