# Handoff DIRECT (epic: the game calls the native producers directly)

STATE
- Branch DIRECT (worktree build3ds/wt/DIRECT), main acca74540 merged. Plan: plan_direct.md. Row file row_DIRECT.md stays (D4 to D9 open).
- Done, byte-equal to build-at 151a86120 (spot_mac01_det, mdl5_iwa10_det, battle_start_f182_cold, nok11_det, direct_pra13_det): D0 frame stream, D1 model records and floor reflection ranges, D2 sprite records, D3 entity, item and animator records. nd_host PASS. Prof line `ndc`.
- Expected console change of D0 to D3: none (structure only). Console list for the user: spot_mac01_prof, spot_mac00_prof, spot_sbk56_prof, starfx_hos01_prof.
- Named fallbacks: records written inside build_custom_gfx (nd_rec_sub) stay in that list until D7; sprites the native draw refuses (battle context) walk their two payload words.
- Next: D4a (plan_direct.md section 5 finding): the texture tile, TMEM and TLUT model of nd_tex_set.c has eager GPU calls (pm_gpu_flush 6, pm_tex_set_wrap, pm_tex_load, sync_combine 7, ensure_tile); it must become a pure model plus lazy apply at the draw before any redirect slice (D4b) can move cost to core 1. Shared draw code: byte gates on all four det scenes.
- Tools: lane build with map_restore_table.c backup and restore (the row's b.sh pattern); base shots by bench.py build-at <base> <bench> (one build per bench).
