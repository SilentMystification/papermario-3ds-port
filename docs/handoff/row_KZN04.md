# Row KZN04: kzn_04 runs at 15-16 fps on the O3DS XL (user, 2026-11-09)

## User facts
"The room kzn_04 is very laggy." "Perhaps it's because of the fireballs." "Fire sticks." (the rotating fire bar
  hazards, Firebar / fire stick enemies or entities in kzn_04).

## Console facts (XL live log, main de4f4b8c8, build3ds/hw_live_log_131.txt lines 2975-3006)
Two windows: fps 15.7 / 16.6, ft 63.4 / 59.9 ms, nd 43.9 / 45.1 ms, lg 49.8 / 48.9 ms, I-miss 69k / 66k, D-miss 40k / 36k.
Class tex 23.4 / 23.7 ms (mac_01: under 2 ms); ph tex 6.8 ms; esync 54.7 / 13.8; ndc spr 19 / 24, ent 61.5 / 34.7, trect 53.5 / 56.3.
kzn_04 shape: 20 lists, 194 triangles (geometry is small).

## Step 1 (oracle, code read; the queue runs one agent at a time)
Find the kzn_04 fire sticks (rotating fire bars) and fireballs (script, effect or entity, count, texture: animated frames, palette cycling, scrolling, per-frame upload)
and every other per-frame texture user in the room (lava, lava fall). Name what costs 23 ms in the tex class, with file:line, and
the fix. Then one coder row.
