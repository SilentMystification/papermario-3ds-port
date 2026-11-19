# ws_NAT1B (Haiku): exact c0src counts and the producer of each walker decision (c0who lines). Counter: c0gate unchanged, expected `c0gate calls 127 interp 37 state 23`. Gate: drec byte-equal.
Designer: oracle NATIVE. Branch from main. Worktree build3ds/wt/NAT1B, branch NAT1B. PM_ROW=NAT1B PM_LANE=NAT1B. Wave W1 with NAT2E (shared file nd_snap.c: this row edits main lines 132-134 and 177 only, NAT2E appends after line 218).
Edit only: 3ds/src/nd_comb.c, nd_set.c, nd_tex_set.c, nd_snap.c, nd_direct.c, pm_gpu.c, 3ds/include/nd_int.h. Line overlaps in W1: none in nd_comb.c, nd_set.c, nd_direct.c (no other W1 row edits them).
## Facts (main 18ecf83ea0)
- FACT the NAT1 bits change BEFORE the GPU calls, and those calls flush the open batch first (pm_gpu.c:4421 set_plan, :4429 set_combine, :3738 set_render). So the flushed draw counts with the new bit: the NAT1 numbers are an upper bound.
- FACT nd_stream_cur (nd_rec.c:555) is the stream command that runs now. NDC_NAPI is op 10 and its word 1 is the runner (nd_cmd.h:24).
## Edits (new lines end with `/* NAT1B */`. Find each line by its text)
1. nd_comb.c scm_replay: delete line 182 `    nd_c0w |= 1u; /* NAT1 */`. Before the closing `}` of scm_replay (after `    else pm_gpu_set_combine(leg_mode);`) add `    nd_c0w |= 1u; nd_c0_src(); /* NAT1B: after the GPU calls */`.
2. nd_comb.c sync_combine_work: delete line 338 `    nd_c0w |= 1u; /* NAT1 */`. After the next line (`    nd_comb_apply(&o, ...);`) add `    nd_c0w |= 1u; nd_c0_src(); /* NAT1B: after the GPU calls */`.
3. nd_set.c nd_rnd_sync: delete line 235 `    nd_c0w |= 2u; /* NAT1 */`. After `    nd_render_apply(&rs, mixk, mixc, nd_ws.omode_h, m, sg);` add `    nd_c0w |= 2u; nd_c0_src(); /* NAT1B */`.
4. nd_tex_set.c nd_tile_bind: delete line 444. In the fast path, before `        return;` add `        if (nd_bsrc) nd_c0w &= ~4u; else nd_c0w |= 4u; /* NAT1B */`. After `    nd_tile_bind_slow(n);` add `    if (nd_bsrc) nd_c0w &= ~4u; else nd_c0w |= 4u; /* NAT1B */`.
5. nd_snap.c nd_snap_render: delete line 132 `    nd_c0w &= ~2u; /* NAT1 */`. Line 133 before: `    if (nd_sr_same(sg)) return;` after: `    if (nd_sr_same(sg)) { nd_c0w &= ~2u; return; } /* NAT1B */`. After the nd_render_apply line add `    nd_c0w &= ~2u; /* NAT1B */`.
6. nd_snap.c line 177 before: `void nd_snap_comb(void) { nd_c0w &= ~1u; /* NAT1 */ nd_comb_apply(&sc_cur.o, ...); }` after: `void nd_snap_comb(void) { nd_comb_apply(&sc_cur.o, ...); nd_c0w &= ~1u; /* NAT1B */ }` (the arguments stay).
7. nd_direct.c nd_direct_bind: delete line 69 `        nd_c0w &= ~1u; /* NAT1: the core 1 decision */`. Before the line `    nd_sr_forget(); /* nd_comb_sync of the stream path ...` add `    nd_c0w &= ~1u; /* NAT1B: the core 1 decision, after the GPU calls */`.
   nd_direct_tex: delete line 94. Line `    return nd_tex_bind_desc(d, 0);` becomes `    { const int r = nd_tex_bind_desc(d, 0); nd_c0w &= ~4u; return r; } /* NAT1B */`.
