# plan_record_capture: the capture driver for UC1 (epic SR, copy path)

Plan only (Opus). Base HEAD dfa71bd36. No source edit, no run, no git change except this file.
Inputs: CLAUDE.md, agent_uc1.md, agent_sr3.md, plan_uniform_cut.md, plan_stateful_records.md, decision_jump_closed.md,
hw_guidelines.md, and grep reads of 3ds/src/pm_gbi.c (3345 to 3476, 1764), pm_gpu.c (129, 476, 480, 535 to 556, 743 to 750,
794 to 838, 853 to 1054, 2052, 2253), src/model.c (1498 to 1519, 2623, 2708), include/model.h (66 to 83, 198),
3ds/tools/azt_scenarios.json (yard, mac01t_cap1). Tags: [C] read from code, [I] inference.

## 0. Decision: (a) a capture row first (RC1), then UC1

Reason:
1. Size. The capture driver is about 55 lines in 4 files. UC1 is about 50 lines in pm_gpu.c. Folded, the row is about 105
   lines. Rule 2 and the SR plan limit a row to 60 lines.
2. One quantity per shot (rule 5). RC1 proves the trigger, the span copy and the header count layout (`wbad` 0). UC1 then
   proves only the forced uniform shape and the patch table (`pt`). Folded, a `wbad` > 0 or a picture change has two
   possible causes in one diff.
3. The header walk moves into RC1. The count-field layout (plan_uniform_cut section 3) is then proven on real words before
   UC1 adds the uniform tracking to the same loop. UC1 extends the walker. It does not add a second one.

State found [C]: SR1 and SR2 are in the tree (pmRecVer, pmSlot, pmMtxVer, PM_REC_BUMP sites, tb_gen pm_gbi.c 2575,
gstat_gen pm_gpu.c 98, header w1 model.c 1516 to 1518, slot matrix cache pm_gbi.c 1753). Not in the tree: pm_rec_ctx,
pm_fog_ver, pm_tint_ver, any rec_* symbol. Header w1 bit 9 is free (load_mtx reads bits 0 to 8 and 16 to 31, pm_gbi.c 1764 to 1766).

Copy path change [I, from decision_jump_closed.md]: the GPU never reads a record body. The CPU copies it into the frame
buffer (GPUCMD_AddRawCommands). So the arena is normal heap (cached), not linearAlloc, and no JUMP0 word is appended.

## 1. Rows (epic SR, one Haiku code agent, branch sr, in order)

### RC1: record capture driver (SR3 capture part)

Files and lines (about 55 changed lines):
- src/model.c 1498 to 1519: hoist `plain` out of the `if (pm_ph_on)` block (same expression as 1504 to 1505). Set header
  w1 bit 9 = `plain && renderMode == RENDER_MODE_SURFACE_OPA`. The mr0 and ms0 calls keep their pm_ph_on guard. About 4 lines.
- 3ds/src/pm_gpu.h: prototypes `void pm_gpu_rec_begin(void);` and `int pm_gpu_rec_end(u32** w, u32* n);`. 2 lines.
- 3ds/src/pm_gpu.c, next to pm_gpu_flush (2253), about 30 lines:
  - `int rec_cap; static u32* rb_buf; static u32 rb_off; static unsigned rb_kick, rb_draw, rb_raw; static int rec_ref;`
  - pm_gpu_rec_begin: `flush(); raw1_reset(); sh_clear();` then GPUCMD_GetBuffer(&rb_buf, NULL, &rb_off), store kick_n,
    draws_frame, pm_dc_c[DC_RAW]; rec_ref = 0; rec_cap = 1.
  - pm_gpu_rec_end: `flush(); rec_cap = 0;` GPUCMD_GetBuffer again. Return 0 (refuse) when: the buffer pointer changed,
    kick_n changed (a KICK1 split ran inside the group, 543 to 556), draws_frame delta != DC_RAW delta (a CPU draw,
    flush_draw_cpu 794), rec_ref, pm_interp_on > 0. Return -1 (empty) when the DC_RAW delta is 0. Else set *w, *n and return 1.
  - Noise refuse, inside the existing `if (noise)` block (967): `if (rec_cap) rec_ref = 1;`. No other line in flush_draw changes.
