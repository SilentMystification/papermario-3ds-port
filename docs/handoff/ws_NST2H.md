# ws_NST2H (Haiku): dschk names the source of each epbad command (core 0 site or core 1 rule, and the unit id) (stage S3 end; counter: epbad, by source)
Designer: oracle FIN. WAIT: branch from main after the NDR5 merge (NDR5 edits nd_rec.c). Worktree build3ds/wt/NST2H, branch NST2H. PM_ROW=NST2H PM_LANE=NST2H.
Edit only: 3ds/src/nd_rec.c (ep rule lines), 3ds/src/nd_walk.c (4 epoch lines), 3ds/src/nd_dschk.c, 3ds/tools/fxrest.d/dschk_base.txt.
## Facts (code read, main 268490a300)
- epbad: core 0 nd_st_epoch != 0 differs from the core 1 ep bit (nd_dschk.c:122). NST2G gate (268490a300): bad 0, epbad 2436/16822/11076/69440 (spot, wscmb, axns, fxchk_a).
- Core 0 raises nd_st_epoch at 4 sites only (grep): nd_walk.c:159 model record, :287 op_typed, :301 op_geo MOVEMEM, :357 op_ctl.
- Core 1 sets ds_ep at 5 sites only (grep): nd_rec.c:252 rule a (field change), :253 rule b (sc_lost), :254 rule c (MOVEMEM), :266 model record, :456 list marker. rec_take writes ds_ep into the ep word (:196) and clears it (:199). The ep word is 0 or 1 today; nd_dschk.c reads it as `ep != 0`.
## Edits
1. nd_rec.c after :178: `static uint32_t ds_epw; /* NST2H: rule << 8 | unit id of the first ep rule since the last dschk record */` and
   `#define DS_EP(r, i) do { if (!ds_ep) ds_epw = (uint32_t)(r) << 8 | ((i) & 0xffu); ds_ep = 1; } while (0)`.
   Before (:252): `... id != NDO_POPMTX) ds_ep = 1;` After: `... id != NDO_POPMTX) DS_EP(1, id);`. The same at :253 `DS_EP(2, id)`, :254 `DS_EP(3, id)`, :266 `DS_EP(4, 0)`,
   :456 `DS_EP(5, 0)`. :196 `ds_rec[sizeof nd_ds / 4u] = ds_ep ? (1u | ds_epw << 8) : 0u;`. After :199 add `ds_epw = 0;`.
2. nd_walk.c: after :282 `unsigned nd_st_why; /* NST2H: site << 8 | unit id of the first nd_st_epoch raise since the last dschk record */`, at :152 `extern unsigned nd_st_why;`. Before each `nd_st_epoch++` put `if (!nd_st_epoch) nd_st_why = <v>;` with v: :159 `4u << 8`, :287 `1u << 8 | id`,
   :301 `2u << 8 | id`, :357 `3u << 8 | id`. Before (:287): `gv_epoch++; nd_st_epoch++; /* GV1b */` After: `gv_epoch++; if (!nd_st_epoch) nd_st_why = 1u << 8 | id; nd_st_epoch++; /* GV1b; NST2H source */`.
3. nd_dschk.c: `extern unsigned nd_st_why;` (nd_walk.c). Add above nd_dschk_run:
   ```
   static uint32_t ek_key[32], ek_n[32]; static unsigned ek_used, ek_other, ek_nlog;
   static void ep_note(uint32_t key) {   /* NST2H: one epbad command, key = side << 16 | site or rule << 8 | unit id */
       if (ek_nlog < 16u) { ek_nlog++; pm_log_evt("dschk epbad f%u k %x", pm_frame_n, (unsigned)key); }
       for (unsigned i = 0; i < ek_used; i++) if (ek_key[i] == key) { ek_n[i]++; return; }
       if (ek_used < 32u) { ek_key[ek_used] = key; ek_n[ek_used++] = 1u; } else ek_other++;
   }
   static void ep_print(void) {   /* NST2H: the counts by key, 8 per line, then the overflow */
       char s[128]; unsigned k = 0; s[0] = 0;
       for (unsigned i = 0; i < ek_used; i++) {
           k += (unsigned)snprintf(s + k, sizeof s - k, " %x=%u", (unsigned)ek_key[i], (unsigned)ek_n[i]);
           if ((i & 7u) == 7u || i + 1u == ek_used) { pm_log_evt("dschk epsrc f%u%s", pm_frame_n, s); k = 0; s[0] = 0; }
       }
       pm_log_evt("dschk epsrc f%u other %u", pm_frame_n, ek_other);
   }
   ```
   :101 becomes `const unsigned epw = rec[sizeof(NdDs) / 4u], ep = epw & 1u;`. :122-123 become
   `if ((nd_st_epoch != 0) != (ep != 0)) { dk_epbad++; ep_note(nd_st_epoch ? (0x10000u | (nd_st_why & 0xffffu)) : (0x20000u | ((epw >> 8) & 0xffffu))); }`
   `nd_st_epoch = 0; nd_st_why = 0;`. Call `ep_print();` just before the pm_log_evt of :126 (the until-line stays last) and before the end line of :133.
## Steps and tests (output to build3ds/out/, read the last 20 lines; 1 build, 6 Azahar starts; the 4 dschk scenes of azt.d/NST3.json)
1. Edits 1-3. `sh 3ds/tools/nd_host_test.sh` PASS. Commit `NST2H 1: dschk epbad source keys`.
2. bench.py base spot_mac01_drec,wscmb_pause_drec (merge-base, not charged). rowloop --row NST2H --bench the same 2: LOOP PASS, byte-equal.
3. bench.py run the 4 dschk scenes; `grep -a "dschk"` of each log > build3ds/out/nst2h_dschk.txt; write it into dschk_base.txt (replace). Pass: bad 0 and epbad equal to
   NST2G in the last counter line of each; the last epsrc counts plus `other` sum to epbad. Else: commit the grep, send the output path to the oracle, wait.
Fix rights: the listed files; compile errors. A DIFF, a crash: stop, send the output path to the oracle. Wait. Stop: last commit `NST2H 2: gate. dschk bad <4> epbad <4>; top key per scene <4>` deletes this page. One SubagentHandback with the epsrc lines of the 4 scenes.
