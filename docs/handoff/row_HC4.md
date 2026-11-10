# Row HC4: finish DIRECT (display lists fully removed) with typed ops, combine off core 0, model hit fast path - one Opus agent
Model (user, 2026-11-10): the oracle reads HC4e/HC4f first and recommends a model per row with its reason; the user chooses. Not decided: one Opus agent.

## User facts
User (2026-11-09): shrink the hot code by a very significant amount; HC4-HC6 need a more capable agent. Only the user starts this
Opus agent. HC0: M3 code run per frame on mac_01 112.5 KB (class A op loop 17.3 KB, C model hit 9.1 KB, D emit 5.7 KB, B combine
14.6 KB when it runs). Console XL de415917f mac_01: nd 14.4 ms, 37.4k I-miss, 20.2k D-miss per frame.

## DIRECT remainder (this row completes the DIRECT epic; agent_direct.md STATE, row_DIRECT.md)
- User (2026-11-09): DIRECT was to run to completion: display lists removed. Left after D11a: (a) the model record render mode list
  read as N64 words (rec_dl, full sync); (b) fallback geometry words in NDC_SPR/NDC_ENT payloads (native sprite draw for the battle
  context and imgfx; ent_draw for every list); (c) g2_same and the word-decoding op handlers in nd_walk.c (51 G_* cases).
- User (2026-11-09): no N64 display list reader in the ELF at all, cold code included (ELF size, performance). Static lists (map
  shapes, entity models) convert in build.sh (host C) to typed ops in romfs; no first-boot path. Lists the game code builds or changes
  at run time: each producer calls the native draw path directly.
- Done means: no N64 command word read anywhere in the ELF; nd_walk.c word cases, g2_same, g2_note, the ent/dlc list compilers
  deleted; row_DIRECT.md deleted.
- Code slices start after HC2 merges.

## Start state
`sh 3ds/tools/wt_new.sh HC4`, branch HC4. PM_ROW=HC4, PM_LANE=HC4. Read plan_hotcode.md (all), plan_direct.md section 5,
agent_direct.md STATE, hw_guidelines.md. The agent spawns no subagents.

## Work
1. Design first (into plan_hotcode.md, replacing the HC4-HC6 text): the typed NdOp format; the recorder-side shadow and its
   invalidation at every NDC_MODEL/NDC_SPR/NDC_ENT that can draw natively (sc_ok = 0, nd_sr_forget); which walker-state reads decide()
   and sc_sig_now need (premise check); the core 1 time budget (lgms per stage; logic must stay within its 30 fps step); the pre-built
   GPU command block per cached draw and the per-frame words it patches. Name the premise and a cheap check per stage.
2. Implement in slices, each committed after its gates: HC4a scalar state, HC4b combine, HC4c tile/load/TLUT, HC4d matrix; HC5 plan
   on core 1; HC6 rec_run hit fast path <= 600 B + cold rec_miss, pre-built command block. Delete g2_same, g2_note and the word cases
   as their slice replaces them.

## Gates per slice (light)
- nd_host PASS (extend nd_cmd_test.c for the typed op format).
- Byte-equal against stored base shots: spot_mac01_det, battle_start_f182_cold; plus the DIRECT set at the end of each stage.
- hotset_size M1 and fncov M3 per class before/after; targets: class A <= 4 KB, B <= 3 KB on core 0, C+D <= 8 KB.
- Console (user) per stage: prof push at mac_01 0 0 200 (pmu_i, pmu_d, cycles, lgms).