- 3ds/src/pm_gbi.c, near mg_run (3345), about 20 lines, all in one `static __attribute__((noinline))` pair:
  - `typedef struct { u32 ver; u32 gen; u8 state; u8 pad; u16 off; u16 len; u16 pad2; } RecKey; static RecKey rk[256];`
    (16 B). `gen` = pm_dlc_gen ^ (gstat_gen << 16). tgen0, tgen1, hgen and pm_rec_ctx go in UC2 (the row that measures miss).
  - Arena: `static u32* ra; static u32 ra_n, ra_gen;` malloc 256 KB once at the first capture. A malloc fail logs once
    and every capture refuses. ra_gen != the global word: ra_n = 0, memset rk (all state 0).
  - `rec_try(w1)`: slot = w1 & 0xff. Return 0 when slot == 0, or `rk[slot-1].state != 0 && rk.ver ==
    (*gCurrentModels)[slot-1]->pmRecVer && rk.gen == gen`. Else pm_gpu_rec_begin and return 1. cap++.
  - `rec_done(w1, whole)`: r = pm_gpu_rec_end(&w, &n). Refuse (state 2, ref++) when !whole (part++), r == 0, n > 1024
    words (4 KB), or the arena is full. r == -1: state 2, emp++. Else walk (below); wbad: state 2. Else copy n words to
    ra + ra_n (16 B aligned), store ver, gen, off, len, state 1, build++, kb += n * 4.
  - Walker (header only): `i = 0; while (i + 1 < n) { c = (w[i+1] >> 20) & 0xff; i += 2 + c + (c & 1); }` and
    `i != n` is wbad. Same layout as plan_uniform_cut 2b, without the uniform tracking.
- 3ds/src/pm_gbi.c case G_PM_MODEL (3467 to 3476), 3 lines: `Gfx* g0 = pc; int rc = (w1 & 0x200u) ? rec_try(w1) : 0;`
  before mg_run, and `if (rc) rec_done(w1, pc == g0 + n && !nx);` after it. mg_run itself does not change.
- Log line, at the dr1 log site cadence (pm_gpu.c 476 counters), log level debug, one line per window:
  `rec f<frame> cap <c> build <b> ref <r> part <p> emp <e> wbad <w> kb <k> n <records>`. Window counters reset after print.

Cost (hw_guidelines): per op 0 (mg_run and the walker loop are not changed). Per plain opaque model per frame: one bit
test, one call, 2 dependent loads (rk line, model pointer), 2 compares. rk is 4 KB, read one line per model. The capture
(flush, full state re-emit, copy, walk) runs only when a key changes: once per model after a map load. No division,
no double, one malloc per run. rec_try and rec_done are outside the op switch, so the hot loop size does not change.

Gate (PM_ROW=RC1 in every shell, status line in 3ds/logs/perf/status_RC1.txt before each start):
| Step | Command | Pass | Starts |
|---|---|---|---|
| 0 | HEAD build: `azt.ps1 mac01t_cap1 -Row RC1 -Try` twice, `azt.ps1 yard -Row RC1 -Try` twice | each pair byte-equal (top shots). Not equal: stop, report the scene | 4 |
| 1 | RC1 build: `azt.ps1 mac01t_cap1 -Row RC1`, `azt.ps1 yard -Row RC1` | top shots byte-equal to step 0; `rec` line: build > 0, wbad 0; cap in the second window is less than 5 (no per-frame capture) | 2 |
Total 6 starts (budget 8). The shot subjects: Mario on the Toad Town street (mac01t_cap1, shot frame 504), Mario in the yard (frame 200).
Not byte-equal: revert the row files, one fix, one more shot. wbad > 0: stop, a fresh investigation agent reads the
first bad header (offset, word, count). No code agent from a guess.

