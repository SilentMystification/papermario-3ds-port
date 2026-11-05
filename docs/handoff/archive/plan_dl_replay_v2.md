# plan_dl_replay_v2: state-list replay, measured first (rows M1, E1 to E4, D5, D6)

Tags: [M] measured (source named), [C] read from code, [I] inference.
Line numbers are at commit 321dcbd6e (branch worktree-agent-a39b22232eb3b937c, D1). Start every row from that branch. Merge it first.
Supersedes plan_dl_replay.md sections 4 to 6. Sections 2 and 3 of v1 (cost of R3, field table, replay order) stay valid.

## 1. Why the D1 shadow saw 0 calls
- The shadow has two gates. Call site pm_gbi.c:2547: `if (!pdl_compile_one && dlc_enabled()) pdl_shadow(next);`. Inside pdl_shadow, pm_dlc.inc.c:941: `if (... || pdl_depth) return;` [C].
- Both flags stay set after the first class 2/3 miss [C]:
  1. Frame 1: the main list runs through pdl_tpl_drive (pm_gbi.c:2459, needs !pdl_compile_one).
  2. pdl_tpl_gdl (pm_dlc.inc.c:1374) calls pdl_try. A miss calls pdl_compile_begin (901-911): pdl_compile_one = 1, pdl_depth++. pdl_tpl_gdl arms the compile (1379) and pushes the list.
  3. pdl_tpl_drive pops G_ENDDL itself (1426-1429) and runs other ops as `pm_gbi_run(pc, 8u)` (1436). The G_ENDDL case of pm_gbi_run (pm_gbi.c:2518) is the only caller of pdl_comp_done. So pdl_compile_finish (876-899) never runs: pdl_compile_one stays 1, pdl_depth stays 1.
- [M diag_dl dl1, diag_dlr]: pdl_try calls f1 to f299: class 2 1, class 7 37. After f299: 0 calls, pdl comp 0. This fits: one class 2 miss, then the stuck flags [I].
- After the stuck point every frame runs the plain interpreter (pdl_tpl_drive, pdl_try, pdl_try_m and pdl_shadow are all gated on the flags) [C]. All Azahar numbers since dl1 (R2 10.07 ms, D1 11.91 ms) are the interpreter plus dlc class 1 [I].
- The diag_dlr inference "class 2 lists enter without a G_DL" is not supported. pm_gbi_run has 3 callers: pm_nusys.c:77/79 (task lists) and the test list at pm_gbi.c:3411 [C]. Class 2 ops can only come from G_DL targets [I].
- Second fault, D1 shadow key: it hashes `rmask |= rd` over all ops (pm_dlc.inc.c:947). Fields that the list writes before it reads them go into the key. This makes the hit count too low [C]. pdl_kind_s (624) uses the correct `rd & ~written`.
- Third fault: pdl_sh_tab is direct-mapped, 256 slots (949-951). A slot collision resets the entry and counts a miss [C].
- Fourth fault (D5 risk): pdl_meta (575, 629) keys kind and readset on address only. A TextureHeap address that holds a new list after a heap gen change keeps the old readset [C].

## 2. What the yard steady state runs (Azahar)
- G_DL walked per frame 238.5: class 1 68, class 7 32.5, other classes about 138 [M diag_dl dl2, f40-101]. RS_GDL counts only 1 and 7 (pm_gbi.c:2540) [C].
- Ops per frame by class of pc (dlsrc on, f9600-9899): main 2626, class 2 1647 (state 1553, flow 94, geo 0), class 3 605, class 7 987 [M diag_dl R1]. Total 5865. taskpl ops 3576 [M diag_dlr f300-599]. The two counts differ, the cause is not known [I].
- Walk f300-599: bg 2.37 + main 9.53 = 11.91 ms. dlc hit 68, dlc ops 1120 per frame [M diag_dlr].
- Not measured: G_DL count per class 2 and 3, ops per G_DL target, refused fraction. M1 measures these.
- Note: without dlsrc, dls_main_lo/hi stay 0 (set only when dls_on, pm_gbi.c:2454-2457). Main-list targets then count as class 7 [C]. dls_rd_lo is set by the first pdl_try call [C 965].

