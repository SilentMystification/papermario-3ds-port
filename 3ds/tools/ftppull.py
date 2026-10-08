"""ftppull.py - pull the game log and Luma crash dumps from a 3DS over FTP (port 5000).

Usage: py -3 -u 3ds/tools/ftppull.py <ip> <o|n> <out-dir>

One attempt, short timeout (see the project's never-block-on-live-device rule): this does not
retry or wait for the console to come online.

Downloads:
  sdmc:/3ds/PaperMario/log.txt  -> <out-dir>/hw_<o3ds|n3ds>_log.txt
  sdmc:/luma/dumps/arm11/*.dmp      -> <out-dir>/hw_<o3ds|n3ds>_dumps/
A missing file or directory (FTP 450/550, "no such file") is reported and skipped, not an error.
"""
import ftplib
import os
import sys

LABEL = {"o": "o3ds", "n": "n3ds"}


def main():
    if len(sys.argv) < 4:
        print("usage: ftppull.py <ip> <o|n> <out-dir>")
        sys.exit(1)
    ip, which, out_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    label = LABEL.get(which, which)
    os.makedirs(out_dir, exist_ok=True)

    try:
        ftp = ftplib.FTP()
        ftp.connect(ip, 5000, timeout=8)
        ftp.login()  # anonymous - the usual 3DS homebrew FTP servers take no credentials
    except Exception as e:
        print(f"[ftppull] could not connect to {ip}:5000: {e}")
        sys.exit(1)

    log_path = os.path.join(out_dir, f"hw_{label}_log.txt")
    try:
        with open(log_path, "wb") as f:
            ftp.retrbinary("RETR /3ds/PaperMario/log.txt", f.write)
        print(f"[ftppull] log.txt -> {log_path} ({os.path.getsize(log_path)} bytes)")
    except Exception as e:
        print(f"[ftppull] no log.txt ({e})")
        try:
            os.remove(log_path)
        except OSError:
            pass

    try:
        names = ftp.nlst("/luma/dumps/arm11")
    except Exception as e:
        print(f"[ftppull] no luma dumps ({e})")
        names = []

    dmp_names = [os.path.basename(n.replace("\\", "/")) for n in names if n.lower().endswith(".dmp")]
    if dmp_names:
        dumps_dir = os.path.join(out_dir, f"hw_{label}_dumps")
        os.makedirs(dumps_dir, exist_ok=True)
        for base in dmp_names:
            dest = os.path.join(dumps_dir, base)
            try:
                with open(dest, "wb") as f:
                    ftp.retrbinary(f"RETR /luma/dumps/arm11/{base}", f.write)
                print(f"[ftppull] {base} -> {dest}")
            except Exception as e:
                print(f"[ftppull] failed {base}: {e}")
    else:
        print("[ftppull] no crash dumps")

    ftp.quit()


if __name__ == "__main__":
    main()
