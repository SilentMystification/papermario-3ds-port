# diag_c1_source: yard texture op sources (Azahar, diag lane)

Run: worktree papermario-3ds-port-diag (temp counters, not committed). PM_ROW=diagc1, 2 starts. Window frames 300-599.

## Numbers (f599)
- Tex class: 1084 ops/frame (est 4368 us). The 3866 figure is not seen now.
- Texture DL emits: 65/frame, 40 distinct models in window. Inline make_texture_gfx: 0/frame.
- Texture DL length: about 9 tex ops per run (65 x 9 = 585, near 596 for the two model producers).

## Emit sites (src/model.c, main lines)
- 1530 and 1534: gSPDisplayList(textureHandle->gfx). Both run.
- 1523: inline make_texture_gfx. Not run in the yard.
- 2012: model display list. No tex ops in the top 6.

## Tex ops by producer
- appendGfx_model (0x160e58): 341
- appendGfx_model_group (0x164914): 255
- WorkerBackUI: 204
- bg: 174
- appendGfx_animator (0x215320): 49
- appendGfx_entity_model (0x212608): 44

## Lists (top 6)
- 0x01001e4c, TextureHeap+0x4558: 72 ops, 8 runs.
- 0x01000d54, TextureHeap+0x3460: 63 ops, 7 runs.
- 0x014883e8, 0x013e41b8, 0x014641a8, 0x015083d8 (class 7): 121, 121, 86, 86 ops, 0 runs (root lists). Emitter not identified.

## Per model (about 9 ops per emit)
- ID 0x58: 23 emits (about 207 ops). ID 0x35: 3. ID 0x07: 2. ID 0x73: 1.

## Fit with plan_c1_texbind
- Works for model texture DLs (sites 1530, 1534; about 585 ops). No C1i needed.
- Not covered: class 7 root lists (about 414 ops) and animator/entity (about 93). Need C1i or emit-site changes.
- C1-1 tbsplit 0 is unexplained: the sites run 65 times per frame. Check first: count op 0x0A in the gbi loop.

## Change for one op per model texture
- Emit: MODEL_TEX_OP at model.c 1530 and 1534. Record: tb_list[idx] in load_texture_impl (model.c 2068-2117).
- Handler: G_PM_TEX case in gbi_run_body runs the tex_op words (SETTIMG to SETOTHERMODE_H), then ensure_tile and sync_combine as before.

Open: model ID of class 2 lists (no map call in default branch); emitter of class 7 lists.
