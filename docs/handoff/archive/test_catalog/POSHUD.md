## POSHUD (2026-10-29): bottom screen position values

- Switch `poshud` (debug string, or options.ini `poshud=1`, Debug tab row 8 "Position HUD"). Bottom screen rows 0 and 1 show Mario x y z yaw and the camera x y z yaw. Refresh: hudint (the memory rows are replaced by these two rows while on).
- Mario yaw is gPlayerStatus.targetYaw. Camera values are gCameras[CAM_DEFAULT].lookAt_eye and curYaw.

| Scenario | Flags | Command | Result (build poshud, same code as the commit) | Status |
|---|---|---|---|---|
| yard_poshud | `poshud` | `yard_poshud -Try -Row POSHUD -Build poshud` | 3 of 3 log checks. Bottom shot bot_0259: `M x 594 y 30 z -216 yaw 180`, `C x 924 y 161 z 60 yaw 310`. Log at f300 gives Mario pos 594 30 -216. Top shot 0000_0_11 byte-equal to yard_headA (HEAD e16fee4b9). | unverified (one run, before the POSHUD2 row change; camera row removed since) |

