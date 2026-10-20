"""decal_diff.py A_DIR B_DIR [threshold]: per frame, count the pixels whose channel differs by more than threshold (default 24).
Compares NNNN.ppm of the two shot folders. Prints one line per frame and the total."""
import sys, glob, os
from PIL import Image, ImageChops
a, b = sys.argv[1], sys.argv[2]
th = int(sys.argv[3]) if len(sys.argv) > 3 else 24
tot = 0
for fa in sorted(glob.glob(os.path.join(a, '*.ppm'))):
    fb = os.path.join(b, os.path.basename(fa))
    if not os.path.exists(fb):
        continue
    d = ImageChops.difference(Image.open(fa).convert('RGB'), Image.open(fb).convert('RGB'))
    n = sum(1 for p in d.getdata() if max(p) > th)
    tot += n
    print(os.path.basename(fa), n)
print('total', tot)
