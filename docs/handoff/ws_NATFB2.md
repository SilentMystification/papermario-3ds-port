# ws_NATFB2 (Haiku, 1 file): the particle list fallback and the boot picture leave the unit interpreter
WAIT: `Merge NAT5G` and `Merge NATFB` (nd_walk.c is a NAT5G file; nd_geo_units comes from NATFB). Oracle NATIVE10 (affae1ea40e24e4cf). Worktree build3ds/wt/NATFB2, branch NATFB2. PM_ROW=NATFB2 PM_LANE=NATFB2.
Owns: 3ds/src/nd_walk.c (nd_pdraw_run, pm_gbi_fallback only). Haiku: 2 one-function edits, no design step.
Counter C13: c0gate calls -2 from the start commit (FACT the 2 sites nd_walk.c:457 nd_ntab_run, :655 nd_nr_list on 8288b39b57; GUESS start value 80 after NAT5G +2 and NATFB -2: read it). Interp and state as the start commit (nd_ntab_run keeps :232, :495; nd_nr_list keeps :231, :395).
## Facts
- FACT a particle list table (NDN_FL_PARTS) holds VTX at slot 0, then only VTX and TRI to END (nd_gtab.h:44, nd_gfx_conv.c parts_ok). nd_ntab_run (nd_walk.c:494) runs it with nd_lsr_geo plus ls_run, else nd_nr_exec: both give the nr_geo arms only.
- FACT the old boot picture is a FILL rectangle (0,0)-(319,239) of fill 0x2229 in G_CYC_FILL: nd_frect_draw gives pm_gpu_set_replace(1), pm_gpu_fill_rect(0, 0, 319, 239, rgba_from_5551(0x2229)), pm_gpu_set_replace(0) (nd_rect.c:316-347). The fill skips nd_img_check (GUESS: no boot frame sets another colour image). The test triangle goes (user yes). Caller: pm_main.c:464 (boot frames before the first drawn frame).
## Edits (new lines end with `/* NATFB2 */`; refresh the line numbers after the NAT5G merge, cite by function name)
1. nd_pdraw_run. before: `if (!nd_part_draw(t)) nd_ntab_run(t); }`. after: `if (!nd_part_draw(t)) { if (nd_ls_on) nd_lsr_geo(); nd_geo_units(t->u, t->nu); } } /* NATFB2: a refused particle list on the native geometry run */`.
2. pm_gbi_fallback: replace the whole body (the static dl, fv, fm, the ready block and the nd_nr_list call) with:
```c
void pm_gbi_fallback(void) { /* NATFB2 (user yes): the boot picture is one native fill, as the old FILL rectangle; no test triangle */
    pm_gpu_set_replace(1);
    pm_gpu_fill_rect(0, 0, 319, 239, rgba_from_5551(0x2229u));
    pm_gpu_set_replace(0);
}
```
   Keep the includes of nd_walk.c as they are.
## Steps (output to build3ds/out/, read the last 20 lines, Bash `timeout: 600000`, foreground only; 1 build, at most 8 Azahar starts)
1. Edits 1-2. Commit `NATFB2 1: particle fallback and boot picture native`.
2. `python 3ds/tools/bench.py base fxparts_drec > build3ds/out/natfb2_base.txt 2>&1`, then the same for spot_mac01_drec (append `>>`).
3. `sh 3ds/tools/rowloop.sh --row NATFB2 --bench fxparts_drec,spot_mac01_drec --why "NATFB2: parts and boot fallback native" > build3ds/out/natfb2_loop.txt 2>&1`.
4. `python 3ds/tools/c0gate.py build3ds/lane-NATFB2/pm_3ds.elf > build3ds/out/natfb2_c0.txt 2>&1`. Also run it on the start-commit ELF of the bench base lane. Expect calls -2, interp and state equal.
## Gate (exact): LOOP PASS (host PASS, build OK, 2 drec byte-equal; GUESS the drec windows hold no boot frame) and the c0gate delta of step 4. Fix rights: compile errors, unused-variable warnings, the count in 3ds/tests/lkey/allow.txt.
A DIFF, a crash, another c0gate delta or a tool fault: stop. SendMessage the output path and the first DIFF line to oracle NATIVE10 (affae1ea40e24e4cf). Wait for the reply.
## Hand-back: last commit `NATFB2 2: gate. <LOOP lines> <c0gate lines>` with `git add -f build3ds/out/natfb2_loop.txt build3ds/out/natfb2_c0.txt`. It deletes this page. One SubagentHandback: hashes, cmp line per bench, c0gate lines.