8. nd_int.h, a new line after `extern unsigned char nd_c0w; ...`: `void nd_c0_src(void); /* NAT1B: pm_gpu.c, the producer of a walker decision */`.
9. pm_gpu.c, after the NAT1 line `unsigned char nd_c0w; static unsigned c0_d, ...`, add these 4 lines:
   `extern const uint32_t* nd_stream_cur; static uint32_t c0_wk, c0_k[8]; static unsigned c0_kc[8], c0_kn, c0_ko; /* NAT1B */`
   `void nd_c0_src(void) { const uint32_t* s = nd_stream_cur; c0_wk = !s ? 0u : (s[0] >> 24) == 10u ? s[1] : (s[0] >> 24); } /* NAT1B: op, or the runner of NDC_NAPI */`
   `static void c0_who(void) { for (unsigned i = 0; i < c0_kn; i++) if (c0_k[i] == c0_wk) { c0_kc[i]++; return; } /* NAT1B */`
   `    if (c0_kn < 8u) { c0_k[c0_kn] = c0_wk; c0_kc[c0_kn++] = 1u; } else c0_ko++; }`
10. pm_gpu.c c0_count, line 1326: insert ` if (nd_c0w & 3u) c0_who(); /* NAT1B */` right after `c0_d++;` in that line. pm_gpu.c drec_end, after the c0src line add:
   `    for (unsigned i = 0; i < c0_kn; i++) pm_log_evt("c0who k %08x n %u", (unsigned)c0_k[i], c0_kc[i]); pm_log_evt("c0who over %u", c0_ko); /* NAT1B */`
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, 1 build, 6 Azahar starts of 8)
1. Edits 1-10. `sh 3ds/tools/nd_host_test.sh > build3ds/out/nat1b_host.txt 2>&1`: PASS (the 3 CACHECHECK lines are known, oracle_handoff.md:49). Commit `NAT1B 1: exact c0src, c0who`.
2. For B in spot_mac01_drec, entcape_kmr03_drec, fxparts_drec, one call each: `python 3ds/tools/bench.py base <B> > build3ds/out/nat1b_base_<B>.txt 2>&1`.
3. `sh 3ds/tools/rowloop.sh --row NAT1B --bench spot_mac01_drec,entcape_kmr03_drec,fxparts_drec --no-host --why "NAT1B: exact c0src, drec equal" > build3ds/out/nat1b_loop.txt 2>&1`.
4. `grep -H "c0src f\|c0who" build3ds/azt/logs/spot_mac01_drec.txt build3ds/azt/logs/entcape_kmr03_drec.txt build3ds/azt/logs/fxparts_drec.txt > build3ds/out/nat1b_c0src.txt`.
5. Names of the keys: `python -c "import sys; sys.path.insert(0,'3ds/tools'); import elfgate,re; s=elfgate.parse_elf('build3ds/lane-NAT1B/pm_3ds.elf')[0]; [print(k, next((n for n,v,z,t,_ in s if t==2 and v<=int(k,16)<v+max(z,1)), 'op')) for k in sorted(set(re.findall(r'c0who k (\w+)', open('build3ds/out/nat1b_c0src.txt').read())))]" > build3ds/out/nat1b_who.txt`.
6. `python 3ds/tools/c0gate.py build3ds/lane-NAT1B/pm_3ds.elf > build3ds/out/nat1b_c0gate.txt 2>&1`.
## Gate (exact)
Host PASS. Rowloop exit 0 (the 3 drec cmp equal). nat1b_c0src.txt has 3 c0src lines. c0gate last line `c0gate calls 127 interp 37 state 23`.
Fix rights: compile errors in the edited files. A DIFF, a missing line, another c0gate value or a tool fault: stop, send the output path to oracle NATIVE with SendMessage.
## Hand-back
Last commit `NAT1B 2: gate. 3 drec equal; exact c0src and c0who` with `git add -f build3ds/out/nat1b_c0src.txt build3ds/out/nat1b_who.txt build3ds/out/nat1b_loop.txt`. It deletes this page.
One SubagentHandback: the commit hash, the 3 c0src lines, nat1b_who.txt (key, name, count per bench).
