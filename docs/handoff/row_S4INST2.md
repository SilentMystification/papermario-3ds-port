# Row S4INST2: CLOSED by ORACLE6 (2026-11-12): superseded by S7RUN + S7EFX6. Never started.

- Same cause as row_S4INST.md: nd_parts_add (3ds/src/nd_rec.c:296-304) needs a table with NDG_F_PARTS, and ring_blast's
  source is not in GCONV_SRC (3ds/tools/gen_gfx_tab.sh:7) until S7EFX6 adds it; the old parts_ok rule this row read is gone
  (S7CONV). S7EFX6 also edits ring_blast_appendGfx's list calls (nd_rec_dl), the same lines this row would edit.
- Orchestrator: delete this file with row_S4INST.md in one docs commit.
