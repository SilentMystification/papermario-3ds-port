# ws_DUPMS (Haiku: deletion, byte-equal): every model record has its snap; delete the nd_c0 fallbacks, nd_c0_to_ws and the dead cdiff code
Designer: oracle NATIVE15 (plan_perf.md sections 2, 3, 7). Branch from main. Worktree build3ds/wt/DUPMS, branch DUPMS. PM_ROW=DUPMS PM_LANE=DUPMS.
Facts (main c23333bb73):
- The only writer of an NDC_MODEL header is nd_rec_model (nd_rec.c:225). It always reserves the snap room (:221) and always calls nd_msnap_rec (:227),
  which sets header bit 0 (nd_msnap.c:26) and the magic (nd_msnap_in.c:14). So ms_of (nd_msnap.c:32) never returns NULL on the target.
- So these paths never run: the nos fallback with nd_c0_to_ws and a 1.4 KB static NdDs (nd_msnap.c:29, :39), and the nd_c0 key words with no snap
  (pm_gpu.c:3829, :3866). nd_c0_to_ws (nd_state.c:46-51) has no other caller. nd_snap_cdiff and nd_cd_* (nd_snap.c:240-245) have no caller since PERF3.
Objective: delete duplicate state copies before the nd_c0 rows. c0gate stays 0 / 0 / 1. No prof value moves.
Edit only (line numbers main c23333bb73): nd_msnap.c:17, :29-41, nd_state.c:46-51, nd.h:191, pm_gpu.c:3829, :3866, nd_snap.c:240-245. Sources are CRLF.
## Edits
1. nd_msnap.c:29-30 `static unsigned ms_nos, ms_nlog;` and `static NdDs ms_d;` become
   `static unsigned ms_nos; static __attribute__((cold, noinline)) void ms_none(void) { if (ms_nos++ < 4u) pm_log_e("msnap: model record with no snap f%u", pm_frame_n); } /* DUPMS */`
   nd_msnap_use (:37-41) becomes
   `void nd_msnap_use(const uint32_t* q) { const NdMSnap* s = ms_of(q); if (!s) { ms_none(); return; } ms_cur = s; } /* DUPMS: nd_rec_model writes every snap */`
   If pm_gpu_env_now or nd_ap lose their last use in this file, delete their declarations (pm_gpu_env_now :17) as well.
2. nd_state.c: delete :46-51 (nd_c0_to_ws). nd.h: delete :191 (`void nd_c0_to_ws(NdWs* d);`).
3. pm_gpu.c:3829 before: `const uint32_t w5[5] = { nd_apw(NDAPW_OH), nd_apw(NDAPW_OL), nd_c0.om_kh, nd_c0.om_kl, nd_c0.geom }; if (!kw) kw = w5;`
   after: `static const uint32_t w5[5]; if (!kw) kw = w5; /* DUPMS: never on the target */`. :3866 the same edit (its w5 reads nd_c0.omode_h, omode_l, om_kh, om_kl, geom).
4. nd_snap.c: delete from the comment `/* NAT1D: 1 when a walker decision differs` (:240) to the last line of nd_snap_cdiff (:245). Keep nd_snap_primz and nd_snap_fill.
## Checklist (hw_guidelines.md)
- 1.4 KB less BSS (ms_d). Fewer loads of nd_c0 on the model hit path (pm_gpu_model_keyb runs per model record, about 93 per frame).
## Tests (output to build3ds/out/, read the last 20 lines; 1 build, at most 2 Azahar starts)
1. `grep -rnw "nd_c0_to_ws\|nd_snap_cdiff\|nd_cd_w\|ms_d" 3ds/src 3ds/include 3ds/tools > build3ds/out/dupms_dead.txt`: empty.
2. `sh 3ds/tools/nd_host_test.sh > build3ds/out/dupms_host.txt` (timeout 600000): PASS. Commit `DUPMS 1: model snap always present, fallbacks go`.
3. `PM_LANE=DUPMS sh 3ds/build.sh`; `python 3ds/tools/c0gate.py build3ds/pm_3ds.elf`: 0 / 0 / 1.
4. Base, run and cmp as ws_MDLBLK step 3, benches spot_mac01_drec,entcape_kmr03_drec: every shot byte-equal. Pass also: no `msnap:` line in either log.
Fix rights: the edited lines, compile errors, a now unused static or declaration (delete it, name it in the commit).
A DIFF or a `msnap:` line: stop, send the output path to oracle NATIVE15.
Stop: last commit `DUPMS 2: gate` deletes this page. One SubagentHandback: commit hash, host, c0gate, cmp, the deleted line count.
