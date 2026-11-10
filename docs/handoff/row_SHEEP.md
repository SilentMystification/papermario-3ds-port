# Row SHEEP: lag during the Sleepy Sheep item (battle)
## Paths (code, src/battle/move/item/sleepy_sheep.c)
1. 10 virtual entities (sheep models, :43). Each is moved every frame (:60-66) and drawn as an entity model: one ent path draw per sheep, with a full combine sync each (the prof counter "full combine syncs in ent"). This is the HC8 subject (ent_run fast path, target class I <= 5 KB).
2. fx_landing_dust: type 2 (1 part) every 2nd frame (:81-90) and type 3 (4 parts) about every 5 frames (:78). Each instance runs its own setup list (landing_dust.c:282-284: segment, dlist2) and one matrix per part. Most of these are live at once over the about 87-frame run (x -200 to 320 at 6 per frame).
3. SOUND_SHEEP_STAMPEDE in a 7-loop (:299-300).
## Fix rows
Path 1 waits for HC8. Path 2 is the effect batching item, which has no row yet (see item 6). Next oracle step: read the console prof line of one XL push, debug "warp=kmr_03:0 story=ch0_toad_town battle battlef=100 invitem=0x08F padscript=@pad_btl_star.txt prof=30". Then path 2 gets its row.
