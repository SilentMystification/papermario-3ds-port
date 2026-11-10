# Row HEARTBLK: lag while HP and FP count up at a Heart Block (user, 2026-11-09)

## User facts
"When low health and you use the HP restore block, the game gets laggy as the health and FP tick up."

## Leads (orchestrator)
- The status bar redraws its HP/FP digits every frame while they count up (text and icon rects). Console fact (XL, row HC7): about
  20 extra text rects cost about 5 ms and 15k I-misses per frame (about 250 us per rect); HC7 steps 3-6 merged, step 2 open.
- The Heart Block effect itself (HeartBlock entity script, fx_* recover/sparkle, sound per tick).
## Step 1 (queue, one agent at a time): oracle code read of the Heart Block sequence and the status bar count-up draw; rank the
per-frame costs; one console prof check (rects per frame, nd, tex) and the fix. Then one coder row.
