"""test_pm_logrecv.py - local check of pm_logrecv.receive (no console, no network beyond 127.0.0.1).

Run: python 3ds/tools/test_pm_logrecv.py   (prints OK on pass, exit 1 on fail)

A local server thread plays the game: it reads "sendlog\\n", then sends live text, a 10-byte crash.txt frame,
more text and a 100000-byte log.txt frame, split into odd-sized chunks. The test runs pm_logrecv.receive on
the client side and checks the three output files and their sizes. Two cheap cases follow: a silent peer
(timeout) and a peer that closes early (closed).
"""
import contextlib
import io
import os
import shutil
import socket
import sys
import tempfile
import threading
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pm_logrecv  # noqa: E402

LOG_PAYLOAD = bytes((i * 7 + 3) % 256 for i in range(100000))
CRASH_PAYLOAD = b"0123456789"
ODD_CHUNKS = [1, 3, 7, 13, 101, 997, 4099]


def build_stream(lead, first):
    # lead: the game form (a newline before each #PMFILE header and before each #PMEND); first: live text before the frames.
    nl = b"\n" if lead else b""
    parts = [
        first,
        nl + b"#PMFILE crash.txt 10\n" + CRASH_PAYLOAD + nl + b"#PMEND crash.txt\n",
        b"more text\n",
        nl + b"#PMFILE log.txt %d\n" % len(LOG_PAYLOAD) + LOG_PAYLOAD + nl + b"#PMEND log.txt\n",
    ]
    return b"".join(parts)


def serve(listener, blob):
    conn, _ = listener.accept()
    try:
        req = b""
        while not req.endswith(b"\n"):
            chunk = conn.recv(64)
            if not chunk:
                return
            req += chunk
        assert req == b"sendlog\n", req
        pos = 0
        i = 0
        while pos < len(blob):
            n = ODD_CHUNKS[i % len(ODD_CHUNKS)]
            conn.sendall(blob[pos:pos + n])
            pos += n
            i += 1
            time.sleep(0.002)
    except OSError:
        pass
    finally:
        conn.close()


def read_file(path):
    with open(path, "rb") as f:
        return f.read()


def check_stream(out_dir, label, lead, first, live_want):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]
    t = threading.Thread(target=serve, args=(listener, build_stream(lead, first)), daemon=True)
    t.start()
    client = socket.create_connection(("127.0.0.1", port), timeout=8)
    try:
        client.sendall(b"sendlog\n")
        got = []
        pm_logrecv.receive(client, label, out_dir, 20.0, got)
    finally:
        client.close()
        listener.close()
        t.join(timeout=10)
    assert got == ["crash.txt", "log.txt"], got

    live = read_file(os.path.join(out_dir, "hw_%s_live.txt" % label))
    assert live == live_want, (label, live)
    crash = read_file(os.path.join(out_dir, "hw_%s_crash.txt" % label))
    assert len(crash) == 10 and crash == CRASH_PAYLOAD, (len(crash), crash)
    log = read_file(os.path.join(out_dir, "hw_%s_log.txt" % label))
    assert len(log) == 100000 and log == LOG_PAYLOAD, (len(log), log[:16])


def check_timeout(out_dir):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]
    accepted = []

    def silent():
        conn, _ = listener.accept()
        accepted.append(conn)
        time.sleep(3)

    t = threading.Thread(target=silent, daemon=True)
    t.start()
    client = socket.create_connection(("127.0.0.1", port), timeout=8)
    got = []
    try:
        client.sendall(b"sendlog\n")
        try:
            pm_logrecv.receive(client, "silent", out_dir, 0.5, got)
        except socket.timeout:
            pass
        else:
            raise AssertionError("expected timeout")
    finally:
        client.close()
        listener.close()
        t.join(timeout=10)
    assert got == [], got


def check_closed(out_dir):
    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.bind(("127.0.0.1", 0))
    listener.listen(1)
    port = listener.getsockname()[1]

    def close_now():
        conn, _ = listener.accept()
        conn.recv(64)
        conn.sendall(b"#PMFILE crash.txt 10\n0123")
        conn.close()

    t = threading.Thread(target=close_now, daemon=True)
    t.start()
    client = socket.create_connection(("127.0.0.1", port), timeout=8)
    got = []
    try:
        client.sendall(b"sendlog\n")
        try:
            pm_logrecv.receive(client, "closed", out_dir, 10.0, got)
        except pm_logrecv._Closed:
            pass
        else:
            raise AssertionError("expected closed")
    finally:
        client.close()
        listener.close()
        t.join(timeout=10)
    assert got == [], got


def check_hello():
    off = pm_logrecv.hello_diag(b"#PMLIVE log_on 0 live 3 disk 3 prof 0")
    assert off and off.startswith("[live] GAME LOGGING OFF (#PMLIVE log_on 0 live 3 disk 3 prof 0)"), off
    assert pm_logrecv.hello_diag(b"#PMLIVE log_on 1 live 3 disk 3 prof 0") is None
    assert pm_logrecv.hello_diag(b"#PMLIVE log_on 0 live -1 disk -1 prof 1") is None
    assert pm_logrecv.hello_diag(b"#12 build 5824") is None
    want = ("[logrecv] no answer on 192.168.1.131:17492 (timeout): the game has no live socket (livelog missing), "
            "or the accept loop ended, or the network path: see log.old/FTP")
    assert pm_logrecv.connect_diag("192.168.1.131", 17492, "timeout") == want


def check_retry(out_dir):
    # Seed: no retry to a closed port gives the refused line and exit 1. Then --retry reaches a server that starts 1.5 s late.
    p = socket.socket()
    p.bind(("127.0.0.1", 0))
    port = p.getsockname()[1]
    p.close()
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        rc = pm_logrecv.main(["--ip", "127.0.0.1", "--port", str(port), "--out", out_dir, "--label", "R0"])
    assert rc == 1, rc
    assert buf.getvalue().startswith("[logrecv] refused on 127.0.0.1:%d: nothing listens" % port), buf.getvalue()

    def late():
        time.sleep(1.5)
        ls = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        ls.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        ls.bind(("127.0.0.1", port))
        ls.listen(1)
        try:
            serve(ls, build_stream(True, b"#PMLIVE log_on 1 live 3 disk 3 prof 0\n"))
        finally:
            ls.close()
    t = threading.Thread(target=late, daemon=True)
    t.start()
    rc = pm_logrecv.main(["--ip", "127.0.0.1", "--port", str(port), "--out", out_dir, "--label", "R1", "--retry", "8", "--timeout", "20"])
    t.join(timeout=20)
    assert rc == 0, rc
    assert read_file(os.path.join(out_dir, "hw_R1_log.txt")) == LOG_PAYLOAD


def main():
    base = tempfile.mkdtemp(prefix="logrecv_test_")
    try:
        # A: game form. B: older form without the leading newlines. C: a partial live line before the game-form frames.
        check_stream(base, "A", True, b"live one\n", b"live one\nmore text\n")
        check_stream(base, "B", False, b"live one\n", b"live one\nmore text\n")
        check_stream(base, "C", True, b"partial", b"partial\nmore text\n")
        check_timeout(base)
        check_closed(base)
        check_hello()
        check_retry(base)
    finally:
        shutil.rmtree(base, ignore_errors=True)
    print("OK")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as e:
        print("FAIL: %s" % (e,))
        sys.exit(1)
