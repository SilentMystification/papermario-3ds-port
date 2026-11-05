## WMODEL (2026-10-29): world model list and render task count in the fsplit line

The fsplit line (fsplit flag only) adds `wm <mean> max <max> tk <mean> max <max>` over the 300 drawn frames. wm = non-null entries of the decomp world model list (wModelList, 256 slots), counted once per frame. tk = gbi tasks walked per frame (gbi_task, key replays included). The first frame of the window is not counted.

| Scenario | What it proves | Command | Result | Status |
|---|---|---|---|---|
| yard_fsplit (WMODEL) | Yard, fsplit f600 line has the wm and tk fields. HEAD (lane pzfix) has no fields and two runs give a byte-equal top shot. | `yard_fsplit -Try -Row WMODEL -Build wmodel` | f600: wm 103.0 max 103, tk 2.00 max 2. Top shot 1 of 1 byte-equal to HEAD (HEAD run 1 and run 2 equal). logstat work at f600 window: HEAD 6.95 and 6.94 ms, new 6.96 ms (+0.01 ms, limit 0.2 ms). | unverified (3 runs) |
| mac01t_fsplit (WMODEL) | Tour to mac_01, fsplit f900 line has the wm and tk fields. | `mac01t_fsplit -Try -Row WMODEL -Build wmodel` | f900: wm 143.0 max 143, tk 2.00 max 2. Top shots 2 of 2 byte-equal to HEAD. | unverified (2 runs) |

