# ws_NAT5G (Sonnet: 2 files, both cores, record layout of every unit copy): the geometry units (VTX, TRI1, TRI2, MTX, POPMTX, MOVEMEM) and the control units (RECT with its 2 RDPHALF units, MISC, CALL, ND_FS_TXR) of every unit copy go into their own records: nd_geo_rec and nd_ctl_rec. NRUN records (nd_nr_run) keep only the state units. Exact by construction: each runner calls the same nr_geo and nr_ctl per unit. Counter: c0gate calls about +2 (GUESS: the 2 runner calls of nr_geo and nr_ctl), interp as base. It prepares NAT6 (the state units). Gate: drec byte-equal.
DRAFT. WAIT: `Merge NAT5T`. Refresh the line facts after the NAT5T merge. Designer: oracle NATIVE3 (a86ed4bedc3a8575e). Worktree build3ds/wt/NAT5G, branch NAT5G. PM_ROW=NAT5G PM_LANE=NAT5G. Owns: 3ds/src/nd_rec.c (st_put, nd_alt_put, nd_rec_nr), 3ds/src/nd_walk.c (two new runners next to nd_nr_run), 3ds/include/nd_api.h (2 prototypes). NAT7x rows (oracle NATIVE6) do not edit these functions.
## Facts (main 73db70e24e plus NAT5T; cite by name)
- FACT st_put (nd_rec.c) extends the open NRUN record (st_hdr, st_end) or opens a new one (NDC_NAPI, flag 0, run nd_nr_run). stream_run keeps the snap arm for nd_nr_run records and for flag 1 records. nd_alt_put and nd_rec_nr write every unit of a copy through st_put (NAT5F, NAT5N, NAT5B, NAT5A, NAT5T).
- FACT nr_geo (nd_walk.c) is the same rule in every executor (nd_nr_exec, ls_run, the stream): gv_epoch only for MOVEMEM, pm_gbi_cur_op, the handler. nr_ctl (dbg, u, at, e) returns the units it took (2 for a texture rectangle, which reads at[3], at[5]) or ~0u for a branch. nd_nr_exec and ls_run skip ND_NOP_A.
## Edits (new lines end with `/* NAT5G */`)
1. nd_rec.c: st_put becomes `static void run_put(NdNapiRun run, unsigned flag, uint32_t w0, uint32_t w1)`, the same body with `h[1] == (uint32_t)(uintptr_t)run` added to the extend test, and the header `NDC_HDR(NDC_NAPI, 1, flag)` with p[1] = run. `#define st_put(w0, w1) run_put(nd_nr_run, 0u, (w0), (w1))` keeps every old call. ndc_n[NDC_NRUN] counts all three runs.
2. nd_rec.c: the router for the units of a copy, called by nd_alt_put (after the NAT5T swap) and by nd_rec_nr in place of its final st_put:
```c
static unsigned char cp_half; /* NAT5G: the RDPHALF units still owed to a texture rectangle */
static void cp_put(uint32_t w0, uint32_t w1) { /* NAT5G: a unit of a copy into the record of its family */
    const unsigned id = w0 >> 24;
    if (w0 == ND_NOP_A) { if (cp_half) { cp_half--; run_put(nd_ctl_rec, 1u, w0, w1); } return; } /* a sync is not run; an RDPHALF follows its rectangle */
    if (nd_op_geo(id)) { run_put(nd_geo_rec, 1u, w0, w1); return; }
    if (nd_op_ctl(id)) { if (id == ND_FS_RECT && (w1 & ND_FS_RECT_TEX)) { if (st_hdr && (((st_hdr[0] >> 16) & 0xffu) > 252u || wp + 6u > wend)) st_hdr = NULL; cp_half = 2u; } run_put(nd_ctl_rec, 1u, w0, w1); return; }
    st_put(w0, w1); }
```
   The NAT5F texture rectangle rule in nd_alt_put goes (cp_put holds it). A copy is never cut between a rectangle and its RDPHALF units: they are the next 2 units of the same list.
3. nd_walk.c, after nd_nr_run:
```c
void nd_geo_rec(const uint32_t* rec) { const unsigned dbg = pm_walk_dbg, n = (rec[-2] >> 16) & 0xffu; /* NAT5G: geometry units, nr_geo each */
    for (unsigned k = 0; k < n; k++) nr_geo(dbg, rec + 2u * k, rec + 2u * k);
    if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); }
void nd_ctl_rec(const uint32_t* rec) { const unsigned dbg = pm_walk_dbg, n = (rec[-2] >> 16) & 0xffu; const uint32_t* const e = rec + 2u * n; /* NAT5G: control units, nr_ctl each */
    ep_stream = alt_mode == 0u; /* as nd_nr_run: nr_ctl reads it for ND_FS_TXR */
    for (const uint32_t* q = rec; q < e; q += 2) { if (q[0] == ND_NOP_A) continue; const unsigned r = nr_ctl(dbg, q, q, e); if (r == ~0u) break; q += 2u * r; }
    ep_stream = 0; if (__builtin_expect(dbg & WD_PH, 0)) pm_gpu_ndt_cls(-1); }
```
   nd_api.h: `void nd_geo_rec(const uint32_t* rec); void nd_ctl_rec(const uint32_t* rec); /* NAT5G, nd_walk.c, core 0 */`. Add weak stubs of both next to the other weak stubs of nd_rec.c (host units link nd_rec.c without nd_walk.c).
4. A branch CALL ends its list: core 1 puts no unit after it (ds_pairs returns), so the break in nd_ctl_rec ends the same units as `q = e - 2` in nd_nr_exec.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 8 Azahar starts of 8)
1. Refresh the facts on main after the NAT5T merge (names above; one line each in the commit message). Edits 1-3. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat5g_host.txt 2>&1`: PASS (stunit `run` checks read the NRUN header: fix their expected header if a test record holds a geometry unit). Commit `NAT5G 1: geometry and control records`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, wscmb_pause_drec: `python 3ds/tools/bench.py base <B>`. 3. `sh 3ds/tools/rowloop.sh --row NAT5G --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec,wscmb_pause_drec --no-host --why "NAT5G: geometry and control records" > build3ds/out/nat5g_loop.txt 2>&1`. 4. `grep -H "drec frame\|drec draw .* DIFF" build3ds/azt/logs/*_drec.txt > build3ds/out/nat5g_c0src.txt`. 5. c0gate on build3ds/lane-NAT5G/pm_3ds.elf (base: the start-commit ELF of the bench base lane).
## Gate (exact): host PASS, 4 drec equal, interp and state as base, report calls. Fix rights: compile errors, prototypes, host stubs and stunit expectations. A DIFF, a crash or a tool fault: stop, send nat5g_c0src.txt and the first DIFF line to oracle NATIVE3 (a86ed4bedc3a8575e), wait.
## Hand-back: last commit `NAT5G 2: gate. <lines>` with `git add -f build3ds/out/nat5g_c0src.txt build3ds/out/nat5g_loop.txt`. It deletes this page. One SubagentHandback: hash, gate line per bench, c0gate line.
