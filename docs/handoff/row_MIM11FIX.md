# Row MIM11FIX: map models with a TEXEL1 plan and lighting or fog are refused and not drawn (mim_11 floor)

(Model: Haiku. Bug: user fact (console, after R13NOP): the floor of mim_11 does not render.)

## Cause (oracle code read, main a8fb2277e; census MIM11CEN f300)
mim_11: `ndm fb 266 why 6:266`, `mdlw why 6:100`, `mdlsub d 266`, `mdlfb 327fdf60 sub - 50`, `327f5f40 sub - 50`; iwa_10 (floor
renders) `ndm fb 0`. why 6 sub 4 is nd_model.c:358: `combine == 8 && co.plan.uses_tex1 && (!c->u1 || (c->gv && c->gv->fxon))` with
fxon: the plan reads TEXEL1 and the draw is a pm5 draw (lighting or the GPU fog). The census prints sub 4 as '-' (nd_model.c:247,
"-abcd"[s & 3]). Since MDL-5e a model record the native path refuses is not drawn (nd_recs.c:191): two map models of mim_11 (the
mdlfb producers) never draw. The refusal is older than FB-3: pm5 has the unit 1 uv output (3ds/shaders/pm5.v.pica:164-166, uniform
uvx) and the submit writes uvx to the pm5 uniform for every program 5 draw (pm_gpu.c:1067-1069, :1341-1342; gv5_on needs u5_uvx,
:1638-1639). So the fxon part of the test refuses a draw the path can make.

## Fix
nd_model.c:358: drop the fxon part: `if (combine == 8 && co.plan.uses_tex1 && !c->u1) { nd_nat_sub = 2u; mf_bx[nd_mdl_cur_x & 3u]++;
return (st->why = 6, 0); }`. Fix the census print (:247) to `"-abcd"[mf_e[bi].s < 5 ? mf_e[bi].s : 0]` so sub 4 prints d.
Keep the comment of :355 in step: "noise stays why 6 (named); pm5 with tex1 is native (FB-3 uvx)".

## Start
`sh 3ds/tools/wt_new.sh MIM11FIX`. PM_ROW=MIM11FIX PM_LANE=MIM11FIX.

## Touch only
3ds/src/nd_model.c (:355, :358 in nd_model_state_in; :247 in nd_model_log).

## Gate (CLAUDE.md 2-3; no console run in this row)
- Build, memcheck OK; nd_host PASS.
- `bench.py base spot_mac01_det` and `bench.py base mdl5_iwa10_det`; one list `bench.py run mim11_cen,spot_mac01_det,mdl5_iwa10_det
  --row MIM11FIX --why "MIM11FIX: pm5 tex1 model records native"`. Pass: mim11_cen f300 `ndm ... fb 0` (no why 6) and mdlsub d 0;
  `bench.py cmp` of spot_mac01_det and mdl5_iwa10_det against `$(git merge-base HEAD main)`: byte-equal (neither has a why 6 sub 4
  record: iwa_10 fb 0). A byte difference: stop and hand back the cmp lines. Budget: 5 Azahar starts.
- The user checks the mim_11 floor on the console with the batch push.

## Stop / At the end
A failed gate or an edit that does not fit: commit, hand back the output. Delete this row file in the completing commit.
