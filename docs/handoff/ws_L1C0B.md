# ws_L1C0B (Haiku): core 0 model hit queue: resolve now, submit at the next cut (design: oracle L1C0)
Worktree build3ds/wt/L1C0B, branch L1C0B. PM_ROW=L1C0B PM_LANE=L1C0B. Start: main after `Merge L1C0A`. Line numbers are main 9e01a84888 plus the L1C0A lines: grep each anchor again (CRLF: Edit tool).
WAIT: start only when both rules hold. Q0 (Azahar, L1C0A hand-back): mqw bad 0 on both benches, and r2 + r3 + r4 >= hits / 2 in the spot_mac01_drec mqr line. C0 (console, the L1PH0 capture, plan_l1.md 5): l1m e0 of mkey + mmtx + mhit + sub + supd + semit >= 50% of the l1t e0. A rule fails: do not start (the oracle writes L1C0D, submit diet).
Objective: pm_gpu_model_hit keeps up to 8 resolved hits in a queue. pm_mq_cut_do (every L1C0A cut) submits them in record order. The command words stay byte-equal. Why exact (FACT code read): inside a run the resolve writes no command word (L1C0A bad 0), and the submit reads only the entry, the queue item and serial GPU shadows. Every load, eviction, viewport change, miss and other command is a cut.
Counters (GUESS, console capture after the merge; scale by (r2 + r3 + r4) / hits): core 0 pmu m -8000 to -30000 per clean frame (FACT now 72384), I misses per draw -60 to -225 (FACT now 544), core 0 -1.2 to -4.5 ms per clean frame. Endgame C1-C12: none.
## Step (one commit `L1C0B 1: hit queue`, 3ds/src/pm_gpu.c only)
1. L1C0A block before drec_begin: after the mq_mark definition add `static void mq_drain(void); /* L1C0B */`. pm_mq_cut_do: Before `mq_why[r & 7u]++; pm_mq_run = 0; }` After `mq_why[r & 7u]++; mq_drain(); } /* L1C0B */`. drec_end mqw line: add ` %u` to the format and `, mq_why[7]` after `mq_why[6]` (reason 7: a full queue).
2. After ffog_note (before `int pm_gpu_model_hit(`) add this block:
```c
/* L1C0B: the model hit queue (ws_L1C0B): a hit stores what it resolved; mq_drain runs the old submit loop of each item in record order at the next cut. */
#define PM_MQ_N 8u
typedef struct { NdMEnt* e; NdUnit u; uint32_t shc; int shr; float pal[16], palk[16]; } PmMq; /* entry, unit 0 words, shroud colour, palette 0 of this view, its K twin */
static PmMq mq_q[PM_MQ_N]; static float mq_zpal[16], mq_zpalk[16]; /* PERF2 step 3, INTERPB2: the zmark pass 2 palette of the draw that goes out now, its K twin */
static void mq_drain(void) {
    const unsigned n = pm_mq_run; pm_mq_run = 0;
    for (unsigned i = 0; i < n; i++) {
        const PmMq* const m = &mq_q[i]; NdMEnt* const e = m->e;
        if (pm_tw_on) memcpy(mq_zpalk, m->palk, sizeof mq_zpalk);
        for (unsigned j = 0; j < e->n; j++) {
            NdNatC* q = nd_mc_nat(e, j);
            if (m->shr && q->tk.r_mix == 1 && q->mix_st >= 0) q->d.tev_col[q->mix_st] = m->shc;
            if (q->zp) { memcpy(mq_zpal, m->pal, sizeof mq_zpal); for (int k = 0; k < 4; k++) mq_zpal[8 + k] = q->zn * m->pal[12 + k]; q->d.pal = (const float(*)[16])mq_zpal; for (int k = 0; k < 4; k++) mq_zpalk[8 + k] = q->zn * m->palk[12 + k]; } /* S5-2: zmark pass 2 */
            else q->d.pal = (const float(*)[16])m->pal;
            q->d.pal_k = pm_tw_on ? (const float(*)[16])(q->zp ? mq_zpalk : m->palk) : NULL; q->d.nrm_k = NULL; /* INTERPB2 */
            if (e->tex) q->d.u[0] = m->u;
            pm_gpu_native_submit_draw(&q->d, q->sd, q->nidx, q->d.u[0].dim != 0);
            mfr_idx += q->nidx;
        }
    }
}
```
3. pm_gpu_model_hit: replace the lines from `static float pal0[16], zpal[16];` (:4388) through `pm_gpu_flush();` (:4394) with these 7 lines: `if (pm_mq_run == PM_MQ_N) pm_mq_cut_do(7u); /* L1C0B: a full queue drains */` / `PmMq* const m = &mq_q[pm_mq_run]; /* L1C0B: the item of this hit */` /
   `float comb[4][4];` / `nd_mtx_combine(comb);` / `pm_gpu_pcomp_native(m->pal, comb);` / `if (pm_tw_on) memcpy(m->palk, pm_pal0_k, sizeof m->palk); /* INTERPB2: the K twin of palette 0 */` / `pm_gpu_flush();`
4. Keep the 4 lines `const uint32_t* rw` to `const uint32_t shc` and the L1C0A bad line. Replace the submit loop (`for (unsigned j = 0; j < e->n; j++) {` to its `}`, old :4399-:4408) with `    m->e = e; m->u = u; m->shc = shc; m->shr = shr; /* L1C0B: mq_drain submits its draws */`. The L1C0A mq_mark line, `ndn_rec++; ...` and `return 1;` follow, in this order.
## Tests (output to build3ds/out/, read the last 20 lines). Gate: 1 build, 2 benches. Budget 8 Azahar starts.
1. `PM_LANE=L1C0B sh 3ds/build.sh` (memcheck OK; .bss +1184 B FACT arithmetic: 8 x 164 B queue - 128 B of statics), `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS, `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. For spot_mac01_drec and entcape_kmr03_drec: `bench.py base <bench> --commit $(git merge-base HEAD main)`, `bench.py run <bench> --row L1C0B --why "L1C0B gate"`, `bench.py cmp <bench> --row L1C0B`. Pass: both byte-equal, `mqw f201 bad 0` in both logs.
Fix rights: compile errors, -Werror warnings, a mechanical tool fault. Any DIFF or bad > 0: stop, send the cmp output path and the log path to the oracle (SendMessage).
Stop: the commit `L1C0B: gate` deletes this page. Hand-back with ONE SubagentHandback: commit hash, host, c0gate, memcheck line, 2 cmp lines, the mqr and mqw lines of both logs.
Capture after the merge (orchestrator, user yes): words `warp=mac_01:0 pos=0:0:200 profdisk=1 prof=1 profph profrot`, capture_check proof `l1m,l1t`. Counter: pmu m / gpu d against the L1PH0 capture.
