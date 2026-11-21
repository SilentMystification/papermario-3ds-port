# ws_PRES0 (Haiku): present timeline census: vblank gaps per picture, dropped pictures, weight error by time, part 1 GPU time (INTERP row 2)
Oracle NATIVE23 (plan_interp.md 6). Worktree build3ds/wt/PRES0, branch PRES0, PM_ROW=PRES0 PM_LANE=PRES0. Start: main after the IK0 merge, else the IK0 gated tip.
Why Haiku: a wrapper, counters, 2 prof lines, all below. No draw change. Counter: none of C1..C12. FACT (objdump of libcitro3d.a, libctru gx.o): C3D_Init sets the queue callback onQueueFinish, which calls gfxScreenSwapBuffers. GX_DisplayTransfer queues id 3. The callback is a public field (gx.h:83-92).
## Edits (3ds/src/pm_gpu.c, pm_main.c, pm_phase.c in this order. Sources are CRLF: use the Edit tool. Each edit ends with `/* PRES0 */`.) 1. pm_gpu.c above the KICK1 comment (:841) add:
```c
/* PRES0: present census (plan_interp.md 6.9). This wrapper runs before citro3d's queue callback (onQueueFinish, the top swap). */
static void (*pres_prev)(gxCmdQueue_s*);
volatile unsigned pm_pres[14]; /* 0..3 gap after an L picture (0, 1, 2, 3+ vblanks), 4..7 after a K or plain one, 8 drop, 9 wob, 10 pictures, 11 L to K ticks, 12 part 1 ticks, 13 part 1 n */
unsigned pm_pres_step; /* one per step, pm_main.c wq_step_end */
static volatile unsigned pres_arm, pres_kind, pres_h, pres_vb0, pres_h0, pres_k0, pres_t0, pres_have, pres_kick;
static void pres_cb(gxCmdQueue_s* q) {
    const unsigned n = q->numEntries;
    if (pres_arm && n && (q->entries[n - 1u].data[0] & 0xFFu) == 0x03u) { /* the finish of an armed picture: its last entry is the display transfer */
        const unsigned vb = C3D_FrameCounter(0), t = (unsigned)svcGetSystemTick(), g = vb - pres_vb0, d = pres_h - pres_h0;
        pres_arm = 0; if (gspIsPresentPending(GSP_SCREEN_TOP)) pm_pres[8]++; /* the last picture is never shown (gspgpu.h:133) */
        if (pres_have) { pm_pres[(pres_k0 == 1u ? 0u : 4u) + (g > 3u ? 3u : g)]++; pm_pres[9] += d > g ? d - g : g - d; if (pres_k0 == 1u) pm_pres[11] += t - pres_t0; }
        pm_pres[10]++; pres_vb0 = vb; pres_h0 = pres_h; pres_k0 = pres_kind; pres_t0 = t; pres_have = 1;
    } else if (pres_kick) { pm_pres[12] += (unsigned)svcGetSystemTick() - pres_kick; pm_pres[13]++; pres_kick = 0; }
    if (pres_prev) pres_prev(q);
}
static void pres_arm_set(unsigned kind) { if (!pm_pf_on) return; pres_kind = kind; pres_h = 2u * pm_pres_step + (kind == 1u ? 1u : 2u); pres_kick = 0; pres_arm = 1; }
```
2. KICK3 (:867): put `if (pm_pf_on) pres_kick = (unsigned)svcGetSystemTick();` before `gxCmdQueueRun(...)` on that line. pm_gpu_init_w: after the `}` that closes
   `if (!C3D_Init(0x180000)) {` (:1892) add `{ gxCmdQueue_s* q_ = (gxCmdQueue_s*)&__C3D_Context; pres_prev = q_->callback; q_->callback = pres_cb; } /* PRES0 */`.
3. Edit shape (an arm call before a frame end). L frame end (:3342) before: `    ndunit_scan(); rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);`. After: the new line
   `    pres_arm_set(pm_icmp_kind == 1 ? 1u : pm_icmp_kind == 2 ? 2u : 0u); /* PRES0 */`, then the old line. Key show (:3224): `    pres_arm_set(2u); /* PRES0 */` before `    rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);`.
4. pm_main.c: above `static void wq_step_end` (:476) add `extern unsigned pm_pres_step; /* PRES0: pm_gpu.c */`. Its body starts with `pm_pres_step++;`.
5. pm_phase.c after the line `... pm_slot_race); } /* SLOTL: since boot, must stay 0 */` (:787) add:
```c
            { extern volatile unsigned pm_pres[14]; static unsigned m_[14]; unsigned d_[14]; /* PRES0 */
              for (int k = 0; k < 14; k++) { const unsigned v_ = pm_pres[k]; d_[k] = v_ - m_[k]; m_[k] = v_; }
              const unsigned nl_ = d_[0] + d_[1] + d_[2] + d_[3], gl_ = nl_ ? (unsigned)(d_[11] * 100ull / PF_TPM / nl_) : 0u, g1_ = d_[13] ? (unsigned)(d_[12] * 100ull / PF_TPM / d_[13]) : 0u;
              pm_log_prof_line("prof f%u w%u pres lk %u %u %u %u kl %u %u %u %u drop %u wob %u glk %u.%02u", pf_total, pf_w, d_[0], d_[1], d_[2], d_[3], d_[4], d_[5], d_[6], d_[7], d_[8], d_[9], gl_ / 100u, gl_ % 100u);
              pm_log_prof_line("prof f%u w%u pres1 n %u gpu1 %u.%02u n1 %u", pf_total, pf_w, d_[10], g1_ / 100u, g1_ % 100u, d_[13]); }
```
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs. 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=PRES0 sh 3ds/build.sh`. `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. `bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_fps60_det --row PRES0 --why "PRES0 census"`, `bench.py cmp ... --row PRES0`: 12/12 byte-equal.
3. `python 3ds/tools/capture_check.py --row PRES0 --proof "pres lk,pres1 n" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1 padscript=go_-300_0_500,wait40,go_-100_0_500`. Expected in Azahar (counts valid, gaps and ms not): pres1 n near 2 per step of the window, lk and kl sums above 0. Never add phase=on.
Fix rights: compile and link errors, a missing declaration. A DIFF, a base fault, a missing line, a hang: stop, send the output path to oracle NATIVE23.
Stop: commit `PRES0 1: gate. <host> <c0gate> <cmp line> <pres line> <pres1 line>` deletes this page. Hand-back: one SubagentHandback: commit hash and both lines.
