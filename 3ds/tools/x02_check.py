"""X02 text check: run a warp with shots, print pixel stats (corners vs centre) of the first shot."""
import subprocess, sys, os
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SHOT = os.path.join(ROOT, "build3ds", "shots", "0000.ppm")
if os.path.exists(SHOT): os.remove(SHOT)
sw = sys.argv[1]; f = sys.argv[2] if len(sys.argv) > 2 else "200"
r = subprocess.run(["powershell", "-File", "3ds/run_azahar.ps1", "-Turbo", "-Smoke", "-FaultStop", "-Timeout", "90",
                    "-Until", "shot 2 frame", "-Debug", f"{sw} shots shotwin={f}:3"], capture_output=True, text=True, cwd=ROOT)
print("exit", r.returncode)
d = open(SHOT, "rb").read()
hd = d.split(bytes([10]), 1); w, h = map(int, hd[0].split()[1:3]); px = hd[1]
def mean(x0, y0, x1, y1):
    t = n = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            i = (y * w + x) * 3; t += px[i] + px[i+1] + px[i+2]; n += 3
    return t / n
q = 20
print(f"{w}x{h} corners", [round(mean(a, b, a+q, b+q)) for a, b in [(0,0),(w-q,0),(0,h-q),(w-q,h-q)]],
      "centre", round(mean(w//2-q, h//2-q, w//2+q, h//2+q)), "mean", round(mean(0,0,w,h)))
