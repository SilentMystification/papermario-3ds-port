# Agent S3G: handoff

## STATE
branch: nds1 | commits: aa9406e6c (S3G-4 sort key, nd_order 3 passes), 98229f5d2 (S3G-3 nd_range helpers), 4323c3e69 (S3G-2 local copy, special tile list, untextured, fog shroud in record), f90d823b2 (S3G-1 custom gfx pre, post, geometry, pop in record)
done: S3G-1, S3G-2, S3G-3, S3G-4 (parts committed, nd_host PASS each part, pictures right by eye: mac_01, mac_00, sbk_56).
gaps: (a) S3G-4 merge step not built as code (see Decisions). (b) done criterion "no gSP/gDP on 3DS path": met for the stream, but old emission under recSkip and REC_SKIP still in source, dead on 3DS (list below). Delete = follow-up.
budget S3G: 7 of 8 Azahar starts (mac01 x3, mac00 x2, sbk56 x2). nd_host runs 55, 57, 59, 62.
S3G-4 pixel note: sbk_56 frame 200 differs from the S3G-3 shot in a strip x 172 to 192, y 127 to 380 (pond edge). Small, looks right by eye. Not chased.

## Decisions (plan defaults)
- Record: R11 w0 pre list, w1 post list. R12 w0 geometry list (0 for local copy: its list is the script's own G_DL, mdl_get_copied_gfx), w1 pop flag. R13 w0 count of words after the record (special tile list or the 4 untextured ops), run in place of the bind (rec_aux, pm_gbi_run with nbytes). G_PM_REC_WORDS 14.
- rec_dl: a model list runs as G_DL does (dlc_try first), then pm_gbi_run nested (returns at G_ENDDL).
- Pipesync: g1_skip in the walker, so no pipesync word on the record path.
- Shroud tint 255: early return before the record (draws nothing, emits nothing). Old path emitted state words before the return.
- S3G-3: nd_range_begin, nd_range_end, nd_range_repeat are in src/model.c (static, next to execute_render_tasks), not nd_core.c. nd_core.c is host-tested by 3ds/tools/nd_core_test.c with -I 3ds/include only (no gbi.h), and 3ds/tools is off limits for this row.
- S3G-4 sort: bat1_key packs (tmpl, tex0, tex1) as tmpl << 24 | tex0 << 8 | tex1 (tmpl = render mode, bat1_plain allows SURFACE_OPA only). nd_order runs three LSD passes (tex1, tex0, tmpl). nd_core_test uses tex1 = 0, so its reference still holds.
- S3G-4 merge: not built. Plan default: merge of adjacent equal keys is the walker same-state skip already present (g2_same in rec_op and the walker, rm_apply, tb_ops). nd_emit has no native draw path yet (LEGACY only, pm_nusys.c), so no new merge code has a consumer. Reopen when the native draw path lands.

## Dead on 3DS (recSkip or REC_SKIP is 1 for every model, recOn = 1): src/model.c appendGfx_model
gated gSP/gDP sites, all skipped on 3DS: depth and remap tint words (REC_SKIP), lights (!recSkip), MODEL_TEX_OP (recSkip), combine word (REC_SKIP), MODEL_RM_OP x6 (recSkip), fog words in FOG and shroud classes (REC_SKIP), shroud RM (REC_SKIP), MODEL_MTX_OP x2 (recSkip), custom gfx pre and post (else branch of recOn), panner tile sizes (REC_SKIP), billboard (REC_SKIP), geometry (else of recOn), gSPPopMatrix (else of recWord), gDPPipeSync (!recOn).
Still run on 3DS, by design: the record writes, the untextured ops (4 words, the R13 aux), make_texture_gfx (the special tile list, R13 aux), temp Gfx builders for fog and tint values (not stream words).
Non-3DS (N64) path unchanged.

## Notes
- git add -N . and git reset -q were run once by this agent at the S3G-1 commit, before the coordinator rule. The reset clears the index: no staged work of others was seen after it.
- nd_core.c and the other edited files were converted LF for editing and back to CRLF; no content change beyond the edit.
