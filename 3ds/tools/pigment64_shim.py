"""Stand-in for the Rust pigment64 CLI (not installable here): `pigment64 to-bin [--flip-y] -f FMT -o OUT IN`.
Calls the repo's own converter tools/build/img/build.py."""
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / "tools/build/img"))
from build import Converter

a = sys.argv[2:]
fmt, out, inp = a[a.index("-f") + 1], a[a.index("-o") + 1], a[-1]
data, _, _ = Converter(fmt, inp, "--flip-x" in a, "--flip-y" in a).convert()
Path(out).write_bytes(data)
