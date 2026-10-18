"""Check that facts from an old doc survive in a trimmed doc.

Usage:
    python doc_factcheck.py OLD NEW [SEARCH ...]

OLD is the old file (for example from `git show HEAD:<path>`).
NEW is the trimmed file.
Each SEARCH is a file or directory where a missing fact may be moved.

Facts are numbers, paths, backticked terms, row ids (H05, B1, PO3)
and commit hashes. The script prints each fact that is in OLD and
missing from NEW. For each missing fact it checks the SEARCH files.
It prints the facts that no SEARCH file holds, then the counts.
Exit code 0 when no fact is unresolved, 1 otherwise.
"""
import os
import re
import sys

NUM = re.compile(r"\d+(?:[.,]\d+)*")
PATH = re.compile(r"[\w.\-]+(?:/[\w.\-]+)+|[\w\-]+\.(?:md|py|ps1|sh|c|h|txt|log|cmake|ld|lds|json|pica|z64|3dsx|png|cdc)\b")
CODE = re.compile(r"`([^`\n]+)`")
ROW = re.compile(r"\b[A-Z]{1,2}\d{1,2}[a-z]?\b")
HASH = re.compile(r"\b(?=[0-9a-f]*\d)(?=[0-9a-f]*[a-f])[0-9a-f]{7,40}\b")


def read(path):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return re.sub(r"\s+", " ", fh.read())


def read_tree(path):
    if os.path.isdir(path):
        out = []
        for root, _dirs, files in os.walk(path):
            for name in files:
                if name.endswith(".md"):
                    out.append(read(os.path.join(root, name)))
        return " ".join(out)
    return read(path)


def facts(text):
    """Return a set of (kind, item) pairs found in text."""
    out = set()
    for m in NUM.findall(text):
        out.add(("num", m))
    for m in PATH.findall(text):
        out.add(("path", m))
    for m in CODE.findall(text):
        out.add(("code", m.strip()))
    for m in ROW.findall(text):
        out.add(("row", m))
    for m in HASH.findall(text):
        out.add(("hash", m))
    return out


def present(kind, item, text_tokens, text_flat):
    if kind in ("num", "row", "hash"):
        return item in text_tokens
    return item in text_flat


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    old_path, new_path = argv[1], argv[2]
    search_paths = argv[3:]
    old_text = read(old_path)
    new_text = read(new_path)
    new_tokens = set()
    for kind, item in facts(new_text):
        if kind in ("num", "row", "hash"):
            new_tokens.add(item)
    missing = []
    for kind, item in sorted(facts(old_text)):
        if not present(kind, item, new_tokens, new_text):
            missing.append((kind, item))
    search_texts = [(p, read_tree(p)) for p in search_paths if os.path.exists(p)]
    unresolved = []
    for kind, item in missing:
        found = False
        for _p, txt in search_texts:
            toks = {i for k, i in facts(txt) if k in ("num", "row", "hash")}
            if present(kind, item, toks, txt):
                found = True
                break
        print(f"MISSING {kind}: {item}" + ("" if found else "  [UNRESOLVED]"))
        if not found:
            unresolved.append((kind, item))
    print(f"old={old_path} new={new_path} missing={len(missing)} unresolved={len(unresolved)}")
    return 0 if not unresolved else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