## 3. Step order
1. M1 (diagnostic, budget 3 starts): census and repeat count with no pdl gate. No draw change.
2. Gate (section 4). FAIL: stop.
3. E1 to E4: deletions and extracts. No change in the steady state (the pdl path is off there today [I section 1]).
4. D5: new pdl_try (delta compile and replay). D6: texture pre-resolve.

## 4. Row M1 (about 60 lines, pm_dlc.inc.c and pm_gbi.c)
- Delete pdl_shadow, PdlShEnt, pdl_sh_tab, pdl_sh_c, pdl_sh_buf (pm_dlc.inc.c:934-961) and the pdlsh print plus memset (1449-1450).
- Add at the same place (pm_dlc.inc.c:934):
```c
/* M1: G_DL census per class and repeat count of the read fields of class 2/3 state lists. Count only, draws nothing. */
typedef struct { u32 addr, gen, h[4], f[4]; u8 n, rr; } DlrEnt;
static DlrEnt dlr_tab[1024];
static struct { unsigned calls[8], ops[8], ref, refops, nend, r0, hitx, hits, miss, hitops, bytes, big, full; unsigned refop[256]; } dlr_c;
static u8 dlr_buf[8192];
static void dlr_count(const Gfx* l) {
    if (!l || !is_host((u32)l)) return;
    if (!dls_rd_lo) dls_init_ranges();
    u32 a = (u32)l;
    unsigned c = dls_class(a) & 7, n = pdl_nops(l);
    dlr_c.calls[c]++; dlr_c.ops[c] += n;
    if (c != 2 && c != 3) return;
    if ((l[n - 1].words.w0 >> 24) != G_ENDDL) { dlr_c.nend++; return; }
    unsigned rs = 0, wa = 0;
    for (unsigned i = 0; i + 1 < n; i++) {
        unsigned op = l[i].words.w0 >> 24, rd, wr;
        pdl_op_rw(op, &rd, &wr);
        int bad = op == G_DL || op == G_VTX || op == G_TRI1 || op == G_TRI2 || op == G_MODIFYVTX || op == G_MTX || op == G_MOVEMEM
            || op == G_SETCIMG || op == G_SETZIMG || op == G_FILLRECT || op == G_TEXRECT || op == G_TEXRECTFLIP
            || (op == G_MOVEWORD && ((l[i].words.w0 >> 16) & 0xff) != G_MW_FOG)
            || (!rd && !wr && op != G_NOOP && op != G_SPNOOP && op != G_RDPFULLSYNC && op != G_RDPTILESYNC && op != G_RDPPIPESYNC && op != G_RDPLOADSYNC);
        if (bad) { dlr_c.ref++; dlr_c.refops += n; dlr_c.refop[op]++; return; }
        rs |= rd & ~wa; wa |= wr;
    }
    if (!rs) { dlr_c.r0++; dlr_c.hitops += n; return; }
    unsigned need = 0;
    for (unsigned j = 0; j < sizeof gbi_fld / sizeof gbi_fld[0]; j++) if (gbi_fld[j].grp & rs) need += gbi_fld[j].n;
    if (need > sizeof dlr_buf) { dlr_c.big++; return; }
    unsigned nb = gbi_pack(rs, dlr_buf);
    dlr_c.bytes += nb;
    u32 h = gbi_state_fnv(dlr_buf, nb), gen = pdl_heap_gen(a);
    unsigned k = (a >> 3) * 2654435761u >> 22 & 1023, p = 0;
    while (p < 1024 && dlr_tab[k].addr && dlr_tab[k].addr != a) { k = (k + 1) & 1023; p++; }
    if (p == 1024) { dlr_c.full++; return; }
    DlrEnt* e = &dlr_tab[k];
    if (e->addr != a || e->gen != gen) { memset(e, 0, sizeof *e); e->addr = a; e->gen = gen; }
    unsigned i = 0;
    while (i < e->n && e->h[i] != h) i++;
    if (i < e->n) { if (e->f[i] != (u32)rm_frame) dlr_c.hitx++; else dlr_c.hits++; e->f[i] = (u32)rm_frame; dlr_c.hitops += n; return; }
    dlr_c.miss++;
    unsigned s = e->n < 4 ? e->n++ : (e->rr++ & 3);
    e->h[s] = h; e->f[s] = (u32)rm_frame;
}
```
- Report: in pdl_report (pm_dlc.inc.c:1443) add one line, then `memset(&dlr_c, 0, sizeof dlr_c);`. The agent finds the 2 most frequent refop values with one loop.
  `pm_log_hw("dlr f%d gdl %u %u %u %u %u %u %u %u ops %u %u %u %u %u %u %u %u | ref %u refops %u top %02x:%u %02x:%u nend %u r0 %u hitx %u hits %u miss %u hitops %u bytes %u big %u full %u", ...)`.
