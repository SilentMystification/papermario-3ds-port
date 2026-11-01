# agent_uc2_ctx: ctx+0xF4 probe (UC2, log only)

## State
- Probe added. Log only. No draw change. Switch: existing `stprobe`.
- 3ds/src/pm_gpu.c: `stc_set` now reads `__C3D_Context+0xF4` after each C3D_StencilTest and logs it beside the port word. New `pm_gpu_stc_ctx_log(const char*)` logs the same word at a named point.
- 3ds/src/pm_gbi.c: `uc2d_first` calls `pm_gpu_stc_ctx_log("uc2d_first")` after its UC2D line.
- Build: lane UC2 (`PM_LANE=UC2 sh 3ds/build.sh`), rc 0. map_restore_table.c not touched.

## Run (START 9, row UC2, budget 7 of 8 used)
- `azt.ps1 yard_stc -Row UC2 -Build UC2 -Try`. Log checks pass (2 of 2). Picture not checked by the agent.
- Log lines (frame 35):
  - site 1: word105 ff01ff11, ctxF4 ff01ff11
  - site 2: word105 ff01ff21, ctxF4 ff01ff21
  - site 3: word105 ff000010, ctxF4 ff000010
- Sites 4 to 6 and the UC2D window did not log. The scenario debug string has no `reccheck`, so `uc2d_first` did not run. The run stopped at the until-line `rec f34\d`.

## Findings
- CONFIRMED at sites 1 to 3: the memory word at ctx+0xF4 equals the value the port passes to C3D_StencilTest.
- No port word in sites 1 to 3 is 0x00000110.
- The port words of sites 4 to 6 (from the earlier decode): 0xFF01FF21, 0xFF01FF11, 0xFF000010. None is 0x00000110. Not yet read in memory.

## Open items
- The first UC2D window read is not yet tested. A run needs `reccheck` in the debug string. Budget left: 1 start.
- Sites 4 to 6 need a window after frame 35.
- The catalog row for yard_stc is missing from docs/handoff/test_catalog.md. The scenario entry is in azt_scenarios.json (uncommitted, from another agent). Not committed here.

## Next steps
1. Decide on the last start: yard_stc with `reccheck` added to the debug string, and an until-line that reaches the UC2D event (frame where j=75 differs).
2. Read the UC2D line with the ctx word. If ctx equals 0x00000110 at the flush, the writer is outside the port stencil calls.

## Commands
- `PM_LANE=UC2 sh 3ds/build.sh` (Git Bash, from the repo root)
- `PM_ROW=UC2 powershell -File 3ds/tools/azt.ps1 yard_stc -Row UC2 -Build UC2 -Try`
- Log: `C:\Users\Andrew\AppData\Roaming\Azahar\sdmc\3ds\PaperMario\log.txt` (grep `UC2P`)

## Acronyms
UC2 = reccheck epic. ctx = citro3d context (`__C3D_Context`). PICA = the 3DS GPU command set. stc = stencil call.
