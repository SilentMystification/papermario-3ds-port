# Decision: close the PICA command-list jump path (2026-11-01)

Reason. The jump test stopped the console after the map load (area 5 map 5, frame 327) in every variant tested:
- JT5 to JT10: register value, size field, clip write, skip in no-draw frames, restore, unused register, finalization write.
- JT11: no register write at all, finalization write only. Console stopped again after the map load. Azahar passed frame 480.

The jump is not proven on the console, and no public example uses it (research_pica_jump.md, research_pica_jump2.md, research_sm64_3ds_jump.md).

Decision. Use the copy path: each record's raw words are copied into the frame's command buffer with GPUCMD_AddRawCommands (a memcpy, confirmed in libctru gpu.o). No jump command is used. The jump test code stays behind the jumptest switch as a debug tool until removed by a later row.

Next. SR3 (record build) on the copy path, then SR4 (batch copy per frame) replaces the jump step in plan_stateful_records.md.
