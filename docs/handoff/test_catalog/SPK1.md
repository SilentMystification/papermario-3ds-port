## SPK1 (2026-10-29): cost of pm_mem_hud (linear free and mallinfo)

- Instrument only: two tick pairs in pm_mem_hud (3ds/src/pm_mem.c), fsplit only. Line `spk1 linearfree N us, mallinfo N us` once per HUD refresh (1 per second).
- Azahar, yard_fsplit (lane spk1, runs 1 and 2): linearSpaceFree 1 us, mallinfo 4 to 5 us per refresh. Total about 6 us Azahar, about 17 us console (x2.86). Gate 3 ms: not met, so the HUD code is not changed.
- HUD values equal to HEAD in the log: `hud mem rows ready: MEM used 32.2 / 63.9 MB free 31.7 MB | VRAM min 2130K pk 4014K` (HEAD and SPK1).
- fsplit f600 over50: 0 in HEAD and SPK1. fsplit f300 (load) over50: 18 in SPK1 run 2; HEAD f300 not kept (log overwritten). Open.
- Bottom shots do not reproduce across runs on this scenario: HEAD against HEAD 667 to 670 px differ per bottom shot; SPK1 against SPK1 about 5000 px. Top shot equal HEAD against SPK1 (1 of 1). Bottom shots are not a clean gate here.
- Starts used on SPK1: 4 of 8.


