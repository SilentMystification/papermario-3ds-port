# Row HC4e2c: nd_shape.c reads typed units

(Model: Haiku. Oracle batch 2, 2026-11-10. Start: `sh 3ds/tools/wt_new.sh HC4e2c`; PM_ROW=PM_LANE=HC4e2c on every build.sh and bench call. CLAUDE.md rules 1-7. No git stash. Foreground waits. Long output to a file, then grep or tail it. Commit after each passed step. Delete this row file in the completing commit. A failed gate or an unexpected fact: stop, commit, hand back the printed values.)

## Touch only
3ds/src/nd_shape.c (:31 dlc_end -> NDO_END unit, :40-45 hash, :156 compile copy).
Same pattern as HC4e2a. Gate: byte-equal spot_mac01_det, mdl5_iwa10_det.
