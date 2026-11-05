## MENULAG (2026-10-29): lag gates for the three menu scenarios

- Scope: pause_menu, partner_menu, file_select. The until-line is a `phase W` window line that starts after the menu is open (pause open at frame 96; file select at frame 15). The pause stays open: no close step. One top shot inside the menu.
- Gate fields (azt_scenarios.json): `max` = numeric limit on capture group 1 of the regex. `pick: last` = the last matching line, which is the window of the until-line.
- Mapping (no taskpl or flsplit line after LOGCLEAN, so the old baseline names do not exist in the log):
  - walk = `cpu (wall-idle-fbegin)` avg of the `phase W` line, in ms (not taskpl main us). The baseline 7634 us is taskpl main us and is not comparable.
  - closes = `flush` avg of the first `phase count` line (all closes). Tex-only closes are counted in pm_fl_n but no line prints them. The tex-only gate is not possible without a new log line.
  - ops = `gbiops` avg of the second `phase count` line (GBI commands per frame).
- Azahar, two runs each (the larger value is the base). Build 1433 (git b5c7b5226), binary build3ds\pm_3ds.3dsx, row MENULAG, 6 starts of 8.

| Scenario | Run | Wall (stop after) | Walk cpu ms | Closes | Ops | Limits set (base +15 %, rounded up) |
|---|---|---|---|---|---|---|
| pause_menu | 1 | 17 s | 11.1 | 135 | 1471 | |
| pause_menu | 2 | 15 s | 11.1 | 135 | 1471 | walk 12.8, closes 156, ops 1692 |
| partner_menu | 1 | 16 s | 9.5 | 107 | 1151 | |
| partner_menu | 2 | 15 s | 9.2 | 107 | 1151 | walk 11.0, closes 124, ops 1324 |
| file_select | 1 | 15 s | 8.1 | 88 | 1406 | |
| file_select | 2 | 15 s | 7.9 | 88 | 1406 | walk 9.4, closes 102, ops 1617 |

- Shots: one top shot per run (`build3ds\azt\shots\<scenario>`). A person must open each shot and confirm the subject (pause status page, Party tab, file select screen).
- Walk gate values are in ms in the log. Limits are in the same unit.


