# Row PROFILE: Cheats > Mario profile folder (every on/off and level saved in Mario's profile)

## User facts
User (2026-11-09): add to the cheat menu toggles for everything on Mario's profile that can be on or off, such as having a hammer or
setting the boots to Ultra Boots, plus whatever else fits from what is saved to Mario's profile; set partner upgrade level; include
action commands (the Lucky Charm gives hasActionCommands).

## Goal
One Cheats folder "Mario profile" that edits every PlayerData field (include/common_structs.h:310) that a player can own or level,
with no duplicate controls: existing Cheats rows for the same fields move into this folder and their old rows are deleted.

## Start state
Worktree build3ds/wt/PROFILE, branch PROFILE from main AFTER STORYMENU merges (STORYMENU edits the same Cheats tab code in
pm_options.c). PM_ROW=PROFILE, PM_LANE=PROFILE.

## Code facts
Cheats tab: pm_options.c tab_ids (about 386-398): Set Stats folder (RW_CSTATS, stat_lbl Max HP, Max FP, BP, Level, Coins, Star points),
Badges, Key items, Partners folder (RW_PARTF: Enable partners, per-partner level pm_dbgg_partner_level), Spawn item, Moon jump,
RW_CHEAT x6, RW_CH8 x10 (Boots level, Hammer level, Max star spirits, Star pieces, Story (deleted by STORYMENU), Inventory item, Fill
inventory, Save, Load, Instant win), Frame advance. Helpers in pm_dbg_game.c (pm_dbgg_*).

## Plan
1. Folder "Mario profile" (level 1 rows, < > changes a value, A toggles):
   - Equipment: Boots (None/Normal/Super/Ultra = bootsLevel -1..2), Hammer (None/Normal/Super/Ultra = hammerLevel -1..2),
     Action commands (hasActionCommands; On also gives the Lucky Charm key item if the game expects it, Off removes it).
   - Stats: Max HP, Max FP, BP, Level, Star points, Coins, Star pieces, Fortress keys (fortressKeyCount).
   - Star power: Star spirits (maxStarPower 0..7), Star power (starPower), Star beam (starBeamLevel: none, Star Beam, Peach Beam).
   - Partners: one row per partner: Owned (enabled) and Upgrade level (Normal/Super/Ultra, PartnerData level); Current partner.
   - Merlee: spell type, casts left.
   Each field change logs `profile: <field> <old> -> <new>`. Values are clamped to the game's valid ranges (read the code that
   uses each field; a value the game never sets is not offered).
2. Move, not copy: Set Stats folder rows, Boots level, Hammer level, Max star spirits, Star pieces, Partners folder level rows go into
   this folder; delete their old rows and code paths. Badges, Key items, Spawn item, inventory rows stay where they are.
3. The HUD and pause menu read these fields: after a change call the existing refresh the Set Stats rows use (check how they refresh).

## Done criteria
1. nd_host PASS. memcheck OK (const tables only).
2. One Azahar run (warp=random, menu by padscript): shots of the folder page and of the pause menu after Boots=Ultra, Hammer=None
   and Action commands=On: the pause menu shows Ultra Boots and no hammer. Log lines `profile:` present.
3. One battle check: with Action commands Off, a jump shows no action command prompt; On shows it (log line or shot).
Affected scenarios (no others): the new profile scenario.

## Rules
CLAUDE.md rules 5, 7, 9, 11. No git stash, no toggles (old rows deleted), no stubs. Commit after each passed step. Never commit
3ds/src/map_restore_table.c.

## At the end
Delete this row file in the commit that completes the row. SendMessage main "PROFILE ready <hash>".
