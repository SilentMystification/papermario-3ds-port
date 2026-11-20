# ws_GD1 (Haiku): fps60 record: count equal nodes with a hash table, not a scan of all nodes (GAMEDRAW row 1)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.1). Worktree build3ds/wt/GD1, branch GD1. PM_ROW=GD1 PM_LANE=GD1. Start: main 4d12301af5 or later; grep the quoted text.
Why Haiku: one function changes by the snippet below; a host test checks it against the old code. Objective: pm_interp_open (3ds/src/pm_interp.c:59-67) reads all nodes of the step per open (:63-64): about 77K compares per step (FACT 392 nodes, Azahar).
After: 1 to 2 table probes per open, node ids equal bit for bit. Counter: console core 1 busy (lgms - bw), GUESS -1.5 to -3.5 ms per step with fps60.
## Edits (3ds/src/pm_interp.c; CRLF file: use the Edit tool)
1. After `#define NT_SIZE 2048` (:20) add `#define OT_SIZE 2048 /* GD1: 2 x MAX_NODES */`. After `static short* ntab;` (:47) add
   `static short* otab; /* GD1: index + 1 of the last node with this (parent, a, b) */` and `static unsigned short* nocc; /* GD1: occurrence of each node */`.
2. Before pm_interp_open add `static unsigned occ_slot(unsigned p, unsigned a, unsigned b) { return ((p * 2654435761u) ^ (a * 2246822519u) ^ (b * 3266489917u)) >> (32 - 11); }`
3. Before (:62-65):
       unsigned parent = nodes[stack[depth]].id, occ = 0;
       for (int i = 0; i < nnode; i++)
           if (nodes[i].parent == parent && nodes[i].a == a && nodes[i].b == b) occ++;
       nodes[nnode] = (Node){ hash4(parent, a, b, occ), parent, a, b, 0 };
   After:
       unsigned parent = nodes[stack[depth]].id, occ = 0, h = occ_slot(parent, a, b);
       for (; otab[h]; h = (h + 1u) & (OT_SIZE - 1u)) { /* GD1: the slot of this key, or the first free slot */
           const int k = otab[h] - 1;
           if (nodes[k].parent == parent && nodes[k].a == a && nodes[k].b == b) { occ = nocc[k] + 1u; break; }
       }
       otab[h] = (short)(nnode + 1);
       nocc[nnode] = (unsigned short)occ;
       nodes[nnode] = (Node){ hash4(parent, a, b, occ), parent, a, b, 0 };
4. malloc (:93-94): add `+ OT_SIZE * sizeof(short) + MAX_NODES * sizeof(unsigned short)`. After `ntab = ptab + PT_SIZE;` (:102) add `otab = ntab + NT_SIZE; nocc = (unsigned short*)(otab + OT_SIZE);`.
5. pm_interp_start, after `stack[0] = nnode++;` (:109) add
   `memset(otab, 0, OT_SIZE * sizeof(short)); otab[occ_slot(0, 0x67616d65u, 0x6c6f6769u)] = 1; nocc[0] = 0; /* GD1: the root is node 0 */`.
6. File end: `unsigned pm_interp_node_id(int i) { return (i >= 0 && i < nnode) ? nodes[i].id : 0u; } /* GD1: iocc host test */` and `int pm_interp_node_n(void) { return nnode; }`.
7. New 3ds/tools/pm_interp_occ_test.c. Stubs: `PmOpt pm_opt;`, `pm_debug_has` (1 for "fps60" only), empty pm_gpu_kc_cut, pm_interp_drop_keys, pm_log,
   pm_log_d, pm_log_e. A reference: a copy of hash4 and of the old open and close (:59-74: node list, stack, depth, skipped, the 2 caps, the old scan).
   5 frames: pm_interp_start; 4000 steps of `r = r * 1103515245u + 12345u; v = r >> 16;` (r starts at 1): v % 8 < 4 opens with
   a = 1 + (v >> 3) % 3, b = (v >> 5) % 4, else closes; pm_interp_stop. The library and the reference get the same calls. Per frame compare
   pm_interp_node_n and every pm_interp_node_id(i) with the reference. The reference counts capn (opens refused at MAX_NODES) and capd (at the depth cap).
   Print `iocc frames 5 nodes <sum> capn <x> capd <y> bad <b>`; exit 1 when b > 0, x == 0 or y == 0. `--fault`: XOR 1 into reference id 7 of frame 0.
8. New 3ds/tests/host.d/iocc.sh, the form of 3ds/tests/host.d/nset1.sh: gcc $F the test and 3ds/src/pm_interp.c; the --fault run must fail with
   `bad [1-9]`, then print `iocc seed ok`; then the real run.
## Tests (output to build3ds/out/, read the last 20 lines). 1 build, 2 Azahar runs. 1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000): PASS with `iocc seed ok` and `iocc frames 5 ... bad 0` (capn and capd above 0).
2. `PM_LANE=GD1 sh 3ds/build.sh`; `python 3ds/tools/c0gate.py <elf>`: calls 0 interp 0 state 0.
3. `python 3ds/tools/bench.py base spot_mac01_fps60_det --commit $(git merge-base HEAD main)`, then
   `bench.py run spot_mac01_fps60_det --row GD1 --why "GD1 fps60 node ids equal"` and `bench.py cmp spot_mac01_fps60_det --row GD1`: byte-equal.
Fix rights: compile errors of the test, its stubs and iocc.sh. capn or capd 0: raise the steps per frame (up to 20000). `bad` above 0 in the real run, a DIFF, a base fault: stop, send the output path to oracle NATIVE18.
Stop: commit `GD1 1: gate. <host> <c0gate> <cmp line>` deletes this page. Hand-back: one SubagentHandback: commit hash, host line, c0gate, cmp line.
