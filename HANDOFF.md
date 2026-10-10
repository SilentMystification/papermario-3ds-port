# Handoff

When a feature or slice is finished, stop and tell the user. They start a new agent from this file. Do not keep working in the same chat.

A commit is not automatically a handoff. Each commit, decide whether the slice is done enough for a new agent. If it is, say so.

Replace this file. Do not append a history. Keep it short.

Plan: [Cutscene systems plan](file:///C:/Users/Andrew/.cursor/plans/cutscene_systems_plan_2097aefa.plan.md). Scene setup and actors are done. Effects are in progress: partially wired, not functional. Audio is pending. Cook ROM assets only if a measured frame budget is missed.

## Now

Paper Mario O3DS port. Intro is `hos_05` entry 3. Last binary is **build 49** (not committed).

Bowser still does not render. Kammy does. Effects are colored rectangles where the effects should be. The real fire, lightning, and other effect graphics are not in yet.

Uncommitted: `src/sprite.c` (large clown-car pieces got their own flat quads; that did not make Bowser show), `3ds/src/pm_fx.c` (lightning zigzag, fire as a cone of quads). Last commits on `main`, not pushed: `1da0d7d3f` overlay getter and stand-in effect quads, `fa20b99d5` sprite heap and intro camera.

## Next

Get Bowser on screen in the clown car, with Kammy still herself. Replace the colored rectangles with the actual intro effects, starting with fire and lightning.

## Do not

Do not push. One Azahar window; the run script kills only the process it started. Do not leave `fast` in `debug3ds.txt` (it skips the logos). `3ds/run_azahar.ps1` stops on `-Until`, a crash, log silence (`-Stall`), or `-Timeout`. The storybook needs on the order of 150s with no early `-Until`. Log: `%APPDATA%\Azahar\sdmc\3ds\PaperMario\log.txt`. Window captures do not show the OpenGL screen. GPU work stays on the GPU. Paperboat is evidence of one little-endian behavior, not a template.
