# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- Branch DIRECT (worktree build3ds/wt/DIRECT), main acca74540 merged. Plan: plan_direct.md. Row file row_DIRECT.md stays (D4 to D9 open).
- Done, byte-equal to build-at 151a86120 (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det, direct_pra13_det): D0 frame stream, D1 model records and floor reflection ranges, D2 sprite records, D3 entity, item and animator records. nd_host PASS. Prof line `ndc`.
- Expected console change of D0 to D3: none (structure only). Console list for the user: spot_mac01_prof, spot_mac00_prof, spot_sbk56_prof, starfx_hos01_prof.
- Named fallbacks: records written inside build_custom_gfx (nd_rec_sub) stay in that list until D7; sprites the native draw refuses (battle context) walk their two payload words.
- Merged into main as 9b8461838 (D0-D3). Next: D4a per plan_direct.md section 5 (compound texture command; step D4a-1 splits set_tilesize into a pure window computation and its apply, byte gates; step D4a-2 the message glyph compound command). Not started in code.
- Tools: lane build with map_restore_table.c backup and restore (the row's b.sh pattern); base shots by bench.py build-at <base> <bench> (one build per bench).
