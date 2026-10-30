# agent_drec1 (row DREC1)

State: not implemented. No source edit, no build, no Azahar start. Start count 0 of 8.

Reason (code evidence, HEAD 037dc0317):
- tex_op SETTILE (pm_gbi.c 2931 to 2940): a tile 0 change with a different state calls pm_gpu_flush, which runs flush_draw (pm_gpu.c 557 to 567). That draws the queued batch of the previous model.
- The sp0c tex period (sp0_kind, pm_gbi.c 2169) wraps tb_run, so it includes that batch draw.
- A compiled bind (plan_mr2_record.md section 5) keeps the same flush when the tile changes. The batch draw is needed work for the earlier model, so the record cannot remove it.
- Dispatch cost of tb_run: about 9 tex_op calls per bind. Estimate 1 to 2 us per bind on console, about 50 to 100 us per frame at 47 binds. This is far below the 40 % tex gate.
- Gate 40 % sp0c tex (plan section 7) therefore needs a change to batching (when a batch closes), not to the bind. That changes the draw order and is outside DREC1.

Next step (orchestrator decides): an investigation agent splits sp0c tex into flush_draw time and tex_op time. Path 2 (DREC1) waits for that split.
