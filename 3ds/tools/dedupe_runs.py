"""dedupe_runs.py <runs.jsonl>: keep every line that contains "run_id" and the first copy of each other exact line.
Writes <file>.new, checks the run-line count, then replaces the file. Usage: python -I dedupe_runs.py <path>"""
import os, sys

p = sys.argv[1]
seen = set()
n_in = n_out = r_in = r_out = 0
with open(p, "rb") as f, open(p + ".new", "wb") as o:
    for line in f:
        n_in += 1
        is_run = b'"run_id"' in line
        if is_run:
            r_in += 1
        elif line in seen:
            continue
        else:
            seen.add(line)
        o.write(line)
        n_out += 1
        if is_run:
            r_out += 1
if r_in != r_out:
    print("FAIL run lines", r_in, r_out)
    sys.exit(1)
os.replace(p + ".new", p)
print("lines", n_in, "->", n_out, "run lines", r_in, "->", r_out, "bytes", os.path.getsize(p))
