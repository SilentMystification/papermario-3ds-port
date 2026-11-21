# ws_DEC0: gate tools for the overlay outline (plan_decal.md section 6)
Row DEC0. Model Haiku. Start main. Worktree build3ds/wt/DEC0, branch DEC0, PM_ROW=DEC0, PM_LANE=DEC0. Oracle NATIVE25.
Objective: add 3 mechanical gates. Change no game code. Counters C1..C12: none. Row metric: the tex_fringe line
(GUESS before: CI4 35581, CI8 1324, RGBA16 2730, IA4 7836, IA8 1966, IA16 368, RGBA32 512, madotif 46). DEC0 records the FACT values.
Files (all new, 3ds/tools only): tex_fringe.py, drec_state.py, azt.d/DEC0.json, bench.d/DEC0.json. Edit no other file.
## 1. tex_fringe.py `<asset_base> [--fill] [--list N]` or `--selftest` (shape: texkey_bake.py, read only)
- Import texkey_bake (sys.path 3ds/tools). Read the ROM as texkey_bake.py:187. Apply bake() patches to a bytearray copy (the game does it at pm_os.c:506).
- Walk the records as texkey_bake.py:99-127. Take each ROM address once. Unpack the header as :128. Decode the main image:
  CI4, CI8 (palette after the raster, mip size as :141-143), RGBA16, RGBA32 (A > 0 opaque), IA4 (3 bit I, 1 bit A), IA8 (high nibble I,
  low nibble A, A > 0 opaque), IA16 (A byte > 0 opaque). CI and RGBA16 keep 5-bit channels. The other formats use 8-bit channels.
- Bad texel: alpha 0, at least 1 opaque 8-neighbour inside the image, a channel more than 1 (5-bit) or 8 (8-bit) off floor(sum / c) of them.
- Seam texel: alpha 0 on the image border, the json wrap of that axis is "Repeat", and the texel on the opposite border is opaque.
- --fill: first give each alpha-0 texel of CI4, CI8, RGBA16 with an opaque 8-neighbour the value floor(sum / c) (opaque texels never change).
- Print `fringe: textures T flagged F bad B seam S CI4 n CI8 n RGBA16 n RGBA32 n IA4 n IA8 n IA16 n`. --list N adds `ft <archive> <name> <fmt> <w>x<h> bad <n>`.
- --selftest: a 3x3 image, centre opaque (31,0,0), the rest alpha 0 colour 0. Expect bad 8. After --fill expect 0. After a 4-neighbour
  fill (the seeded fault) expect 4. Print `tex_fringe selftest PASS` (exit 0) or FAIL (exit 1).
## 2. drec_state.py `census|tdiff|selftest`
- drec.txt (nd_drec.c:31-58): `draw <seq> ...`, `r <reg> <mask> <val>`, `t <u> type <ty> dim <d> bytes <n> h <hash|gpu>`, `v`, `i`, `m`, `end`, `frame <F>` (pm_gpu.c:661).
- Files: two explicit paths, or `--bench B --row R`. Row file: `bench.row_shots_run(B, R)["shot_path"]/drec.txt`. Base file:
  `bench.store_root()/bench.base_commit(None)/B/drec.txt` (import bench from 3ds/tools).
- census [--all]: one line per draw with r 104 bit 0 set, or r 101 not 01010000, or r 04e not 0 (every draw with --all):
  `ds <seq> fr <F> at <104> bl <101> dp <107> dm <04e> p0 <083> f <(p0>>1)&1> ws <(p0>>12)&3> wt <(p0>>8)&3> td <dim> ty <type> th <hash>`.
  Last line: `dstate draws N listed L atest A blend B dmoff D lin X`.
- tdiff: a different draw count prints `tdiff count base N row M` and exits 1. Per block pair: equal lines = same, only t lines differ = tex,
  else other. A tex block with r 104 bit 0 clear and r 101 = 01010000 counts as tex_opaque.
  Print `tdiff draws N same S tex T tex_opaque O other X` and the first 10 seqs of tex_opaque and of other. Exit 0 only when X = 0 and O = 0.
- selftest: a t change on an alpha-test draw exits 0. An r 104 change (other 1) and a t change on an opaque draw (tex_opaque 1) exit 1. Print `drec_state selftest PASS`.
## 3. Scenario and bench (copy the shape of azt.d/HOSTCORE2.json and bench.d/HOSTCORE2.json)
- azt.d/DEC0.json: name kmr11_drec, debug `warp=kmr_11:0 story=-120 det drec=221:3:20 loglevel=debug`, until `drec f261 draws [1-9]\\d* bytes \\d+`,
  cap 30, deadline 25, first_marker `warp: kmr_11 entry 0`, expects warp, needs_picture false, events: that marker, then the until.
- bench.d/DEC0.json: name kmr11_drec, kind azahar_pic, speed normal, default false, cap 30, the same until, tags azahar det drec, gates [].
## Gate (1 build, 3 Azahar runs)
1. `python 3ds/tools/tex_fringe.py --selftest` and `python 3ds/tools/drec_state.py selftest`: both PASS.
2. `python 3ds/tools/tex_fringe.py .`, then `--fill`, then `--list 10`. Record the lines. With --fill expect CI4 0 CI8 0 RGBA16 0.
3. `PM_LANE=DEC0 sh 3ds/build.sh`. `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. c0gate: calls 0 interp 0 state 0.
4. `python 3ds/tools/bench.py base kmr11_drec --commit $(git merge-base HEAD main)`: 2 equal runs.
5. `bench.py run kmr11_drec --row DEC0 --why "DEC0 gate"`, then `bench.py cmp kmr11_drec --row DEC0`: byte-equal.
6. `drec_state.py census --bench kmr11_drec --row DEC0`: at least 1 line with `td 00100020` and `at 00003061`. Record the dstate line.
Fix scope: edit only the 4 new files. Stop: the census has no such line, or step 4 or 5 differs: SendMessage NATIVE25 with the output path.
Hand-back: commit hash, the 3 fringe lines, the dstate line, the cmp result. The last commit deletes this page.
