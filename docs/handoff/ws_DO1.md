# ws_DO1 (Sonnet): core 1 model record memo: an unchanged model copies its last stream block (DRAWOBJ row 1)
Designer: oracle NATIVE17 (plan_drawobj.md section 6). User rule: the user reads plan_drawobj.md before any DRAWOBJ coder starts. Parallel with DO0 (other files).
Worktree build3ds/wt/DO1, branch DO1. PM_ROW=DO1 PM_LANE=DO1. Lines: main 5beccb695c or later; grep the quoted text.
Why Sonnet: the memo key must hold every input of nd_ds_model and nd_msnap_rec; the census names missing fields and the coder adds them (a design loop).
Objective: rmdl (c1d, FACT 4.33 ms per step on the console) to about 1 to 1.5 ms (GUESS) with the stream bytes unchanged. Counter: drec end line
`mmemo f<n> models <m> hits <h>`; console c1d rmdl.
## Facts (FACT code read, nd_rec.c:226-240)
- nd_rec_model writes one block: header p[0], key A p[1] (nd_mkey_a(q)), the record words (G_PM_REC_WORDS pairs and the kept R13 pairs), a jump j[0..1]
  whose j[1] is an absolute stream address, then the model snap (ND_MSNAP_U pairs, nd_msnap_rec). Before it, nd_msnap_pre and nd_ds_model change nd_ds.
- The block depends on: the record words, the core 1 state before (nd_ds, read by nd_ds_model and nd_msnap_take), the decision state (sn_leg, sn_pat,
  sn_mode5, sn_vprim, the SD table: nd_snap_dec), and the light data the R3 pointer names (rec_lights reads it, nd_rec.c of core 0; nd_ds_model on core 1).
- The record words R2 w1 (final matrix) and R8 (billboard) point into the display context: they alternate between the 2 contexts each step.
- The stream of the other display context (the last step) stays intact until core 1 records into it again (nd_rec.c:170-180 ch_enter per context).
## Design
- Memo table: 256 entries, open addressing, key = q[24] (R12 w0, the geometry address; page fault fixed) and q[0]. Entry: the last block pointer and size (in the other context's
  stream), the record words with R2 w1, R8 w0 and w1 set to 0, the before snapshot S (an NdMSnap from nd_msnap_take of nd_ds before, ms_pre's 5 words,
  sn_leg, sn_pat, sn_mode5, sn_vprim, the light block bytes R3 names), and the after values A that nd_ds_model writes. Reset at a map load and when
  the entry's context is not the other one.
- Rings (oracle reply round 1): S holds tst1.ldrec and blk in logical order (`[(n + i) & 3]`, i 0..3), not ldrec_n, blk_n. A holds the loads the model
  appended (count and entries); a hit appends them at the current n as nd_ds_loadtile does.
- Hit (all equal): copy the last block into the new room (one memcpy), set R2 w1, R8 from q, j[1] to the new address, p[1] = nd_mkey_a(q), write A into
  nd_ds, run the side effects of nd_snap_dec (sn_leg, sn_mode5, sn_pat, sn_cmds = ~0u: keep its decision call, it is cheap now). Miss: the full path, then store.
## Steps
1. `DO1 1`: the full path always writes the block. The memo runs beside it in drec frames: on a hit it builds the block into a scratch buffer and memcmps it
   with the real block, and memcmps nd_ds after against a copy. A difference logs `mmemo f<n> diff <off>` (the first NdDs byte offset or `blk <word>`).
   Add the field at that offset to S or A (use offsetof to name it); rerun. 3 rounds, then send the lines to oracle NATIVE17.
2. `DO1 2: gate`: the hit path writes the copy. The census code stays as a debug switch-free drec-frame check (a hit still compares in drec frames).
## Checklist (hw_guidelines.md 2, 3, 7): the table is on the heap (about 45 KB GUESS); one sequential copy per hit; no division; no per-frame malloc.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS (fix host expectations that count nd_rec_model work). Build; c0gate 0 / 0 / 0.
2. Step 1: spot_mac01_drec, entcape_kmr03_drec byte-equal; `grep -c "mmemo f.*diff"` 0; note the mmemo hits line. Step 2: fxparts_drec, hudfb_drec byte-equal.
Fix rights: compile errors; fields added to S or A from a census line (3 rounds). A DIFF in step 2, a crash: stop, send the path to oracle NATIVE17.
Stop: `DO1 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, the 4 cmp lines, the mmemo lines, the fields added to S and A.
