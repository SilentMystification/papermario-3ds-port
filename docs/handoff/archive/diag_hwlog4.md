# HWLOG4: O3DS live log, build 1467, ids #2138 to #5989

Source: build3ds/hw_live_log_170.txt. Tags: [M] measured, [E] estimate.
Area names: index in AREA_* list, include/enums.h lines 2997-3024 (MAC = 1, IWA = 8, DRO = 9, SBK = 10, ISK = 11, MIM = 12, GV = 26). Warp lines confirm IWA, DRO, SBK, ISK, MIM. Toad Town = MAC 2 (inferred, HWLOG3). [M]
Modes: game_modes.h (4 ENTER_WORLD, 5 WORLD, 8 BATTLE, 10 PAUSE, 11 UNPAUSE). Modes 16 and 17 are not in that enum. [M]

## 1. Log health
- Missing ids: #1792-#1884 (93), #2138-#3233 (1096), #3302-#3509 (208). Total 1397 of 5990. [M]
- Gap 2 hides map loads. Map labels for #3234-#3301 and #3510-#3558 are carried over. [E]
- Writer dropped 0. Live eagain 0 at #145, 14 at #1906, 31 at #3245. Stall closes 1, then 2. Sink max: file 377.6 ms (#3670), console 366.1 ms (#3542). [M]
- Fatal 0. Sleep 0. hudint 0. Line #3301 holds two pace texts. [M]
- Soft reset at #5325. Boot restarts at #5329. Exit to Homebrew confirm at #5988, exit not logged. [M]
- ROM READ IN DROPPED RANGE: 14 lines in the file. [M]

## 2. Segments (pace lines). Spikes are frames over 50 ms.
| Ids | Map | Mode | ms/frame (fps) | Spikes | dcsum closes / draw us / gc us |
|---|---|---|---|---|---|
| #3234-3301 | MAC2 | ENTER | 47.4 (21.1) | 13, about 4% [E] | 70 / 29 / 28 (f17538) |
| #3510-3536 | MAC2 (gap) | ENTER | 33.4 (29.9) | 0 | none |
| #3918-4055 | IWA1 | ENTER | 37.4 (26.7) | 40, about 8% [E] | 30 / 33 / 9 (f20838), 31 / 34 / 18 (f21138) |
| #4062-4215 | IWA1 | PAUSE (10) | 48.0 (20.8) | 80, about 18% [E] | 65 / 26 / 18 (f21438) |
| #4220-4366 | IWA1 | ENTER after unpause | 35.4 (28.3) | 0 | 31 / 33 / 23 (f22038), 22 / 36 / 9 (f22338) |
| #4435-4558 | DRO0 | ENTER | 37.5 (26.7) | 9, about 1% [E] | 41.5 / 32 / 13 (avg of 2) |
| #4608-4796 | SBK3, SBK10 | ENTER | 33.4 (29.9) | 0 in 37 s | 11 / 39 / 1 (f24438), 28 / 31 / 12 (f24738) |
| #4899-4969 | ISK15 | ENTER | 43.5 (23.0) | 30, about 22% [E] | 26 / 31 / 7 |
| #4970-5147 | ISK15 | BATTLE (8) | 35.1 (28.5) | 31 | 32 and 40 / 28 and 30 / 1 and 2 |
| #5162-5211 | GV0 | ENTER | 33.4 (29.9) | 0 | 40 / 27 / 3 |
| #5422-5487 | MIM4 | ENTER | 39.6 (25.3) | 25, about 8% [E] | 9 / 29 / 1 |
| #5488-5548 | MIM4 | BATTLE | 34.6 (28.9) | 11, about 4% [E] | 25 / 37 / 8 (f28038) |
| #5551-5679 | MIM4 | ENTER | 51.7 (19.4) | 100 of 217 frames, 46% [M] | 27 / 35 / 10 (f28338) |
| #5680-5813 | MIM4 | BATTLE | 34.1 (29.3) | 20, about 3% [E] | 34 / 33 / 10 |

Menu overlay windows (menu: open, options or debug UI, modes 4 and 5): 33.2 to 38.3 ms. The 8 windows of this kind in the segment list have 0 spikes. [M]
Mode 10 (PAUSE) occurs once. Frames: pause 20793 to 21249 (456). MIM4 ENTER 27826 to 28043 (217). [M]
Mode 4 spans 1000 or more frames in field play. The name ENTER_WORLD may not mean load only. [E]

## 3. Pause (IWA1) against gameplay (IWA1)
- Pause: 48.0 ms. Before pause: 37.4 ms (+10.6 ms). After unpause: 35.4 ms (+12.6 ms). [M]
- Work per frame (logstat): pause 41.6 ms (wall 45.0). Before: 31.8 to 34.9 ms (wall 36.4 to 37.4). After: 45.3 ms at #4225, then 25.7 to 33.6 ms. [M]
- Closes per frame: pause 65, 74 at f21738 (#4224, just after unpause), then 22 to 31. Before: 30 to 31. [M]
- Draw us per close: pause 26, before 33 to 34. Logged draw cost per frame: about 1.7 ms pause, about 1.0 ms before. [E]
- The logged draw path explains about 0.7 ms of the 10 to 13 ms gap. The rest is not logged. [E]
- Pause allocates a 150 KB z-buffer image (#4063) and two full images into bank A (#4067, #4069). Cost not measured. [M]
- 60 FPS option reads Off at #3270 and #5304. No toggle line between. [M]

## 4. Forever Forest (MIM, area 12)
- MIM4 ENTER #5551-5679: 51.7 ms, 19.4 fps, 46% of frames over 50 ms. Toad Town steady: 48.5 ms (HWLOG3). [M]
- Pace (1 s) and logstat (300 frames) disagree for #5551-5679: 51.7 ms and 38.9 ms. Not reconciled. The log does not show the cause of the spikes. [M]
- Closes per frame: 25 to 27 (Toad Town 70 to 77). Draw us per close: 35 (Toad Town 29 to 31). Logged draw cost: about 1.0 ms (Toad Town about 2.3 ms). [E]
- dlc2 tri 19539 at f26999 (one sample). Toad Town 277040 (HWLOG3). [M]
- First Forest load (#5220): VRAM bank A used 3031 KB, free 41 KB. Other loads: 2007 KB used, 1065 KB free. Session minimum 41 KB (acct), 82 KB (vram free min). [M]
- Tex cache at Forest loads: 1483 KB (#5222), 1589 KB (#5256). Other maps: 2.6 to 3.1 MB. [M]
- Texture misses 0 and texture failures 0 on all lines. No tex cache eviction counter. Row 4 frees at #5237 and #5238. [M]

## 5. Dry Dry Desert (SBK, area 10)
- SBK3 and SBK10 ENTER: 33.4 ms, 29.9 fps. Zero spikes in 37 s of SBK10 ENTER. [M]
- Texture failures 0, texmiss 0, b7 grabs 0 on all 714 b7 lines. No full-image or copy-buffer event from #4592 to #4934. [M]
- ROM READ IN DROPPED RANGE at #4603 (0040bba0, 2560 bytes). Link to the sun is not known. [E]
- The log cannot show the transparent texture break. It shows no frame or texture fault in the desert windows. [M]
