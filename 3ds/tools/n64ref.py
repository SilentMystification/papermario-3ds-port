#!/usr/bin/env python3
"""N64 RAM reference: run the stock gopher64 exe, dump RDRAM at a frame. No screenshots.

usage: python 3ds/tools/n64ref.py <map>[:<entry>] [--frames N] [--state SLOT] [--out DIR] [--cap S]

Warp is not done (gopher64 has no debugger or scripting). The game must already be in the
map: --state SLOT loads a gopher64 save state. The map id check reads gGameStatus.
RDRAM is found by scanning the emulator's private memory for the set_game_mode code (ROM bytes).
Each native u32 word holds one big-endian N64 word, so each group of 4 bytes is reversed.
Windows only (ctypes: kernel32, ntdll). No third-party packages.
Writes DIR/rdram.bin (8 MB, big-endian, N64 address = offset) and DIR/info.json.
"""
import argparse, ctypes, json, os, re, subprocess, sys, time
from ctypes import wintypes

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MAIN = "C:/Users/Andrew/Documents/GitHub/papermario-3ds-port"
EXE = "C:/Users/Andrew/Documents/Emulation/Emulators/gopher64/gopher64-windows-x86_64.exe"
ROM = os.environ.get("N64_ROM", os.path.join(MAIN, "ver", "us", "baserom.z64"))
RDRAM_SIZE = 0x800000
MEM_COMMIT, MEM_PRIVATE = 0x1000, 0x20000
PROC_ACCESS = 0x0010 | 0x0400 | 0x0800  # VM_READ | QUERY_INFORMATION | SUSPEND_RESUME

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
ntdll = ctypes.WinDLL("ntdll")

class MBI(ctypes.Structure):
    _fields_ = [("BaseAddress", ctypes.c_size_t), ("AllocationBase", ctypes.c_size_t),
                ("AllocationProtect", ctypes.c_ulong), ("PartitionId", ctypes.c_ushort),
                ("RegionSize", ctypes.c_size_t), ("State", ctypes.c_ulong),
                ("Protect", ctypes.c_ulong), ("Type", ctypes.c_ulong)]

k32.OpenProcess.restype = wintypes.HANDLE
k32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
k32.VirtualQueryEx.argtypes = [wintypes.HANDLE, ctypes.c_size_t, ctypes.POINTER(MBI), ctypes.c_size_t]
k32.VirtualQueryEx.restype = ctypes.c_size_t
k32.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]
k32.ReadProcessMemory.restype = wintypes.BOOL
k32.CloseHandle.argtypes = [wintypes.HANDLE]
ntdll.NtSuspendProcess.argtypes = [wintypes.HANDLE]
ntdll.NtResumeProcess.argtypes = [wintypes.HANDLE]

def read(h, addr, n):
    buf = ctypes.create_string_buffer(n)
    got = ctypes.c_size_t(0)
    if not k32.ReadProcessMemory(h, addr, buf, n, ctypes.byref(got)) or got.value != n:
        return None
    return buf.raw

def symbols():
    """name -> (RAM address, ROM offset or None), from ver/us/symbol_addrs.txt."""
    syms = {}
    for line in open(os.path.join(ROOT, "ver", "us", "symbol_addrs.txt")):
        m = re.match(r"(\w+) = (0x[0-9A-Fa-f]+);(.*)", line)
        if m:
            rom = re.search(r"rom:(0x[0-9A-Fa-f]+)", m.group(3))
            syms[m.group(1)] = (int(m.group(2), 16), int(rom.group(1), 16) if rom else None)
    return syms

def map_ids(name):
    """Map name -> (areaID, mapID): index in gAreas (world.c) and index in <area>_maps[]."""
    src = open(os.path.join(ROOT, "src", "world", "world.c"), encoding="utf-8").read()
    areas = re.findall(r"AREA\((\w+),", re.search(r"AreaConfig gAreas\[\] = \{(.*?)\n\};", src, re.S).group(1))
    for ai, area in enumerate(areas):
        m = re.search(r"MapConfig %s_maps\[\] = \{(.*?)\n\};" % area, src, re.S)
        maps = re.findall(r"MAP(?:_WITH_INIT)?\((\w+)\)", m.group(1)) if m else []
        if name in maps:
            return ai, maps.index(name)
    sys.exit("n64ref: map not found in gAreas: " + name)

