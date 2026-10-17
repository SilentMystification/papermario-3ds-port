#!/usr/bin/env python3
"""G01g golden frames. Run fixed scenes with warp and turbo, take one frame, compare with 3ds/golden/<scene>.png.

  python 3ds/tools/golden.py                 compare all scenes, print a table
  python 3ds/tools/golden.py toad yard       only these scenes
  python 3ds/tools/golden.py --update [..]   save the frame as the new reference
  python 3ds/tools/golden.py --list

Each scene is one Azahar run (about 25 s, turbo). The frame number is absolute (pm_frame_count), the game is frame
driven, so the same switches give the same frame. A pixel differs when a channel differs by more than TOL.
A scene fails when more than PCT percent of the pixels differ. Exit 0 = all ok, 1 = a scene differs, 2 = a run failed.
"""
import subprocess, sys, os
import numpy as np
from PIL import Image

TOL, PCT = 8, 0.5
PCT_SCENE = {"battle": 3.0}  # the battle frame is not exactly the same in every run (random effects, seen 1.06 percent)
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GOLD = os.path.join(ROOT, "3ds", "golden")
SHOT = os.path.join(ROOT, "build3ds", "shots", "0000.ppm")
PAUSE = "wait80,START*3,wait100"  # pause open in toad (scene frame 0 = frame 51)
# name: (switches, first shot frame). Three frames are taken (F, F+1, F+2), the first one is compared.
SCENES = {
    "title":      ("fast", 400),
    "fileselect": ("fast padscript=wait200,START*6,wait400", 500),
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
    dbg = f"{sw} shots shotwin={f}:3"
    r = subprocess.run(["powershell", "-File", os.path.join(ROOT, "3ds", "run_azahar.ps1"), "-Turbo", "-FaultStop",
                        "-Timeout", "90", "-Until", "shot 2 frame", "-Debug", dbg], capture_output=True, text=True, cwd=ROOT)
    if "matched 'shot 2 frame'" not in r.stdout or not os.path.exists(SHOT):
        return None, r.stdout.strip().splitlines()[-3:]
    return Image.open(SHOT).convert("RGB"), None

def main():
    a = sys.argv[1:]
    update = "--update" in a
    names = [x for x in a if not x.startswith("--")] or list(SCENES)
    if "--list" in a:
        for n, (s, f) in SCENES.items(): print(f"{n:11} frame {f:4} {s}")
        return 0
    os.makedirs(GOLD, exist_ok=True)
    rows, bad, failed = [], 0, 0
    for n in names:
        img, err = run(n)
        ref = os.path.join(GOLD, n + ".png")
        if img is None:
            rows.append((n, "RUN FAILED", "", "", " ".join(err))); failed += 1; continue
        if update or not os.path.exists(ref):
            img.save(ref); rows.append((n, "saved", "", "", "")); continue
        d = np.abs(np.asarray(img, np.int16) - np.asarray(Image.open(ref).convert("RGB"), np.int16)).max(axis=2)
        pct = 100.0 * (d > TOL).mean()
        st = "DIFF" if pct > PCT_SCENE.get(n, PCT) else "ok"
        bad += st == "DIFF"
        rows.append((n, st, f"{pct:.2f}", f"{int(d.max())}", f"mean {d.mean():.2f}"))
    print(f"{'scene':11} {'status':10} {'%px>'+str(TOL):>8} {'max':>4}  note")
    for r in rows: print(f"{r[0]:11} {r[1]:10} {r[2]:>8} {r[3]:>4}  {r[4]}")
    return 2 if failed else 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