Risks:
| # | Risk | Handling |
|---|---|---|
| c1 | cap stays high each window (a script calls PM_REC_BUMP each frame, or the global word changes each frame): the capture becomes a per-frame cost | gate item "cap < 5 in window 2". Fail: investigation names the bump site |
| c2 | A KICK1 split inside a group resets the command buffer | kick_n compare refuses. The model stays state 2 until its ver changes. `ref` shows the count |
| c3 | mg_run returns early (G_DL push, G_PM_TEX miss, pm_ph_fine on): the group is not whole | `part` count. If build is 0 on mac01t_cap1 because of phase fine, use yard for the build count and report it |
| c4 | The flush at rec_begin splits a batch that DR1 merged across models. More draws on the capture frame | pixels equal by construction [I]; the byte-equal gate checks it |
| c5 | A model out of view at capture (dlc box) gives an empty group | `emp`, state 2. A later row decides on retry. Records are not drawn yet |
| c6 | Texture staging inside the capture (C3D_TexLoadImage) | not refused in RC1. UC2 reccheck counts it as bad or miss |
| c7 | 256 KB heap on the O3DS | the boot memory line of the same yard run; malloc fail refuses all captures |

### UC1: forced uniform shape and patch table (plan_uniform_cut row UC1, changed)

Files and lines (about 40, 3ds/src/pm_gpu.c and 3ds/src/pm_gbi.c):
- pm_gpu.c flush_draw at the start of `if (graw)` (890): `if (rec_cap) rec_force();` (cold noinline: sh_ok = 0, sh5_ok = 0,
  memset sh_pal_ok and sh5_pal_ok; stores RecDraw use5, pal_n, ntab, tex1, pal_g, g_cullm, vp into a 64-entry static array;
  sets rec_ref when g_use5, since UC1 to UC3 take pm4 only).
- pm_gbi.c rec_done walker: add the uniform tracking of plan_uniform_cut 2b (0x2C0 single with bit 31: cur; 0x2C1 data:
  one 4 B patch entry per vec4, split flag; incremental 0x2C0: no patch). Patch table and RecDraw go into the arena after the
  body. Counters pt (patch entries) and pk (words to registers 0x280 to 0x2DF except 0x2C0 and 0x2C1) on the rec line.
- Per-draw cost: one load and one branch on rec_cap per raw draw (not per op). The cold body runs only on capture frames.

Gate (PM_ROW=UC1): `azt.ps1 mac01t_cap1 -Row UC1`, `azt.ps1 yard -Row UC1`. Pass: top shots byte-equal to RC1 step 0
(the forced Sets send the same values); rec line build > 0, wbad 0, pt > 0. 2 starts. No new baseline (the RC1 step 0 pair
is the baseline of the epic).

Risks: u1 to u4 and u8 of plan_uniform_cut. Added: u10, a pm5 model refused in UC1 lowers build against RC1. Report build
and ref of both rows. A drop of build is expected, not a fail.

### UC2 to UC4

As plan_uniform_cut section 4 and 5, with one change: UC2 adds the key fields tgen0, tgen1, hgen (RecKey grows to 32 B,
aligned 32, still one cache line per model) and pm_rec_ctx (src/model.c 2518, 2532, 2583, 2593, 2481,
plan_stateful_records table b) into the global word. Gates: UC2 2 starts, UC3 3 starts, UC4 2 starts, all against the
RC1 step 0 baseline.

## 2. Commands

- Build: `python 3ds/tools/gen_map_restore.py`, `sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c from its .bak.
- Never commit 3ds/src/map_restore_table.c. Commit the row files on branch sr when the shot passes. Revert them when it fails.