def find_rdram(h, pat, phys):
    """Bases of 8 MB RDRAM images whose set_game_mode code (pat) is at phys offset."""
    bases, hits = [], []
    addr, mbi = 0, MBI()
    while k32.VirtualQueryEx(h, addr, ctypes.byref(mbi), ctypes.sizeof(mbi)):
        if mbi.State == MEM_COMMIT and mbi.Type == MEM_PRIVATE and mbi.RegionSize >= RDRAM_SIZE:
            start, size, step = mbi.BaseAddress, mbi.RegionSize, 1 << 24
            for pos in range(0, size, step):
                chunk = read(h, start + pos, min(step + 64, size - pos))
                if chunk is None:
                    break
                i = chunk.find(pat)
                while i >= 0:
                    hits.append((start + pos + i, start, size))
                    i = chunk.find(pat, i + 1)
        if mbi.RegionSize == 0:
            break
        addr = mbi.BaseAddress + mbi.RegionSize
    for m, start, size in set(hits):
        base = m - phys
        if start <= base and base + RDRAM_SIZE <= start + size:
            bases.append(base)
    return sorted(set(bases))

def be(mem):
    """Native-word RDRAM -> big-endian N64 order (reverse each group of 4)."""
    out = bytearray(len(mem))
    out[0::4], out[1::4], out[2::4], out[3::4] = mem[3::4], mem[2::4], mem[1::4], mem[0::4]
    return bytes(out)

def wait_frames(h, base, addr, n, cap):
    """Count changes of a 0/1 display context index (one flip per game frame)."""
    def val():
        raw = read(h, base + addr, 4)
        return None if raw is None else int.from_bytes(raw, "little")  # native word = BE value
    last, count, t0 = val(), 0, time.time()
    while count < n:
        time.sleep(0.001)
        v = val()
        if v is None:
            sys.exit("n64ref: RDRAM read failed while waiting for frames")
        if v != last:
            count, last = count + 1, v
        if time.time() - t0 > cap:
            sys.exit("n64ref: cap %s s hit at frame %d of %d" % (cap, count, n))
    return count

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("map", help="<map>[:<entry>], e.g. kmr_20")
    ap.add_argument("--frames", type=int, default=60)
    ap.add_argument("--state", type=int, help="gopher64 save slot 0-9 to load")
    ap.add_argument("--out", help="output dir (default build3ds/out/n64ref/<map>)")
    ap.add_argument("--cap", type=float, default=180, help="seconds for the frame wait")
    a = ap.parse_args()
    name, _, entry = a.map.partition(":")
    area, mid = map_ids(name)
    syms = symbols()
    gs, _ = syms["gGameStatus"]
    sgm, roff = syms["set_game_mode"]
    cur, _ = syms["gCurrentDisplayContextIndex"]
    phys = lambda a_: a_ & 0x1FFFFFFF
    rom = open(ROM, "rb").read()
    if rom[:4] != b"\x80\x37\x12\x40":
        sys.exit("n64ref: ROM is not big-endian z64: header %s" % rom[:4].hex())
    code = rom[roff:roff + 64]
    pat = b"".join(code[i:i + 4][::-1] for i in range(0, 64, 4))  # expected native-word layout
    out = a.out or os.path.join(ROOT, "build3ds", "out", "n64ref", name)
    os.makedirs(out, exist_ok=True)
    p = subprocess.Popen([EXE, ROM] + (["--load-state", str(a.state)] if a.state is not None else []))
    h = None
    try:
        h = k32.OpenProcess(PROC_ACCESS, False, p.pid)
        if not h:
            sys.exit("n64ref: OpenProcess failed, winerror %d" % ctypes.get_last_error())
        bases, t0 = [], time.time()
        while not bases and time.time() - t0 < 90:
            time.sleep(0.5)
            bases = find_rdram(h, pat, phys(sgm))
        if len(bases) != 1:
            sys.exit("n64ref: RDRAM candidates %s (expected 1)" % [hex(b) for b in bases])
        base = bases[0]
        frames = wait_frames(h, base, phys(cur), a.frames, a.cap)
        ntdll.NtSuspendProcess(h)
        mem = read(h, base, RDRAM_SIZE)
        ntdll.NtResumeProcess(h)
        if mem is None:
            sys.exit("n64ref: RDRAM read failed")
        dump = be(mem)
        code_ok = dump[phys(sgm):phys(sgm) + 64] == code
        g = phys(gs)
        area_id = int.from_bytes(dump[g + 0x86:g + 0x88], "big", signed=True)
        map_id = int.from_bytes(dump[g + 0x8C:g + 0x8E], "big", signed=True)
        entry_id = int.from_bytes(dump[g + 0x8E:g + 0x90], "big", signed=True)
        info = {"map": name, "entry": entry or None, "expected": {"areaID": area, "mapID": mid},
                "areaID": area_id, "mapID": map_id, "entryID": entry_id, "frames": frames,
                "rdram_base": hex(base), "code_bytes_ok": code_ok, "state": a.state}
        with open(os.path.join(out, "rdram.bin"), "wb") as f:
            f.write(dump)
        with open(os.path.join(out, "info.json"), "w") as f:
            json.dump(info, f, indent=1)
        print(json.dumps(info))
        return 0 if code_ok and area_id == area and map_id == mid else 2
    finally:
        if h:
            k32.CloseHandle(h)
        p.kill()
        p.wait()

if __name__ == "__main__":
    sys.exit(main())
