# ws_PRES1 (Haiku): present gate: each fps60 picture waits until the last picture is on screen (INTERP row 3, plan_interp.md 6.8)
Worktree build3ds/wt/PRES1, branch PRES1. PM_ROW=PRES1 PM_LANE=PRES1. Start: main 3a29e12504 or later (line numbers: grep each anchor again). CRLF sources: use the Edit tool.
Objective: no fps60 picture is dropped, no display transfer writes the buffer on screen. Only the time of the presents changes, no command word. Why Haiku: 6 closed edits. Counters: none of C1..C12. Console census `pres drop` 150 -> 0 (FACT now: drop 150 of 300 pictures, hw_live_log_170.txt f3089). Each new line ends with `/* PRES1 */`.
## Edits (3ds/src/pm_gpu.c, then 3ds/src/pm_phase.c). One commit.
1. pm_gpu.c :875 `static volatile unsigned pres_arm, ..., pres_kick;`: add `, pres_tx, pres_sw` before the `;`. Line end: `/* PRES1: pres_tx pictures queued, pres_sw swaps done */`.
2. pres_cb. Edit shape "a test into a named value". Before :878: `    if (pres_arm && n && (q->entries[n - 1u].data[0] & 0xFFu) == 0x03u) { /* the finish of an armed picture: ...`
   After, 2 lines: `    const int tx_ = n && (q->entries[n - 1u].data[0] & 0xFFu) == 0x03u; /* PRES1: a picture finish: its last entry is the display transfer */`
   and `    if (pres_arm && tx_) { /* the finish of an armed picture */`. After :884 `    if (pres_prev) pres_prev(q);` add
   `    if (tx_) pres_sw = pres_tx; /* PRES1: citro3d's swap (pres_prev) of the last queued picture is done */`.
3. After :886 (the line `static void pres_arm_set(unsigned kind) { ...`) add:
```c
volatile unsigned pm_pres_g[3]; /* PRES1: gate waits, wait ticks, timeouts */
static void pres_gate(void) { /* PRES1 (plan_interp.md 6.8): the next transfer waits until the last picture is swapped and on screen */
    if (pm_interp_on <= 0 || pm_wd_wake_frames) return;
    if (pres_sw == pres_tx && !gspIsPresentPending(GSP_SCREEN_TOP)) return;
    const unsigned t0 = (unsigned)svcGetSystemTick();
    int i = 0;
    while ((pres_sw != pres_tx || gspIsPresentPending(GSP_SCREEN_TOP)) && i++ < 200) svcSleepThread(200000ll); /* 0.2 ms steps, 40 ms cap as vblank_wait_timed (pm_main.c:446) */
    pm_pres_g[0]++; pm_pres_g[1] += (unsigned)svcGetSystemTick() - t0; if (i > 200) pm_pres_g[2]++;
}
```
4. Edit shape "a gate before a frame end". L frame end, before (:3416-3417): `    pres_arm_set(pm_icmp_kind == 1 ? 1u : pm_icmp_kind == 2 ? 2u : 0u); /* PRES0 */`
   and `    ndunit_scan(); rd_sync(); C3D_FrameEnd(GX_CMDLIST_FLUSH);`. After: the new line `    pres_gate(); pres_tx++; /* PRES1: pres_tx counts before C3D_FrameEnd runs the queue */`,
   then the 2 old lines unchanged. Key show: put the same new line before :3296 `    pres_arm_set(2u); /* PRES0 */`.
5. Key show: after :3290 (the `}` that closes the entry loop, below `gxCmdQueueAdd(q, &e);` :3289), before `    u32 *cb, csz, coff;` :3291, add
   `    if (q->numEntries) gxCmdQueueRun(q); /* PRES1: the GPU draws the K entries while pres_gate waits (as KICK3 :914) */`.
6. pm_phase.c :820. Before: `              pm_log_prof_line("prof f%u w%u pres1 n %u gpu1 %u.%02u n1 %u", pf_total, pf_w, d_[10], g1_ / 100u, g1_ % 100u, d_[13]); }`. After, 3 lines:
```c
              extern volatile unsigned pm_pres_g[3]; static unsigned mg_[3]; unsigned e_[3]; for (int k = 0; k < 3; k++) { const unsigned v_ = pm_pres_g[k]; e_[k] = v_ - mg_[k]; mg_[k] = v_; } /* PRES1 */
              const unsigned gm_ = pf_w ? (unsigned)(e_[1] * 100ull / PF_TPM / pf_w) : 0u; /* PRES1: gate wait ms per step x 100 */
              pm_log_prof_line("prof f%u w%u pres1 n %u gpu1 %u.%02u n1 %u gw %u gms %u.%02u gto %u", pf_total, pf_w, d_[10], g1_ / 100u, g1_ % 100u, d_[13], e_[0], gm_ / 100u, gm_ % 100u, e_[2]); }
```
## Tests (output to build3ds/out/, read the last 20 lines). 1 build. Azahar: the base (if not cached), 1 run, 1 capture check. Budget 8 starts.
1. `PM_LANE=PRES1 sh 3ds/build.sh` (memcheck OK). `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `python 3ds/tools/c0gate.py <elf>`: 0 / 0 / 0.
2. Gate: `bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`, `bench.py run spot_mac01_fps60_det --row PRES1 --why "PRES1 gate"`, `bench.py cmp spot_mac01_fps60_det --row PRES1`: 12/12 byte-equal (12 shots: FACT, PRES0 commit 9c989a1627).
3. `python 3ds/tools/capture_check.py --row PRES1 --proof "pres lk,pres1 n" --profile prof fps60 warp=mac_01:0 pos=0:0:200 profdisk=1`. Never add phase=on.
   Pass in Azahar: every `pres` line has `drop 0`. Every `pres1` line has `gto 0` and `gw` above 0. Seeded-fault reference: these words gave `drop 19` without the gate (FACT, PRES0 commit 9c989a1627).
Fix rights: compile and link errors, a missing declaration, a -Werror warning, a mechanical tool fault (note it in the commit). Stop and send the output path to the INTERP oracle (SendMessage, the orchestrator names it): a DIFF, `drop` above 0, `gto` above 0, `gw 0`, a hang, a base fault.
Not a fault: `c0k gpuk` drops (the K entries run before C3D_FrameEnd, as gpul after KICK3). Console (GUESS, no gate): drop 0, lk 0 300 0 0, glk 10 to 14, gms 4 to 7, ft +0 to +2.
Stop: commit `PRES1 1: gate. <host> <c0gate> <cmp line> <pres line> <pres1 line> CAPTURE CHECK OK` deletes this page. Hand-back: ONE SubagentHandback, the hash and these lines.
