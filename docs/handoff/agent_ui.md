# agent_ui STATE (UI1)
STATE: fixed in pm_gpu.c. Cause: the drop shadow and the glove shadow (PM_CC_07, msg_draw.c and msg.c) are combine class 4 (constant PRIM colour times texel alpha). pm_gpu_tex_rect mapped every class outside 5 to 8 to class 2 (texel times white), so the shadow drew as an opaque copy of the glyph texel colours at +2,+2. Fix: class 4 keeps its class in pm_gpu_tex_rect, and an open class 4 rect batch is closed when the constant colour changes (rect_cc).
Gate (sbk56, ui1_sbk56, steady f513 to f524): Yes box one colour copy plus grey (166,166,161) = bg*0.72 + 40*0.28 at +2,+2; glove one copy plus the same grey shadow.
Debug lines behind PMDL("ui1log") stay in msg.c and msg_draw.c. Walker debug (ui1walk, ui1cc) is only in the scratch tree.
