# ws_PROF3 (Haiku): core 1 draw timers by game subsystem (prof line c1g), no behaviour change
Designer: oracle NATIVE17 (plan_perf.md section 11). Start now from main. Parallel with MSNAP2 (other files). Worktree build3ds/wt/PROF3, branch PROF3.
PM_ROW=PROF3 PM_LANE=PROF3. Lines: main f0a9f642e4; grep the quoted text. Why Haiku: timer pairs at named call sites and one log line; the gate is byte-equal.
Objective: split core 1 `c1s dl 40.60` (FACT build 7680) into exclusive classes. The existing timers cover 15.97 ms (snap, lsc, alt, rmdl, rent, rspr, rnr).
Counter: the new line `prof f<n> w<n> c1g ent .. mdl .. ply .. npc .. wrk .. fx .. task .. thud .. ui ..` (ms per sampled step, 2 decimals).
## Facts (FACT code read)
- The timers are exclusive: pm_gpu_ndt_pop (pm_gpu.c:1185-1192) subtracts the nested time. NDT_B / NDT_E (nd.h:457-459) run only when pm_ph_on.
  The stack has 4 levels (pm_gpu.c:1182). A class here is level 1, rmdl/rnr level 2, snap and alt level 3.
- render_frame (src/cam_main.c:222-234, the non-HUD camera) calls render_entities, render_models, render_player, render_npcs, render_workers_scene,
  render_effects_scene (they queue render tasks), then execute_render_tasks (the appendGfx draw code that records), then render_transformed_hud_elements.
- gfx_draw_frame (src/main_loop.c:190-280) runs render_frame(false) at :208 and render_frame(true) at :232. The UI calls run from :212 to :228 and from :236 to the end.
## Edits
1. nd.h:451 enum: before `NDT_N` add `NDT_GENT, NDT_GMDL, NDT_GPLY, NDT_GNPC, NDT_GWRK, NDT_GFX, NDT_GTASK, NDT_GTHUD, NDT_GUI,` (PROF3).
2. src/cam_main.c:222-234: wrap each of the 8 calls. Example before `render_models();` after `{ NDT_B(t_); render_models(); NDT_E(t_, NDT_GMDL); }`.
   Map: render_entities GENT, render_models GMDL, render_player GPLY, render_npcs GNPC, render_workers_scene GWRK, render_effects_scene GFX,
   execute_render_tasks GTASK, render_transformed_hud_elements GTHUD (both calls of it, :233 and :244). Keep the FI_OPEN/FI_CLOSE lines as they are.
3. src/main_loop.c: after the FI_CLOSE that follows render_frame(false) (:209-210) open `{ NDT_B(tu_a);` and close `NDT_E(tu_a, NDT_GUI); }` just before
   `FI_OPEN("gfx_world_hud");`. After the FI_CLOSE that follows render_frame(true), open `{ NDT_B(tu_b);`. Close `NDT_E(tu_b, NDT_GUI); }` before the last `}` of gfx_draw_frame.
4. pm_phase.c after the c0r line (:635): `pm_log_prof_line("prof f%u w%u c1g ent %u.%02u mdl %u.%02u ply %u.%02u npc %u.%02u wrk %u.%02u fx %u.%02u task %u.%02u thud %u.%02u ui %u.%02u", pf_total, pf_w, CS(cm[NDT_GENT]), ..., CS(cm[NDT_GUI]));`
## Checklist (hw_guidelines.md 9): the timers run in sampled steps only. They add 18 SVC pairs per camera pass. The line stays under the 256-byte log slot.
## Tests (build3ds/out/, last 20 lines). Gate: 1 build, 2 Azahar runs. Base: `bench.py base <b> --commit $(git merge-base HEAD main)`.
1. Host `sh 3ds/tools/nd_host_test.sh` (timeout 600000) PASS. `PM_LANE=PROF3 sh 3ds/build.sh`; c0gate unchanged.
2. `bench.py run spot_mac01_drec,entcape_kmr03_drec --row PROF3 --why "PROF3 gate"`; `bench.py cmp <b> --row PROF3` byte-equal for both.
3. Capture check (one Azahar run): `python 3ds/tools/capture_check.py --row PROF3 --proof "c1g ent,c1d rmdl,c1s logic" warp=mac_01:0 pos=0:0:200 profdisk=1`. Need CAPTURE CHECK OK.
Fix rights: compile errors (an include of nd.h or the 2 prototypes and 3 macros of nd.h:457-463 in a src file), a mechanical tool argument fault.
A DIFF, a missing proof line, a crash: stop, send the path to oracle NATIVE17.
Stop: last commit `PROF3 2: gate` deletes this page. Hand-back: one SubagentHandback: commit hash, host, the 2 cmp lines, the CAPTURE CHECK line and the Azahar c1g line.