- Call sites, both gated on the phase switch so a normal build pays nothing:
  pm_gbi.c:2541 (before the dlc_try line 2542): `if (dlc_on && pm_ph_on) dlr_count(next);`. Delete line 2547 (the pdl_shadow call).
  pm_dlc.inc.c:1369 (pdl_tpl_gdl, before dlc_try): the same line. This counts the frames before the stuck point.
- No pdl_compile_one or pdl_depth test in dlr_count. It works with the pdl path stuck or live [C].
- Run: `$env:PM_LANE='dlr'; $env:PM_ROW='M1'; powershell -File 3ds/run_azahar.ps1 -Timeout 90 -LockWait 180 -Debug "fast preset=yard phase fixedseed"`. Until-line: the first `dlr f` line after frame 599. Read the f300-f599 window and one later window. Divide by 300 for per-frame values.
- Status line to 3ds/logs/perf/status_M1.txt: gdl per class, ops per class, ref, top refops, r0, hitx, hits, miss, hitops, bytes per call.

## 5. Gate (after M1)
Let acc = calls[2] + calls[3] - ref - nend - big - full, per frame. hit = hitx + hits + r0.
- G1: hitops >= 1000 per frame (about 4.8 ms on the console at 4.8 us per op [I console_numbers]).
- G2: hit / acc >= 70 %.
- G3: bytes / (acc - r0) <= 256 per call. A larger compare costs about as much as the ops it skips [I].
- All pass: start E1. Any fail: stop. Report the numbers and the top refused ops to the user. No code agent. If ref > acc, the next step is a read-only spec of the top refused op (it can be an op missing from pdl_op_rw, such as G_RDPSETOTHERMODE) [I].

