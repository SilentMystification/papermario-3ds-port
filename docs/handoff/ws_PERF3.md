# ws_PERF3 (Haiku: closed edits, byte-equal): delete dead code and dead drec branches; a model cache hit reads its own shroud blend colour
Designer: oracle NATIVE15 (plan_perf.md sections 3 and 4). Branch from main. Worktree build3ds/wt/PERF3, branch PERF3. PM_ROW=PERF3 PM_LANE=PERF3.
Objective: less code on the hot files, one fixed bug. c0gate stays 0 / 0 / 1. No prof value moves (FACT: no deleted function has a caller).
Edit only the lines below (line numbers main 1472130686). Sources are CRLF: use the Edit tool or `sed -b -i`.
## Facts
- No caller in 3ds/src, src, 3ds/tools, 3ds/tests (grep -w): nd_c0_src, cc_text, cc_uses, nd_env_vtx, stdump_on, tri_stats, pm_gbi_ptr_of.
- nd_c0d is never written: c0_dwho never runs. c0_wk is written only by nd_c0_src: c0_who counts key 0 only.
- pm_gpu.c:4228 reads nd_msnap_w(). ms_cur changes only in nd_msnap_use (nd_msnap.c:36-40), which runs only in rec_miss (nd_recs.c:196).
  A cache hit (nd_recs.c:267, :179) so takes the blend colour of the last record that missed. k->rp is the record payload (pm_gpu.c:3875).
## Edits
1. pm_gpu.c: delete lines 600 to 604 (nd_stream_cur extern, c0_wk, c0_k, nd_c0_src, c0_who, nd_c0d, c0_dk, c0_dwho). Delete lines 634 and 635 (c0who, c0diff events).
   :1336 before: `c0_d++; if (nd_c0w & 3u) c0_who(); if (nd_c0w & nd_c0d & 3u) c0_dwho(); /* NAT1D */ /* NAT1B */ c0_c += ...`
   after: `c0_d++; c0_c += ...` (the rest of the line stays).
2. nd_int.h: delete line 144 (`void nd_c0_src(void);`) and line 246 (`void tri_stats(void);`).
3. nd_comb.c: delete lines 78 to 97 (cc_text, the blank line, the comment `/* 1 if the combiner reads texel tile t (0 or 1). */`, cc_uses to its `}`).
4. nd_shape.c: delete line 51 (stdump_on) and lines 122 to 126 (nd_env_vtx to its `}`). nd_geom.c: delete line 147 (tri_stats). nd_walk.c: delete line 444 (pm_gbi_ptr_of).
5. nd_msnap.c after line 33 (nd_msnap_key):
   `const uint32_t* nd_msnap_rw(const uint32_t* q) { const NdMSnap* s = ms_of(q); return s ? &s->magic : NULL; } /* PERF3: the snap words of record q, NULL when none */`
   nd_msnapw.h after the nd_msnap_key line: `const uint32_t* nd_msnap_rw(const uint32_t* q); /* PERF3 */`.
   pm_gpu.c:4228 before: `const uint32_t shc_bc = nd_msnap_w()[MSW_BLENDC];` after:
   `const uint32_t* const msr_ = rw ? nd_msnap_rw(rw) : NULL; const uint32_t shc_bc = (msr_ ? msr_ : nd_msnap_w())[MSW_BLENDC]; /* PERF3: this record's snap (a hit runs no nd_msnap_use) */`
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `grep -rnw "nd_c0_src\|cc_text\|cc_uses\|nd_env_vtx\|stdump_on\|tri_stats\|pm_gbi_ptr_of\|c0_dwho\|c0_who" 3ds/src 3ds/include src 3ds/tools > build3ds/out/perf3_dead.txt`: empty.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/perf3_host.txt` (timeout 600000): PASS. Commit `PERF3 1: dead code, shroud blend of a hit`.
3. `PM_LANE=PERF3 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
4. Base, run and cmp as ws_PERF0 step 3, benches spot_mac01_drec,hudfb_drec: every shot byte-equal (GUESS: these scenes have no shroud record).
Fix rights: the edited lines, compile errors, warnings for a static helper that lost its last caller (delete that helper too, name it in the commit).
A DIFF, a host failure or a caller found in step 1: stop, send the output path to oracle NATIVE15.
Stop: last commit `PERF3 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp results, the deleted line count.
