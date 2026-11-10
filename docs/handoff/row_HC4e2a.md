# Row HC4e2a: nd_ent.c reads typed units

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e2a`; PM_ROW=PM_LANE=HC4e2a on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/src/nd_ent.c.
## Exact edit
At every `>> 24` id test of a list unit (:112-136, :155-161, :202-221, :395):
- Read `uint32_t t[2]; const uint32_t* u = nd_op_typed(w0 >> 24) ? q : (nd_op_xlate(w0, w1, t), t);` and compare u[0] >> 24 with NDO_END/NDO_CALL/NDO_VTX/NDO_TRI1/NDO_TRI2.
- The branch flag: `(u[0] & 1)` replaces `((w0 >> 16) & 0xff) == G_DL_NOPUSH`.
- VTX and TRI fields are unchanged (same low 24 bits).
- :202: write `NDO_W0(NDO_END, 0)` and keep the G_ENDDL test (both until HC4e4).
## Gate
nd_host. Byte-equal spot_mac01_det, battle_start_f182_cold, nok11_det.
