# Rows S7: list data and its callers through the S7CONV tables (index; ORACLE6, 2026-11-12)

All rows are written as their own files. Blocker for all: S7CONV merged (done, 07d0d9104). Each reads plan_nativeapi.md section 6.

| row | model | blocker | scope |
|---|---|---|---|
| row_S7TEX1.md | Haiku | none | tex_win0 and nd_tex_win_desc into nd_texwin.c (host-linkable) |
| row_S7RUN.md | Sonnet | none | nd_gtab_run (core 0), nd_rec_dl (core 1), the 11 empty GCONV_SRC lines (one per row, against merge conflicts) |
| row_S7EFX1.md .. row_S7EFX8.md | Haiku | S7RUN | effect groups (alphabetical, disjoint): GCONV_SRC line + nd_rec_dl call sites |
| row_S7TEX2.md | Sonnet | S7TEX1, S7RUN, S7ENT | NDG_TEX: texture load groups as build-time NdBindDesc binds |
| row_S7ENT.md | Sonnet | S7RUN | src/entity/model sources in GCONV_SRC (device link addresses), ent_compile takes the table's defs |
| row_S7MAP.md | Sonnet | S7RUN | nd_dlc_compile on the host into .nds v3; dlc_build loads it |
| row_S7MDL.md | Sonnet | S7RUN, S6KEY, S6VORD (merged) | rec_rm through the RM defs, keymiss proof (R13 binds moved to S9DEL2) |

| row_S7FLAME.md | Haiku | none | flame.c lists; the two flame buffers defined in src/effects/gfx/flame.c (TARGET_3DS), not src/main.c; build.sh stamp adds entity sources |
| row_S7ENT1.md | Haiku | S7ENT | nd_rec_dl at 14 src/entity call sites in 12 files |
| row_S7ENTX.md | Haiku | S7ENT | gen_gfx_syms VERSION_PAL filter; SaveBlock.c and Shadow.c into the set; SaveBlock call sites |
| row_S7ENT3.md | Haiku | S7ENT, S6E3 | HeartBlock and SuperBlock content call sites |

Follow-up rows not written yet: S7MAP1.. (ORACLE7 read: src/world holds `Gfx N(x)[]` lists, e.g. dgb_01/gfx.c calling the
data-only files dgb_01/img0-7.c, pra_31/texture_1-3.c, sam_05/monstar_gfx.c; files with functions (pra_31/stairs_skele.c,
iwa_01/splash.c, kmr_20/records.c, dro_02/npc_merlee.c, kzn_19/npc.c) reference outside symbols, so the tool link needs a
data-only split (the S7FLAME form: a definition moves to the file that uses it; no stub, user rule 2026-11-12): one Sonnet design row after S7ENT merges (its `.c` path rule)), S7MDL1..
(make_texture_gfx and the custom gfx builders of src/model.c, after S7MDL), S6E4.. (the state lines left in src/entity
setupGfx functions, e.g. Tweester.c, StarBoxLauncher.c, Plants1.c: the S6E2 form). S8 (row_S8FB.md) starts when all S7 rows
are merged.
Delete this file when the last S7 row merges.
