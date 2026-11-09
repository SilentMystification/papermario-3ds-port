# Row DOORSKY: first map entry builds a 313 KB black shroud sky texture (about 47 ms)

## User facts
User (2026-11-09): "entering and exiting a house still causes a noticeable hitch." DOORHITCH (merged 40489a5f1) cut model misses
per door from 476/474/231/419 to 136/13/13/3. The first entry of a map still has about 51 ms of texture time on console.

## Start state
Worktree build3ds/wt/DOORSKY, branch DOORSKY from main. PM_ROW=DOORSKY, PM_LANE=DOORSKY.

## Facts (DOORHITCH agent)
- The background palette fill at fogA 255 (src/background.c fill path) makes a 296x200 CI8 texture, 313 KB, about 47 ms of the
  first-entry texture time.
- Tried and reverted: drawing it as a GPU mix left fog alpha 255 set for later draws (Mario drawn black, about 20k px diff).
- Fix point: the fill path in background.c. Draw the fully shrouded sky as a solid colour (or GPU mix) and put the frame's fog and
  blend colour back after the sky.

## Gates
- xfer_door_kmr20 (Azahar): no 313 KB sky texture upload on the first entry (log), texture ms at the first entry lower.
- Byte-equal to a build-at of main: xfer_door_kmr20_shots 16/16, spot_mac01_det 6/6, battle_start_f182_cold, mdl5_iwa10_det 6/6.
  If the sky pixels change, name them and show they match the N64 colour.
- nd_host PASS. memcheck: no static growth (main margin 254 KB, OVER by 2 KB).
- Console (user): xfer_door_kmr20_prof, max < 50 ms.
