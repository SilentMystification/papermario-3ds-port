# ws_GD7 (Sonnet): HUD rect record memo: an unchanged HUD rect copies its last stream block (GAMEDRAW row 7)
Designer: oracle NATIVE18 (plan_gamedraw.md 1.4, 4). Worktree build3ds/wt/GD7, branch GD7. PM_ROW=GD7 PM_LANE=GD7. Start: main after GD0B merges.
WAIT: start only on the user's Sonnet yes and oracle NATIVE17's yes for the alt record copy (nd_rec.c is its recorder). Do not edit nd_rec_alt or nd_rec_model.
Why Sonnet: the memo key must hold every input of the HUD rect path; the census names missing fields and the coder adds them (a design loop, as ws_DO1).
Objective: the status bar draws about 12 HUD rects per step with the same NdHudRec each step. Each runs hud_rect_direct, hud_rect_emit, nd_snap_pso (a full
NdDs copy and a snap build) and nd_rec_alt (its units through nd_rec_nr). A hit copies the last block instead. Counter: console c1x mode and c1v mode
(GD0B lines), core 1 busy; GUESS -0.8 to -2 ms per step. Stream bytes unchanged.
## Facts (FACT code read)
- The path: src/hud_element.c:873-879 (hud_rect_direct :472, hud_rect_emit :285 into a static 1024-word buffer, then nd_dir_rec_alt or nd_rec_alt).
  nd_dir_rec_alt (3ds/src/nd_direct.c:257) = nd_snap_pso (nd_snap.c:124-131, reads all of nd_ds) + nd_rec_alt (nd_rec.c:502-514).
- nd_rec_alt writes a NAPI record (header, run, the NdHudRec units, ab[0..1]), a JUMP br[0..1], then the alt units (nd_rec_nr, nd_rec_texrect).
  ab[0], ab[1] and br[1] are absolute stream addresses. It sets nd_ds.sc_lost, runs sh_all and nd_snap_task, and adds to nd_altw[3], nd_altr_n, ndc_n.
- The stream of the other display context (last step) stays intact until core 1 records into it again (plan_drawobj.md, ws_DO1 facts).
## Design
- New file 3ds/src/nd_hmemo.c, one call from hud_element.c:873-879 around the else branch. Table: 32 entries on the heap, key = the NdHudRec bytes
  (64 B + 48 B x npc, hud_element.c:271-281) hashed to a slot, then a full compare of: those bytes, the NdDs before (memcmp of nd_ds), and the snap memo
  state the census names. Entry: the block pointer and size in the other context's stream, the NdDs after, the counter deltas.
- Hit: copy the block to rec_take room (one memcpy), patch the address words (ab[0], ab[1], br[1] and any word the census names), nd_ds = the stored
  after value, add the counter deltas. Miss: the full path, then store. Reset at a map load and when the entry's context is not the other one.
## Steps
1. `GD7 1`: the full path always runs. In drec frames, on a key hit, build the copy into a scratch buffer and memcmp it with the real block; memcmp
   nd_ds after with the stored value. A difference logs `hmemo f<n> diff <off>` (NdDs offset by offsetof, or `blk <word>`). Add that field to the key
   or the patch list; rerun. 3 rounds, then send the lines to oracle NATIVE18.
2. `GD7 2: gate`: the hit path writes the copy. The drec-frame compare stays (a hit still compares in drec frames).
## Checklist (hw_guidelines.md 2, 3, 7): heap table (about 45 KB GUESS), one sequential copy per hit, no division, no per-frame malloc.
## Tests (build3ds/out/, last 20 lines). Each gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. Build; `python 3ds/tools/c0gate.py <elf>` 0 / 0 / 0.
2. Step 1: hudfb_drec, spot_mac01_drec byte-equal; `grep -c "hmemo f.*diff"` 0; note the hit count line. Step 2: the same 2 benches byte-equal.
Fix rights: compile errors; fields added to the key or the patch list from a census line (3 rounds). A DIFF in step 2, a crash: stop, send the path to
oracle NATIVE18.
Stop: `GD7 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hashes, host, the cmp lines, the hmemo lines, the fields added.
