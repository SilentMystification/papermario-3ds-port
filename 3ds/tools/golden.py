#!/usr/bin/env python3
"""G01g golden frames. Run fixed scenes with warp and turbo, take one frame, compare with 3ds/golden/<scene>.png.

  python 3ds/tools/golden.py toad yard       compare these scenes (at most 3 per call; name the touched scenes)
  python 3ds/tools/golden.py --milestone     full set, once per 3dsx sha256
  A scene that passed on the same 3dsx sha256 is not run again ("cached ok").
  python 3ds/tools/golden.py --update [..]   save the frame as the new reference
  python 3ds/tools/golden.py --list

Each scene is one Azahar run (about 25 s, turbo). The frame number is absolute (pm_frame_count), the game is frame
driven, so the same switches give the same frame. A pixel differs when a channel differs by more than TOL.
A scene fails when more than PCT percent of the pixels differ. Exit 0 = all ok, 1 = a scene differs, 2 = a run failed.
"""
import subprocess, sys, os, json, hashlib
import numpy as np
from PIL import Image

TOL, PCT = 8, 0.5
PCT_SCENE = {"battle": 3.0}  # the battle frame is not exactly the same in every run (random effects, seen 1.06 percent)
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GOLD = os.path.join(ROOT, "3ds", "golden")
LANE = os.environ.get("PM_LANE")
SHOT = os.path.join(ROOT, "build3ds", *(["lane-" + LANE] if LANE else []), "shots", "0000.ppm")
PAUSE = "wait80,START*3,wait100"  # pause open in toad (scene frame 0 = frame 51)
# name: (switches, first shot frame). Three frames are taken (F, F+1, F+2), the first one is compared.
SCENES = {
    "title":      ("fast", 400),
    "fileselect": ("fast padscript=wait200,START*6,wait300,START*6,wait300", 400),
    "intro":      ("fast preset=intro", 183),
    "toad":       ("fast preset=toad", 251),
    "yard":       ("fast preset=yard", 249),
    "battle":     ("fast preset=toad battle", 300),
    "pause1":     (f"fast preset=toad padscript={PAUSE}", 51 + 130),
    "pause2":     (f"fast preset=toad padscript={PAUSE},R*3,wait80", 51 + 175),
    "pause3":     (f"fast preset=toad padscript={PAUSE},R*3,wait80,R*3,wait80", 51 + 215),
    "pause4":     (f"fast preset=toad padscript={PAUSE},R*3,wait80,R*3,wait80,R*3,wait80", 51 + 255),
}

def run(name):
    sw, f = SCENES[name]
    dbg = f"{sw} fixedseed shots shotwin={f}:3"  # R-SEED: a normal boot has a random RNG seed, tests use the fixed one
    if os.path.exists(SHOT): os.remove(SHOT)  # a stale shot from an earlier run must never be read
    r = subprocess.run(["powershell", "-File", os.path.join(ROOT, "3ds", "run_azahar.ps1"), "-Turbo", "-FaultStop",
                        "-Timeout", "90", "-Until", "shot 2 frame", "-Debug", dbg], capture_output=True, text=True, cwd=ROOT)
    if "matched 'shot 2 frame'" not in r.stdout or not os.path.exists(SHOT):
        return None, r.stdout.strip().splitlines()[-3:]
    return Image.open(SHOT).convert("RGB"), None

CACHE = os.path.join(ROOT, "3ds", "logs", "golden_ok.json")
def load_cache():
    try: return json.load(open(CACHE))
    except Exception: return {}

def img_bytes(img):
    import io
    b = io.BytesIO(); img.save(b, "PNG"); return b.getvalue()

def main():
    a = sys.argv[1:]
    update = "--update" in a
    milestone = "--milestone" in a
    names = [x for x in a if not x.startswith("--")]
    if "--list" in a:
        for n, (s, f) in SCENES.items(): print(f"{n:11} frame {f:4} {s}")
        return 0
    x3 = os.path.join(ROOT, "build3ds", *(["lane-" + LANE] if LANE else []), "pm_3ds.3dsx")
    sha = hashlib.sha256(open(x3, "rb").read()).hexdigest()
    cache = load_cache()
    ent = cache.setdefault(sha, {"ok": [], "milestone": False})
    if milestone:
        if ent["milestone"]: print(f"REFUSED: --milestone already ran on this 3dsx ({sha[:12]})."); return 2
        names = list(SCENES); ent["milestone"] = True
    elif not names or "all" in names or len(names) > 3:
        print("REFUSED: name 1 to 3 touched scenes (not 'all'). Use --milestone for the full set, once per build."); return 2
    unknown = [n for n in names if n not in SCENES]
    if unknown: print("REFUSED: unknown scene", unknown); return 2
    os.makedirs(GOLD, exist_ok=True)
    rows, bad, failed = [], 0, 0
    for n in names:
        if not update and n in ent["ok"] and not milestone:
            rows.append((n, "cached ok", "", "", "same 3dsx")); continue
        img, err = run(n)
        ref = os.path.join(GOLD, n + ".png")
        if img is None:
            rows.append((n, "RUN FAILED", "", "", " ".join(err))); failed += 1; continue
        if update or not os.path.exists(ref):
            same = [o for o in SCENES if o != n and os.path.exists(os.path.join(GOLD, o + ".png"))
                    and open(os.path.join(GOLD, o + ".png"), "rb").read() == img_bytes(img)]
            if same:
                rows.append((n, "REFUSED", "", "", "equals reference of " + ",".join(same))); failed += 1; continue
            img.save(ref); rows.append((n, "saved", "", "", "")); continue
        d = np.abs(np.asarray(img, np.int16) - np.asarray(Image.open(ref).convert("RGB"), np.int16)).max(axis=2)
        pct = 100.0 * (d > TOL).mean()
        st = "DIFF" if pct > PCT_SCENE.get(n, PCT) else "ok"
        bad += st == "DIFF"
        if st == "ok" and n not in ent["ok"]: ent["ok"].append(n)
        rows.append((n, st, f"{pct:.2f}", f"{int(d.max())}", f"mean {d.mean():.2f}"))
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    json.dump(cache, open(CACHE, "w"))
    print(f"{'scene':11} {'status':10} {'%px>'+str(TOL):>8} {'max':>4}  note")
    for r in rows: print(f"{r[0]:11} {r[1]:10} {r[2]:>8} {r[3]:>4}  {r[4]}")
    return 2 if failed else 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
