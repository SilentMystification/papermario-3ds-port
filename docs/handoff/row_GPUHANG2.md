# Row GPUHANG2: remove the only NULL bind of texture unit 1 (dead pm_tex_unbind_aux)

(Model: Haiku. One function and one declaration are deleted; no other change.)

## Fact (GPUHANG1 coder flag, main 9b99b7d46)
pm_tex.c:2130-2131 `void pm_tex_unbind_aux(void) { pm_gpu_rect_sync(); au_last = NULL; pm_c3d_texbind(1, NULL); }`.
The comment at pm_tex.c:819 says C3D_TexBind(1, NULL) faults (C3D_TexBind reads the texture type for any unit above 0), and a
NULL on unit 1 is the state GPUHANG1 removes.

## Cause (oracle code read)
pm_tex_unbind_aux has no caller: `grep -rn pm_tex_unbind_aux 3ds src include` finds only its definition (pm_tex.c:2130) and its
declaration (3ds/include/pm_tex.h:45). The NULL bind never runs today. It is the only NULL bind of unit 1 or 2 left:
- the other binds are pm_tex.c:961, :2016, :2039, :2106, :2124 with a texture pointer, and :819 for unit 0;
- the unit_drop path uses the dummy for units 1-2 (pm_tex.c:22-23).
Deleting the dead function removes the trap for a future caller. No picture change.

## Start
`sh 3ds/tools/wt_new.sh GPUHANG2`; PM_ROW=GPUHANG2 PM_LANE=GPUHANG2.

## Exact edits
1. 3ds/src/pm_tex.c: delete pm_tex_unbind_aux (the 2 lines at :2130-2131).
2. 3ds/include/pm_tex.h: delete line 45 (`void pm_tex_unbind_aux(void);`).
3. Pre-flight after the edit: `grep -rn "pm_c3d_texbind([12], NULL)\|C3D_TexBind([12], NULL)" 3ds/src` returns nothing.

## Gate
- Build, memcheck OK. `python 3ds/tools/bench.py run nd_host --row GPUHANG2 --why "GPUHANG2 dead NULL bind removed"`: PASS.
- No Azahar run: no caller existed, so no picture can change (the build proves there is no caller).

## Stop
A build error (a caller exists): stop and hand back the error line. Delete this row file in the completing commit.
