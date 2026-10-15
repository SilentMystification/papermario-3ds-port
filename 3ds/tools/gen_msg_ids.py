"""Regenerate 3ds/stub_include/message_ids.h from tools/splat_ext/msg.yaml."""
import re, pathlib
root = pathlib.Path(__file__).resolve().parents[2]
out = ["#ifndef MESSAGE_IDS_H", "#define MESSAGE_IDS_H"]
seen = set()
for line in (root / "tools/splat_ext/msg.yaml").read_text().splitlines():
    m = re.match(r"- \[(0x[0-9A-Fa-f]+), (0x[0-9A-Fa-f]+), (\w+)\]$", line)
    if not m or m[3] in seen:
        continue
    seen.add(m[3])
    out.append(f"#define MSG_{m[3]} 0x{(int(m[1], 16) << 16) | int(m[2], 16):08X}")
out.append("#endif")
(root / "3ds/stub_include/message_ids.h").write_text("\n".join(out) + "\n")
print(len(seen))
