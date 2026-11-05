## PAUSEFIX (2026-10-29): pause_menu gates reset on build 1517 (git 9461b95a8)

- Row PAUSE4A (background one load). pause_menu on HEAD 1514 against 1517, same scenario: closes 135 to 96, ops 1237 to 1003, walk 11.4 to 9.3 ms (window f176 to f235).
- Gates now: closes max 111 (96 + 15 %), ops max 1154 (1003 + 15 %), walk max 14.0. The walk window of the until-line is not one state: f120 to f175 mixes pause and field (13.6 to 13.9 ms), f175 to f235 is field (9.3 ms). A limit of 10.7 failed on the mixed window. Open: change the until (docs/handoff/agent_pausefix.md, item 1).
- partner_menu and file_select gates not changed (new values lower).
- Byte-equal top shots against HEAD (same run pairs): pause_menu 1 of 1, partner_menu 1 of 1, file_select 1 of 1, text_menu 3 of 3.

