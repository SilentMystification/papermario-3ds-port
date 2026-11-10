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

## Oracle task
Design the smallest tool that meets the goal. Decide how to get memory reads and input control from gopher64: for example a fork
with a small control port (read RDRAM, set pad state, step N frames, save and load state), or reads from outside the process.
Decide how warp/pos map to N64 RAM writes of the same variables the port's warp sets (from the decomp source). Name the parts
(build of the tool, harness, symbol lookup, display list decoder reusing the port's N64 decoders in host C, scenario format),
give each its Haiku row with files and one gate, and say what needs the user (install of a Rust toolchain, if the design needs one).
