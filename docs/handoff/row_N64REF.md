# Row N64REF: N64 reference tool (gopher64), memory reads instead of screenshots

(Design row for the oracle, batch 5. Then Haiku rows. User, 2026-11-10.)

## User facts
- Use gopher64, not Mupen64Plus: C:\Users\Andrew\Documents\Emulation\Emulators\gopher64\gopher64-windows-x86_64.exe (v1.1.42).
- Use the same ROM as the 3dsx: ver/us/baserom.z64 (41943040 bytes).
- Control the game in a similar way to the 3DS tools now (scenario strings: warp=, pos=, padscript=, until-lines, frame windows)
  if it is not too much to create. "Given the decomp it should be somewhat straight forward."
- Goal: read N64 RAM at a set frame (the display list of one object, the RDP state of a draw, game state structs) by the decomp
  symbols, with no screenshot comparison. Used by the oracle for draw bugs (COINSTAR-type: what the N64 sends for this draw) and
  for original timing (frame counts of effects).

## Facts (orchestrator, 2026-11-10)
- gopher64 v1.1.42 CLI options: fullscreen, widescreen, overclock, expansion pak, input profiles and controller binding, --port,
  --load-state <slot 0-9>, RetroAchievements login. No debugger, no scripting, no memory or input port in the release build.
- gopher64 is open source (Rust, github.com/gopher64/gopher64). No Rust toolchain (cargo, rustc) on this machine now.
- Symbols: ver/us/symbol_addrs.txt holds N64 RAM addresses (e.g. gGameStatus = 0x80074024, gGameStatusPtr = 0x8007419C,
  gCurrentDisplayContextIndex = 0x800741F4). The repo builds no N64 map file (3DS build only).
- The 3DS tools to mirror: 3ds/tools/azt_scenarios.json, bench.py, run_azahar.ps1, the debug strings (warp, pos, padscript, shots).

## Oracle design (batch 5, 2026-11-10)

1. gopher64 v1.1.42 is a pure per-instruction interpreter: `cpu::run`, with `cpu.pc: u64` and `cpu.gpr: [u64; 32]`. That makes a PC hook cheap. The source has no debugger or scripting. It does have cheats.rs, savestates.rs and netplay.rs.
2. The fork is a source clone in build3ds/n64ref/gopher64, outside git. The repo keeps only 3ds/tools/n64ref/gopher64_probe.patch (GPL-3, same as upstream). The patch adds src/probe.rs, enabled by `--probe <file>`. A stock run is unchanged.
3. Hooks are PC checks in `cpu::run` before decode, with addresses from ver/us/symbol_addrs.txt:
   - `gfx_draw_frame` 0x80026B08 counts frames. This frame number matches the port's pm_frame_count.
   - `nuGfxTaskStart` 0x8002CCC4 captures the DL: gpr[4] (a0) is the task's display list. It is the dump point, where the DL is complete.
   - `set_game_mode` 0x800334F0 drives warp and pos.
4. Warp copies pm_warp.c:798-845 through a call queue in the fork:
   - At the first `set_game_mode` entry with a0 == GAME_MODE_LOGOS, save the registers.
   - Set pc = clear_player_data (0x800E73A0), ra = 0x800334F0. When execution returns to the hook, set pc = clear_saved_variables (0x80145320). On the next return, write the RAM values below and resume with a0 = GAME_MODE_ENTER_WORLD.
   - RAM values: gGameStatus (0x80074024) +0x086 areaID, +0x08C mapID, +0x08E entryID. Write the story byte to gCurrentSaveFile (0x800DACC0) globalBytes[0], and the partner fields in gPlayerData (0x8010F290).
