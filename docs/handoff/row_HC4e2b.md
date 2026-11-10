# Row HC4e2b: game-side readers of list words

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e2b`; PM_ROW=PM_LANE=HC4e2b on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
src/entity_model.c (:257-310), src/animator.c (:908), src/imgfx.c (:1252-1320), src/model.c (:4487 mdl_local_gfx_update_vtx_pointers), src/effects/hieroglyphs.c (:151), src/effects/underwater.c (:172-256), src/effects/energy_orb_wave.c, src/world/dead/area_flo/flo_10/waterfx.c (:308).
## Exact edit
Same pattern as HC4e2a, under `#ifdef TARGET_3DS`. Map each G_* op to its NDO_* id (the nd_op_n64 table in nd_op.c). Field reads are unchanged, except the G_DL branch flag. Code that writes a word into a list writes ND_X0/ND_X1 of that word.
## Gate
nd_host. Byte-equal spot_mac01_det, battle_start_f182_cold, fx_direct_det.
