# Row QTMPROBE: can a New 3DS CIA map and use the QTM RAM region (probe; PARKED: do not start)

(Model: Haiku. Planned work, user 2026-11-11: released as a possible avenue for more performance, with CIA packaging. It joins the
ASSETFREE epic (parked) together with the CIA build row; start only when the user starts that epic.)

## Question (user's plan)
Can a CIA title on a New 3DS map the 4 MiB QTM RAM (libctru os.h: OS_QTMRAM_VADDR 0x1E800000, OS_QTMRAM_PADDR 0x1F000000,
OS_QTMRAM_SIZE 4 MiB; the last 128 KiB kernel-reserved), read and write it, and sample a texture from it on the GPU? A .3dsx cannot (it
runs under the launcher's ExHeader); the QTM service (head tracking of the 3D camera) owns the region while it runs.

## Files (a standalone probe, not the game)
probe.c (the test program), probe.rsf (the CIA ExHeader: the static memory mapping under MemoryMapping), Makefile (target qtmprobe.cia,
devkitPro libctru and citro3d, makerom). Put them in 3ds/tools/qtmprobe/; nothing in the game build changes.

## Steps (user's plan)
1. Build qtmprobe.cia with devkitPro (make in 3ds/tools/qtmprobe).
2. Install it with FBI on a New 3DS.
3. Run it from the HOME Menu, not the Homebrew Launcher, with camera and head-tracking apps closed (QTM not holding the region).
4. Read the results on the top screen: map ok, write and read back at the start, middle and last word before the reserved 128 KiB,
   the QTM state.
5. Texture sampling test with citro3d: a 64x64 RGBA8 texture placed in the region, drawn on one quad, 4 pixels read back and shown.
6. Adopt: add `- 1e800000-1ebfffff` under MemoryMapping in the rsf (the game's CIA rsf, once the CIA build row exists), and a fixed
   carve-up of the region (no libctru allocator: hot native textures, command and vertex rings, frame matrices).

## Not covered
Latency: the probe shows whether the region works, not its read or write speed against FCRAM or VRAM for the CPU or the GPU; a speed
step follows only if the probe passes.

## Gate
The top screen results of steps 4 and 5, read by the user and handed back as text. No Azahar gate (Azahar has no QTM RAM).

## At the end
Delete this row file in the completing commit.
