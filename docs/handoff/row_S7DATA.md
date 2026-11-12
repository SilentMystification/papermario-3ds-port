# Rows S7: list data and its callers through the S7CONV tables (index; ORACLE6, 2026-11-12)

All rows are written as their own files. Blocker for all: S7CONV merged (done, 07d0d9104). Each reads plan_nativeapi.md section 6.

| row | model | blocker | scope |
|---|---|---|---|
| row_S7TEX1.md | Haiku | none | tex_win0 and nd_tex_win_desc into nd_texwin.c (host-linkable) |
| row_S7RUN.md | Sonnet | none | nd_gtab_run (core 0), nd_rec_dl (core 1), the 11 GCONV_SRC placeholder lines |
| row_S7EFX1.md .. row_S7EFX8.md | Haiku | S7RUN | effect groups (alphabetical, disjoint): GCONV_SRC line + nd_rec_dl call sites |
| row_S7TEX2.md | Sonnet | S7TEX1, S7RUN | NDG_TEX: texture load groups as build-time NdBindDesc binds |
| row_S7ENT.md | Sonnet | S7RUN | entity model blobs linked at 0x0A on the host, NDG_F_SEG tables, ent_compile lookup |
| row_S7MAP.md | Sonnet | S7RUN, S7ENT | nd_dlc_compile on the host into .nds v3; dlc_build loads it |
| row_S7MDL.md | Opus | S7RUN, S6KEY, S6VORD, S7TEX2 | rec_rm through the RM defs, R13 texture setup as binds, keymiss proof |

Follow-up Haiku rows (written by the oracle after the core merges, one file group each): S7ENT1.. (src/entity setupGfx call
sites, the S6E2 form plus nd_rec_dl), S7MAP1.. (dro_02 npc_merlee, hos_05 intro strips, end Credits: row_S6M.md list), S7MDL1..
(make_texture_gfx and the custom gfx builders of src/model.c). S8 (row_S8FB.md) starts when all S7 rows are merged.
Delete this file when the last S7 row merges.
