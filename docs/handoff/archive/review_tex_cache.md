# Texture cache review (3ds/src/pm_tex.c)

Scope: HEAD 12c5e9887. HEAD moved from 251c64189 during the review. Line numbers are HEAD. Uncommitted working-tree slab edits are not reviewed.

## Blocker
- HEAD does not build. 12c5e9887 removed `static u8* slab` and `slab_ext.off`, but code still uses both: pm_tex.c:442, 445-449, 477-489.

## Ranked findings
1. Boot slab failure disables every texture. pm_tex.c:443-458, 1117-1125. If vramMemAlign(1536 KB) returns NULL, fl1_on() is false, every load returns 0, and pm_gpu.c:752 skips every textured batch. slab_init runs once (pm_tex.c:675 area), so there is no retry. Likelihood: high for "nothing renders".
2. Partial-overlap invalidation missed. pm_tex.c:790 and 799-800 test only the slot start address. Scenario: slot img=A, slen 8 KB, write [A+4096, A+4352). No match, no dirty mark, old texels stay. Wrong texture. Likelihood: medium.
3. RGBA8 lookup returns a stale composite. pm_tex.c:1323-1337 has no slot_valid and no dirty check. pm_gbi.c:2414-2421 builds the key from addresses and sizes, not bytes. Scenario: source bytes change in place, same key, old box is bound. Wrong texture. Likelihood: medium for dialog boxes.
4. pm_tex_bind_slot (pm_tex.c:606-617) binds a dirty slot without a hash check. The DLC path is safe: pm_dlc.inc.c:857 compares sgen first, and invalidation bumps sgen. Only a new caller without that check is unsafe.
5. Slab leak on linearAlloc failure. pm_tex.c:1158 sets s->slab=1, then 1245-1250 returns 0 without slab_free. The next miss sets s->slab=0 at 1116 and loses the block. Scenario: linear heap full. Each failed miss loses one reserve. Likelihood: low to medium, cumulative.
6. Failed slot allocation: no null dereference and no negative cache. Slot index is checked at 1102-1109. A failed load inserts nothing, so the next call retries. Draws skip only in pm_gpu.c:752 (combine >= 2 and texture used). Other paths draw with tex=0 (pm_gpu.c:697, 852). Not verified which path a missing texture takes.
7. 32-bit FNV hash at lookup (pm_tex.c:847-869). A false match needs equal hash after a byte change. Probability about 2^-32 per check. Low.
8. LRU stamp compare `(int)(a-b) < 0` (evict, 621-630) is wrap-safe while live stamps differ by less than 2^31. A slot unused for 2^31 stamps can look newer. Not a missing texture. Low.

## Checked, no defect
- (1) Eviction skips slots with epoch == epoch (623-626). Lookup hits and pm_tex_bind_slot call TOUCH. Stale slots keep VRAM until pm_tex_begin_frame drops them (739-740).
- (2) Every slot hit stamps: TC_HIT at 72, used at 254, 611, 1000, 1014, 1025, 1331. Direct stamps at 1361 and 1451. Off-screen path (932-953) has no slot.
- (5) Page filter add and sub balance: undirty 544-550, drop 548, invalidate 811-812 and 834-835, slot_valid 864-866. Every drop or invalidate bumps sgen (577, 820, 838, 869).
- (6) slen = src_len (763-766) covers stride rows, CI4 bit-exact (siz+2 shift), glyph atlas aw x ah (glyph_remap 226-230), masks (over-covers), and shaded dx4/dy4 (expand_dual clamps, 384-400). Palette changes miss at the phash compare (1004) before slot_valid.
- tex_inv1 counters do not change the cache.

## Fix candidates (not applied)
- Fix the HEAD build: declare the slab block and use .p and .len only.
- Invalidate on overlap: test [img, img+slen) against [start, end) in the page filter and in the loop.
- Route pm_tex_load_rgba8 (NULL path) and pm_tex_bind_slot through slot_valid.
