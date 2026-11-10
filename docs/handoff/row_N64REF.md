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

### Decision (2026-11-10): no fork
The fork build failed on its compiler setup (the cc crate passes gcc flags to cl.exe; the upstream Windows build needs clang-cl inside
vcvars64), and a fork needs patch upkeep. The stock gopher64 exe plus outside memory access is smaller. The clone in
C:/Users/Andrew/Documents/GitHub/gopher64-probe and the installed rustup 1.29.3 / Rust 1.99.0 / LLVM 23.1.5 stay unused.

### Rows
- **N64PEEK (Haiku).** 3ds/tools/n64ref.py, Python 3 with ctypes only (kernel32 OpenProcess, VirtualQueryEx, ReadProcessMemory,
  WriteProcessMemory, NtSuspendProcess/NtResumeProcess from ntdll). One command:
  `python 3ds/tools/n64ref.py <map>[:<entry>] [--frames N] [--state SLOT] [--out DIR]`.
  1. Start the stock exe with ver/us/baserom.z64 (and `--load-state SLOT` when given).
  2. Find RDRAM: scan committed private regions of 8 MB or more for the 64 bytes at ROM offset 0xE8F0 (set_game_mode code), each u32
     word stored native-endian (the byte order is a fact the row confirms). RDRAM base = match - (0x800334F0 & 0x7FFFFF).
  3. Warp: wait for the title (a RAM value from the decomp), then write the simplest gGameStatus fields that start a map change
     (areaID +0x086, mapID +0x08C, entryID +0x08E, plus the trigger the decomp uses). Map names to ids from the decomp area tables.
     If warp needs more than about 20 lines, drop it and use --state only.
  4. Wait N frames by polling a frame counter in RAM, suspend, copy the 8 MB as big-endian to DIR/rdram.bin plus a small json (frame,
     the display list start of the last finished frame from the display contexts), resume, kill the exe.
  Gate: one command on kmr_20 writes rdram.bin whose areaID/mapID equal kmr_20, and the 64 bytes at 0x800334F0 equal ROM 0xE8F0.
- **N64DL (Haiku).** 3ds/tools/n64dl.c, host C built in the pm3ds-host:1 image like nd_host_test.sh: decode rdram.bin from a start
  address, follow G_DL and the segment table, print each command with its address and the RDP state on each tri, texrect and fill.
  n64ref.py calls it after the dump. Gate: the kmr_20 frame reaches G_ENDDL with 0 unknown opcodes and at least 1 G_TEXRECT.

### User answers (2026-11-10)
- Install: yes. The N64REF-1 agent installs rustup (version from rust-toolchain.toml) and, if SDL3 builds from source, CMake and the MSVC Build Tools.
- Fork: superseded by the no-fork decision above.
- Scope (user): "We're using this to test functionality, so ensure we're not overengineering a testing harness just to verify things."
  So: a standalone oracle tool (n64ref.py), no bench kind, no runs.jsonl rows. Keep only what a draw question needs: warp, pos, a short
  pad table, a frame dump and n64dl. until= only as a frame number. No timing statistics, no scenario catalog beyond what a row needs.

## Parked (user, 2026-11-10: "lets not pursue this path for now")
n64ref.py (N64PEEK) is merged: it dumps RDRAM of the stock gopher64 (title or a save state). No save states made; N64DL stopped before a commit. Resume only when the user asks.
