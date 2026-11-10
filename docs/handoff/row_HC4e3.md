# Row HC4e3: static and local lists typed at compile time

(Model: Sonnet (user, 2026-11-10). Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e3`; PM_ROW=PM_LANE=HC4e3 on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/include/nd_gbi.h, include/gbi_custom.h, 3ds/src/nd_rec.c (nd_gx_w).
## Work, one commit per group
- Group 1 scalar: SETOTHERMODE, GEOMETRYMODE, colours, PRIMDEPTH, SCISSOR, TEXTURE, MOVEWORD, SETCOMBINE, KEY/CONVERT.
- Group 2 texture: SETTIMG, SETTILE, SETTILESIZE, LOADBLOCK, LOADTILE, LOADTLUT.
- Group 3: VTX, TRI1/2, MTX, POPMTX, MOVEMEM, DL, ENDDL, syncs, NOOP, FILLRECT/TEXRECT, the G_PM_* macros.
For each gs* base macro of the group in include/PR/gbi.h (grep `^#\s*define gs`, F3DEX_GBI_2 branch): `#undef` it, then redefine it in nd_gbi.h as `{{ ND_X0(<its w0 expr>, <its w1 expr>), ND_X1(...) }}`, with the expressions copied from gbi.h. The g* bases pass ND_X0/ND_X1 into nd_gx_w.
Group 3 only: nd_gx_w detects the texture rectangle by NDO_RECT with NDO_RECT_TEX, the top-level end by NDO_END, and the branch by NDO_CALL with a&1.
Premise check: the ELF .data size is equal before and after (memcheck line). If it differs, hand back.
## Gate per group
nd_host. Byte-equal spot_mac01_det, battle_start_f182_cold, mdl5_iwa10_det. 2 tries per group.
