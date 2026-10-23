#!/usr/bin/env python3
"""sweep_sheet.py - one contact sheet per item of a full sweep: 3ds/logs/sweep/<item>/sheet.png with 6 to 12 thumbnails (an even pick of the shots, top screen).
Use: sweep_sheet.py [--dir 3ds/logs/sweep] [--n 12]. The shots are PPM files (240 x 400, turned by 90 degrees) that the `shots` switch writes."""
import os, sys, argparse
from PIL import Image, ImageDraw

def sheet(d, n):
    sd = os.path.join(d, 'shots')
    if not os.path.isdir(sd): return False
    fs = sorted(f for f in os.listdir(sd) if f.endswith('.ppm'))[1:] # shot 0000 is the boot test triangle
    if os.path.basename(d).startswith('credits_') and os.path.basename(d) != 'credits_full': fs = fs[-20:] # the scene after the jump
    if not fs: return False
    k = min(n, max(6, len(fs))) if len(fs) >= 6 else len(fs)
    pick = [fs[min(len(fs) - 1, int(i * len(fs) / k))] for i in range(k)] if k < len(fs) else fs
    cols = 4 if len(pick) > 6 else 3
    rows = (len(pick) + cols - 1) // cols
    tw, th = 200, 120
    img = Image.new('RGB', (cols * tw, rows * (th + 14)), (24, 24, 24))
    dr = ImageDraw.Draw(img)
    for i, f in enumerate(pick):
        try: im = Image.open(os.path.join(sd, f)).convert('RGB').transpose(Image.ROTATE_90).resize((tw, th))
        except Exception: continue
        x, y = (i % cols) * tw, (i // cols) * (th + 14)
        img.paste(im, (x, y + 14)); dr.text((x + 3, y + 1), '%s #%s' % (os.path.basename(d)[:24], f[:-4]), fill=(230, 230, 230))
    img.save(os.path.join(d, 'sheet.png'))
    return True

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--dir', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'logs', 'sweep')); ap.add_argument('--n', type=int, default=12)
    a = ap.parse_args()
    n = 0
    for name in sorted(os.listdir(a.dir)):
        d = os.path.join(a.dir, name)
        if os.path.isdir(d) and sheet(d, a.n): n += 1
    print('%d sheets in %s' % (n, a.dir))

if __name__ == '__main__':
    main()
