# Row PROFILE: Cheats tab reorganized, with every on/off and level saved in Mario's profile

## User facts
User (2026-11-09): add to the cheat menu toggles for everything on Mario's profile that can be on or off, such as having a hammer or
setting the boots to Ultra Boots, plus whatever else fits from what is saved to Mario's profile; set partner upgrade level; include
action commands (the Lucky Charm gives hasActionCommands).

## Goal
Every PlayerData field (include/common_structs.h:310) that a player can own or level is editable from the Cheats tab, with no duplicate
controls, and the Cheats tab is reorganized. User order (2026-11-09) for the first rows: Moon jump, Spawn item, Equipment, Stats + star
points, Partners. The rest grouped by the orchestrator (below).

## Start state
Worktree build3ds/wt/PROFILE, branch PROFILE from main AFTER STORYMENU merges (STORYMENU edits the same Cheats tab code in
pm_options.c). PM_ROW=PROFILE, PM_LANE=PROFILE.

## Code facts
Cheats tab: pm_options.c tab_ids (about 386-398): Set Stats folder (RW_CSTATS, stat_lbl Max HP, Max FP, BP, Level, Coins, Star points),
Badges, Key items, Partners folder (RW_PARTF: Enable partners, per-partner level pm_dbgg_partner_level), Spawn item, Moon jump,
RW_CHEAT x6, RW_CH8 x10 (Boots level, Hammer level, Max star spirits, Star pieces, Story (deleted by STORYMENU), Inventory item, Fill
inventory, Save, Load, Instant win), Frame advance. Helpers in pm_dbg_game.c (pm_dbgg_*).

## Plan
1. Cheats tab, top level, in this order (folders end with " >"):
   1 Moon jump (On/Off row)  2 Spawn item >  3 Equipment >  4 Stats >  5 Partners >
   6 Star power >  7 Badges >  8 Key items >  9 Inventory >  10 Battle >  11 Merlee >  12 Position >  13 Frame advance (row)
   - Equipment: Boots (None/Normal/Super/Ultra = bootsLevel -1..2), Hammer (None/Normal/Super/Ultra = hammerLevel -1..2),
     Action commands (hasActionCommands; On also gives the Lucky Charm key item if the game expects it, Off removes it).
   - Stats: Max HP, Max FP, BP, Level, Star points, Coins, Star pieces, Fortress keys (fortressKeyCount).
   - Partners: Enable partners, Current partner, then one row per partner: Owned and Upgrade level (Normal/Super/Ultra).
   - Star power: Star spirits (maxStarPower 0..7), Star power (starPower), Star beam (none / Star Beam / Peach Beam).
   - Inventory: Inventory item, Fill inventory.
   - Battle: Infinite HP, Infinite FP, Infinite star power, No battles, Perfect action commands, Instant win.
   - Merlee: spell type, casts left.
   - Position: Save position, Load position. Speed x2 goes next to Moon jump only if it is a movement cheat in the code
     (RW_CHEAT row); else in Battle. Check RW_CHEAT 0..5 labels and place each by its meaning.
   Each field change logs `profile: <field> <old> -> <new>`. Values clamp to the ranges the game uses (read the code that uses each
   field; a value the game never sets is not offered).
2. Move, not copy: the Set Stats folder, Boots level, Hammer level, Max star spirits, Star pieces, the Partners folder level rows, the
   CH7/CH8/CH9 rows go into the folders above; delete their old rows and code paths. No row exists twice.
3. The HUD and pause menu read these fields: after a change call the existing refresh the Set Stats rows use.

## Done criteria
1. nd_host PASS. memcheck OK (const tables only).
2. One Azahar run (warp=random, menu by padscript): shots of the Cheats tab top level (the order above), the Equipment page and of the pause menu after Boots=Ultra, Hammer=None
   and Action commands=On: the pause menu shows Ultra Boots and no hammer. Log lines `profile:` present.
3. One battle check: with Action commands Off, a jump shows no action command prompt; On shows it (log line or shot).
Affected scenarios (no others): the new profile scenario.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles (old rows deleted), no stubs. Commit after each passed step. Never commit
3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "PROFILE ready <hash>".
