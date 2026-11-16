"""pm_logrecv.py - receive log.txt and crash.txt from the game over the live socket (TCP 17492).

Usage: python 3ds/tools/pm_logrecv.py --ip <3ds-ip> [--label o3ds] [--timeout 60] [--out build3ds] [--retry 0] [--port 17492]

Use this when FTP is not available (see pull text in 3ds/go.ps1). The game must be running with the live
log on. The tool sends "sendlog\\n". The game answers with live text lines and then frames:

  \\n#PMFILE <name> <size>\\n <size> raw bytes \\n#PMEND <name>\\n   (LOGX4: the game writes a newline before the header and before #PMEND)

Names: crash.txt and log.txt. A crash.txt frame may come before any live text at the first client of a boot.

Writes:
  <out>/hw_<label>_live.txt   live text lines (outside frames)
  <out>/hw_<label>_crash.txt  crash.txt frame (a repeated frame overwrites)
  <out>/hw_<label>_log.txt    log.txt frame

Stops after the log.txt frame, or at the timeout (exit 2). Python standard library only.
"""
import argparse
import os
import socket
import sys
import time

PORT = 17492
FRAME_PREFIX = b"#PMFILE "
END_PREFIX = b"#PMEND "
KIND = {b"log.txt": "log", b"crash.txt": "crash"}


HELLO = b"#PMLIVE "


def connect_diag(ip, port, cls):
    """CAPTOOL: one line for a failed connect. It never says that the console is down."""
    if cls == "refused":
        return "[logrecv] refused on %s:%d: nothing listens: the game is not running, or it started without the livelog word" % (ip, port)
    return ("[logrecv] no answer on %s:%d (%s): the game has no live socket (livelog missing), or the accept loop ended, "
            "or the network path: see log.old/FTP" % (ip, port, cls))


def hello_diag(line):
    """CAPTOOL: the first game line of a connection (pm_livesrv.h). A warning when the game logs nothing, else None."""
    if not line.startswith(HELLO):
        return None
    f = line.split()
    kv = dict(zip(f[1::2], f[2::2]))
    if kv.get(b"log_on") == b"0" and kv.get(b"prof") == b"0":
        return ("[live] GAME LOGGING OFF (%s): no log line follows. Add the launch word log=on (push --profile log has it) "
                "or set Options > Logging On" % line.decode("ascii", "replace"))
    return None


class _Closed(Exception):
    pass


def receive(sock, label, out_dir, timeout, got):
    """Read the game stream from a connected socket until the log.txt frame is complete.

    Each received file name is appended to `got`, so the caller can report it after an exception.
    Raises socket.timeout at the deadline, _Closed when the peer closes first, ValueError on a bad frame.
    """
    os.makedirs(out_dir, exist_ok=True)
    live_path = os.path.join(out_dir, "hw_%s_live.txt" % label)
    deadline = time.monotonic() + timeout
    buf = bytearray()

    def fill(need):
        # Read until buf holds at least `need` bytes.
        while len(buf) < need:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise socket.timeout("deadline")
            sock.settimeout(remaining)
            chunk = sock.recv(65536)
            if not chunk:
                raise _Closed()
            buf.extend(chunk)

    def next_line():
        # Return one complete line (newline removed) from buf, reading as needed.
        while True:
            nl = buf.find(b"\n")
            if nl >= 0:
                line = bytes(buf[:nl])
                del buf[:nl + 1]
                return line
            fill(len(buf) + 1)

    with open(live_path, "wb") as live:
        # LOGX4: an empty live line is held. The game writes "\n#PMFILE" before each frame, so a held empty line
        # followed by a #PMFILE header is that newline and is dropped; any other line makes the held line real.
        held = False
        try:
            while True:
                line = next_line()
                if held:
                    held = False
                    if not line.startswith(FRAME_PREFIX):
                        live.write(b"\n")
                        live.flush()
                if not line.startswith(FRAME_PREFIX):
                    if line == b"":
                        held = True
                        continue
                    d = hello_diag(line)
                    if d:
                        print(d, flush=True)
                    live.write(line + b"\n")
                    live.flush()
                    continue
                parts = line[len(FRAME_PREFIX):].split(b" ")
                if len(parts) < 2:
                    raise ValueError("bad frame header: %r" % line[:80])
                name = parts[0]
                size = int(parts[1])
                fill(size)
                data = bytes(buf[:size])
                del buf[:size]
                end = next_line()
                if end == b"":
                    end = next_line()  # LOGX4: the game's trailer starts with a newline (pm_log.c send_file)
                if end != END_PREFIX + name:
                    raise ValueError("bad frame end for %s: %r" % (name.decode("ascii", "replace"), end[:80]))
                if name not in KIND:
                    raise ValueError("unknown frame name: %s" % name.decode("ascii", "replace"))
                path = os.path.join(out_dir, "hw_%s_%s.txt" % (label, KIND[name]))
                with open(path, "wb") as f:
                    f.write(data)
                text = name.decode("ascii")
                print("[logrecv] %s -> %s (%d bytes)" % (text, path, size), flush=True)
                got.append(text)
                if name == b"log.txt":
                    return
        finally:
            if held:
                live.write(b"\n")  # LOGX4: a held empty line at the end (closed or timeout) is real text


def _names(got):
    return ", ".join(got) if got else "none"


def main(argv=None):
    ap = argparse.ArgumentParser(description="Receive log.txt and crash.txt over the game live socket.")
    ap.add_argument("--ip", required=True)
    ap.add_argument("--label", default="o3ds")
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--out", default="build3ds")
    ap.add_argument("--port", type=int, default=PORT)  # CAPTOOL: the tests use a local port
    ap.add_argument("--retry", type=float, default=0.0, help="retry the connect for this many seconds, 1 s apart")
    args = ap.parse_args(argv)

    end = time.monotonic() + args.retry
    while True:
        try:
            sock = socket.create_connection((args.ip, args.port), timeout=8)
            break
        except OSError as e:
            cls = "refused" if isinstance(e, ConnectionRefusedError) else "timeout" if isinstance(e, socket.timeout) else "error"
            if time.monotonic() >= end:
                print(connect_diag(args.ip, args.port, cls), flush=True)
                return 1
            time.sleep(1)
    got = []
    try:
        sock.sendall(b"sendlog\n")
        receive(sock, args.label, args.out, args.timeout, got)
    except socket.timeout:
        print("[logrecv] timeout: got %s" % _names(got))
        return 2
    except _Closed:
        print("[logrecv] closed before log.txt: got %s" % _names(got))
        return 2
    except ValueError as e:
        print("[logrecv] protocol error: %s" % e)
        return 1
    finally:
        sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