5. Pos copies pm_warp.c:1433 and :593. At the first `set_game_mode(GAME_MODE_WORLD)` entry, record the frame. 30 frames later write gPlayerStatus.pos (f32 x/y/z, 0x8010EFC8 + offset from common_structs.h) and gCameras[0].needsReinit = 1.
6. Padscript: n64ref.py expands the port grammar (pm_input.c:39-43) into a per-frame table `frame btn stickx sticky`. controller.rs `process` takes that row in place of `ui::input::get` while the table runs. The same site already switches for netplay.
7. Until lines become RAM conditions: `until=<sym>[+off]:<s8|s16|u16|s32|f32><op><val>`, checked at each frame hook, with a cap in frames and seconds. On a hit the fork prints `until f<N>`, and that number is the original-timing measurement. Frame windows: `dump=F` or `dump=F0:N`.
8. Each dump is the full RDRAM, written big-endian (each native u32 word swapped back), plus a json with frame, the task a0, and the segment table if read.
9. n64ref.py: symbol lookup from symbol_addrs.txt; map name to area/map ids from the decomp source (src/world area tables); scenario file 3ds/tools/n64_scenarios.json, using the port's warp=/story=/pos=/padscript= syntax, writes probe.txt, runs gopher64 with the ROM, kills it at the cap.
10. n64dl (host C, `-DF3DEX_GBI_2` with include/PR/gbi.h) reads a dump plus a start address, follows G_DL/G_ENDDL and G_MOVEWORD segments, and prints each command with its address. It tracks the RDP state (othermode H/L, combine, prim/env, timg, settile, tilesize, tlut) and prints it on each tri, texrect and fill. `--from <addr>` decodes one object's DL. It is separate from the 3DS walker, which emits GPU state, not text.

### Facts the first Haiku row must confirm in the source (tag v1.1.42)
- rdram.rs stores `mem: Vec<u8>` in native-endian u32 words (`from_ne_bytes`), so N64 byte A is mem[(A & 0xFFFFFF) ^ 3] on x86.
- `cpu::run` decodes one instruction at a time, including the icache_fetch path (no block cache).
- KSEG0 0x80xxxxxx maps to physical memory by `& 0x1FFFFFFF`.
- Where the frame or event loop can call std::process::exit cleanly.
- That it runs with no controller bound.
- The rust-toolchain.toml version, and whether SDL3 builds from source (CMake plus MSVC).

### Rows, cut to the minimum (user, 2026-11-10: "I dont want to invest a huge amount of time and resources into the n64 hook.
We need something that is repeatable and quick and easy for the agent to use.")
One command for the agent: `python 3ds/tools/n64ref.py <map>[:<entry>] <frames> [--from <sym|addr>]` -> prints the decoded display list
of that frame (or of one object) and the RDP state per draw. Nothing else: no padscript, no until, no pos, no scenario file, no bench.
- **N64REF-1 (running).** Toolchain, clone, build, facts.
- **N64REF-2 (probe, at most about 80 lines of Rust).** `--probe <file>`: lines `w <addr> <u8|u16|u32> <val> @<frame>` (RAM write at a frame,
  frame counted at the gfx_draw_frame PC) and `dump <frame>` (RDRAM to a file, big-endian, plus the nuGfxTaskStart a0), then exit.
  Warp is only RAM writes from n64ref.py (the simplest gGameStatus fields that start a map change; the agent confirms them from the decomp).
  If warp needs a call queue or more than about 20 lines, drop warp: the user makes gopher64 save states and n64ref.py uses --load-state.
  Gate: one command prints a kmr map's frame list.
- **N64REF-3 (n64dl, host C).** Decode a dump from an address: commands with addresses, RDP state on each tri, texrect and fill.
  Gate: 0 unknown opcodes on that frame, G_ENDDL reached.

### User answers (2026-11-10)
- Install: yes. The N64REF-1 agent installs rustup (version from rust-toolchain.toml) and, if SDL3 builds from source, CMake and the MSVC Build Tools.
- Fork: a separate repo, C:/Users/Andrew/Documents/GitHub/gopher64-probe (a local git clone of upstream tag v1.1.42 on branch probe).
  The fork changes live there, not in this repo; this repo keeps only 3ds/tools/n64ref.py, n64_scenarios.json and n64dl.c.
- Scope (user): "We're using this to test functionality, so ensure we're not overengineering a testing harness just to verify things."
  So: a standalone oracle tool (n64ref.py), no bench kind, no runs.jsonl rows. Keep only what a draw question needs: warp, pos, a short
  pad table, a frame dump and n64dl. until= only as a frame number. No timing statistics, no scenario catalog beyond what a row needs.