## 6. Rows after the gate (one composer-2.5 agent, branch dl-replay, each at most 100 changed lines)
| Row | Change | Gate |
|---|---|---|
| E1 | Delete the pdl_tpl path: pm_dlc.inc.c:1183-1441 (PdlTplEnt to pdl_tpl_drive), the call at pm_gbi.c:2459-2465, the prototype at 336, the tpl fields in pdl_report. Split at 1292 if the cap counts deletions. | taskpl ops within 1 %, walk within 0.3 ms, picture equal, statehash equal |
| E2 | Delete pdl_try_m and all PdlM code: pm_dlc.inc.c:998-1181, calls at pm_gbi.c:2554-2558, prototype 335, m fields in pdl_report. | as E1 |
| E3 | Delete the old pdl_try (963-996), PdlEnt, pdl_tab, pdl_find, pdl_key, pdl_compile_begin/finish, pdl_comp_arm/done and pdl_comp_* (754-768, 795-809, 865-932), pdl_compile_one (pm_gbi.c:333) and its uses at 2518, 2548-2553. Keep pso_tab, pdl_pso_capture, pdl_lazy_tile, pdl_op_rw, gbi_fld, dlr_count. | as E1 |
| E4 | Extract gbi_set_combine(w0, w1) from the G_SETCOMBINE case and gbi_tile0_wrap(w1) from the SETTILE wrap code (v1 section 3 lines 3-4). The cases call them. | statehash equal |
| D5 | New pdl_try, called at the old place in the G_DL case (after dlc_try). Accept test = the dlr_count loop (move it into `static unsigned pdl_scan(const Gfx* l, unsigned* rs, unsigned* ws)`, dlr_count calls it). Entry `typedef struct { u32 addr, gen; u8 n, rr; PdlVar v[4]; } PdlDelta;` with `PdlVar { u32 rs, ws; u16 nin, nout; u32 cw0, cw1; u8* blob; u8 bad; }`. Hit: memcmp of gbi_pack(rs) with blob[0..nin). Replay order of v1 section 3, steps 1 to 6. Miss: `gbi_pack(rs)` into a new blob, `pdl_depth++; pm_gbi_run((void*)l, n * 8u); pdl_depth--;`, then `gbi_pack(ws)` after it; refuse the variant if dual_on or m1.valid changed or SETTIMG hit a frame buffer (v1 section 3). Change pm_gbi.c:2454 to `!dlc_depth && !pdl_depth`. pdlcheck: every 16th hit runs the compile again and memcmps the blob (count drawbad). | ops down by at least 0.7 x hitops of M1, walk down, drawbad 0, all tests pass |
| D6 | Store tile 0 slot and gen in PdlVar after the compile run (as old pdl_compile_finish 888-894). Replay sets pdl_tex_slot/pdl_tex_gen; pdl_lazy_tile (846-863) binds it. | tex hit count per frame down, walk down |

## 7. Stop rule
- M1 gate fails: stop the epic (section 5).
- A row whose shot does not show its quantity change: revert its files before the next row (CLAUDE.md rule 7).
- E1 to E4 change no steady-state quantity. A change of ops above 1 % or of walk above 0.3 ms: revert and send a read-only diagnosis.
- D5: if walk rises, or two shots show no ops drop, stop. The next step is a read-only diagnosis of the replay cost (bytes per compare, flushes per replay). No third code try.
- Any picture or statehash difference: revert the row.

## 8. Tests (rows E1 to D6; M1 needs only the dlr line)
1. Yard picture, shotmode=4:80: Mario, textured ground, shadow. Compare with build3ds/lane-dl/shots/0040.ppm. Any texture or colour change fails.
2. statehash, fixedseed, equal to the bases: toad f100 94083528, f200 83b272fe, f300 aed188c8. battle f100 94083528, f200 dda688fa, f300 6b6fe648 [M diag_dl R2].
3. Pause menu: shot shows readable text in every tab name (class 3 render-mode lists run there [I]).
4. File select flip: text strips correct, no stale palette (LOADTLUT path).
- For 3 and 4 the agent greps 3ds/SLICES.md for "pause" and "PT86" for the switches. Do not guess a preset.
- PM_ROW=<row> in every shell. Budget 8 starts per row, 3 for M1. One start per check.
- Before a build: `python 3ds/tools/gen_map_restore.py`, build `PM_LANE=dlr sh 3ds/build.sh`, then restore 3ds/src/map_restore_table.c.

## 9. Risks [I]
- dlr_count adds pdl_nops scans to every G_DL under phase. Do not use M1 walk numbers as a baseline. Take the E1 baseline walk from an E1 shot.
- A read field missing from gbi_fld or pdl_op_rw gives a wrong texture with equal statehash. Tests 1, 3, 4 and pdlcheck drawbad cover it.
- 4 variants per address can be too few. M1 miss shows it before D5.
