# ws_NC0F (Sonnet): the segment table of a static call comes from its entry record, not from nd_c0.segbase
Designer: oracle NATIVE17 (plan_perf.md section 7). WAIT NC0M and NC0T1: branch from main after both merges (shared line nd_set.c:110 with NC0M;
nd.h segbase sits between their deleted lines; NC0V entry record words). Worktree build3ds/wt/NC0F, branch NC0F. PM_ROW=NC0F PM_LANE=NC0F.
Lines: main 8bcf0252ee; NC0V, NC0M, NC0T1 move lines: grep the quoted text. Why Sonnet: a record format change and a census that decides step 2.
Objective: ptr_res and ent_segkey read the table of the static call (entry record block). Step 2 deletes nd_c0.segbase. Counter: nd_c0 -64 B. c0gate 0 / 0 / 1.
## Facts (FACT code read, main 8bcf0252ee)
- Core 0 readers: ptr_res (nd_walk.c:435-441, behind ptr_of: 20 call sites), ent_segkey (nd_ent.c:361), debug orbdbg2 (pm_gpu.c:1234).
  Writers: nd_mw_run (nd_set.c:93, stream MW SEG), nd_sv_seg (nd_set.c:110, static and copy units). Core 1 keeps its own table (nd_ds.c:38 ds_ptr, w.segbase).
- Static calls run with nd_gs = &nd_ew (nd_walk.c:186, :221, :226, :376, :380); stream VTX units with nd_gs = &loc (nd_geom.c:443); else nd_gs is NULL.
- Design premise (census checks it): a stream unit never needs the table. Only static and entity lists carry segment addresses (nd_ent.c:230 useseg).
## Step 1: new source, nd_c0 writes stay, census (commit `NC0F 1`)
1. NdGs (nd_snap.h): add `const uint32_t* seg;` at the end. nd_ew init (nd_walk.c:246) and loc (nd_geom.c:443) get `NULL` for it.
2. rec_ew (nd_rec.c): the free word p[9] (0 after NC0V) = the segment block. Memo as ew_lb: `static uint32_t ew_sv[16]; static const uint32_t* ew_sb;`
   when `!ew_sb || memcmp(s->w.segbase, ew_sv, 64u)`: copy s->w.segbase into `nd_rec_data_st(64u)` and into ew_sv, ew_sb = the copy. `ew_sb = NULL;` next to
   `ew_lb = NULL;` (nd_rec.c:179). nd_ew_run (nd_walk.c:247): `nd_ew.seg = (const uint32_t*)(uintptr_t)r[7];`.
3. nd_snap_hold (nd_snap.c:197): also `if (nd_ew.seg) { memcpy(sc_ews, nd_ew.seg, sizeof sc_ews); nd_ew.seg = sc_ews; }` with `static uint32_t sc_ews[16];`.
4. nd_sv_seg (nd_set.c:110) keeps the nd_c0 store and adds, with `static uint32_t gs_segw[16];` (copy on write):
   `if (nd_gs) { if (nd_gs->seg != gs_segw) { if (nd_gs->seg) memcpy(gs_segw, nd_gs->seg, sizeof gs_segw); else memset(gs_segw, 0, sizeof gs_segw); nd_gs->seg = gs_segw; } gs_segw[r[0]] = r[1]; }`
5. ptr_res (nd_walk.c:438-439) before: `if (seg < 16 && nd_c0.segbase[seg]) return (void*)(nd_c0.segbase[seg] + (w & 0x00ffffffu));`
   after: `const uint32_t* const sb = nd_gs ? nd_gs->seg : NULL; const uint32_t b = seg < 16 && sb ? sb[seg] : 0u; /* NC0F */`
   `if (__builtin_expect(seg < 16 && nd_c0.segbase[seg] != b, 0)) nd_seg_chk(nd_gs ? 2u : 1u);` and `if (b) return (void*)(b + (w & 0x00ffffffu));`.
6. ent_segkey (nd_ent.c:361): hash `nd_gs && nd_gs->seg ? nd_gs->seg[i] : 0u`; site 3 census when it differs from the nd_c0 hash.
7. Census nd_seg_chk(site) in nd_walk.c, cold noinline: in drec frames `pm_log_evt("segdiff f%u s%u", frame, site)`, once per site and frame. Sites: 1 stream, 2 static, 3 entkey.
## Step 2: delete (commit `NC0F 2: gate`)
- nd.h NdC0 `uint32_t segbase[16];`; the nd_c0 stores at nd_set.c:93 (the MW SEG arm) and :110; the census and its calls.
- pm_gpu.c:1234 `nd_c0.segbase[9]` to `nd_seg_of(9)`; nd_walk.c: `uint32_t nd_seg_of(unsigned i) { return nd_ew.seg ? nd_ew.seg[i] : 0u; }` and its prototype at pm_gpu.c.
## Checklist (hw_guidelines.md 3, 6): 64 B of stream data only when the table changes; a 64 B memcmp per entry record on core 1. ptr_res hot path (is_host) unchanged.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; c0gate 0 / 0 / 1.
2. Step 1: entcape_kmr03_drec (entity segments), spot_mac01_drec byte-equal; `grep -c "segdiff f"` 0 in both. Step 2: fxparts_drec, hudfb_drec byte-equal.
Fix rights: compile errors; host test expectations that read entry record words. A DIFF, a segdiff line, a crash: stop, send the path to oracle NATIVE17.
Stop: `NC0F 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, c0gate, nd_c0 size base and tip, the 4 cmp lines, segdiff count per site.
