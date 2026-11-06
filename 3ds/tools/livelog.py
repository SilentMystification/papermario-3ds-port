"""livelog.py - read the game's live log from a 3DS (or Azahar) over the network (TCP 17492).

Usage: py -3 -u 3ds/tools/livelog.py <3ds-ip> [seconds] [out-file] [--silence <seconds>] [--append]
       py -3 -u 3ds/tools/livelog.py --probe <3ds-ip>   (one TCP connect to 17492, prints its class, no read)
--append adds to out-file (a reconnect of the same run; the file is not truncated).
Connect errors are printed with a class: refused (the game's server is not listening), timeout, unreachable
(no route or host down), error (anything else). A failed connect is printed on a class change and at most every
10 s; with no connection at all the last class is printed as "NO LOG: last error <class>".
Exit codes: 0 time limit or a game end line, 1 no connection at all, 3 the read ended early (silent, or the connection
stayed closed), so push_3ds.sh reconnects with --append.
On connect the game sends the last 32 KB of its log, then each new line. Each line starts with
"#<seq> ". After a reconnect, lines already received are skipped by that number, and a gap in
the numbers is reported, so a lost line is never silent. Writes raw bytes, so Japanese debug
text cannot break the Windows console.
The read stops early on a game end line (END_LINES), or when the connection stays closed for
GONE_SECS seconds after the last received data, or when no byte arrives from the console for
--silence seconds (default 60; the console may be off with the connection still open).
The final line is "live log: ended (<reason>)".
"""
import errno
import re
import socket
import sys
import time

SEQ = re.compile(rb"^#(\d+) ")
# Game end lines (3ds/src/pm_main.c, pm_log.c, pm_wdog.c). A match stops the read.
END_LINES = (
    (re.compile(rb"exit: audio job done"), "exit: audio job done"),
    (re.compile(rb"boot_main returned"), "boot_main returned"),
    (re.compile(rb"cpu limit: set \d+ \(exit \(main\)\)"), "cpu limit exit (main)"),
    (re.compile(rb"watchdog: exiting the process"), "watchdog: exiting the process"),
)
GONE_SECS = 5  # no data for this long after a close or loss ends the read
PORT = 17492
ERR_PRINT_SECS = 10  # a repeated connect error of the same class is printed at most this often
UNREACH = {errno.EHOSTUNREACH, errno.ENETUNREACH, getattr(errno, "EHOSTDOWN", -1), getattr(errno, "ENETDOWN", -1)}


def err_class(e):
    """Class of a connect error: refused, timeout, unreachable or error."""
    if isinstance(e, ConnectionRefusedError) or e.errno == errno.ECONNREFUSED or getattr(e, "winerror", None) == 10061:
        return "refused"
    if isinstance(e, TimeoutError) or getattr(e, "winerror", None) == 10060:
        return "timeout"
    if e.errno in UNREACH or getattr(e, "winerror", None) in (10051, 10064, 10065):
        return "unreachable"
    return "error"


def probe(ip):
    """One TCP connect to the log server. Exit 0 when it connects, 1 otherwise."""
    try:
        s = socket.create_connection((ip, PORT), timeout=3)
    except OSError as e:
        print("[livelog] probe %s:%d %s: %s" % (ip, PORT, err_class(e), e), flush=True)
        sys.exit(1)
    s.close()
    print("[livelog] probe %s:%d connected (the server listens; the log is then missing a line, see the game side)"
          % (ip, PORT), flush=True)
    sys.exit(0)


def main():
    args = sys.argv[1:]
    if args[:1] == ["--probe"]:
        probe(args[1])
    silence = 60.0  # no byte for this long ends the read
    if "--silence" in args:
        i = args.index("--silence")
        silence = float(args[i + 1])
        del args[i:i + 2]
    append = "--append" in args
    if append:
        args.remove("--append")
    ip = args[0]
    secs = float(args[1]) if len(args) > 1 else 0
    path = args[2] if len(args) > 2 else "build3ds/hw_live_log.txt"
    forever = secs <= 0
    end = time.time() + secs if not forever else 0
    out = open(path, "ab" if append else "wb")
    stdout = sys.stdout.buffer
    last_seq = -1
    gaps = 0
    connects = 0
    buf = b""
    ended = None  # reason text once the read must stop
    down_since = None  # time of the last close or loss with no data since
    last_data = 0.0  # time of the last byte from the console (or of the connect)
    err_cls, err_t = None, 0.0  # class and time of the last connect error printed
    last_err = None  # (class, text) of the last connect error, for the NO LOG line

    def emit(line):
        out.write(line)
        out.flush()
        stdout.write(line)
        stdout.flush()

    while ended is None and (forever or time.time() < end):
        try:
            sock = socket.create_connection((ip, PORT), timeout=2)
        except OSError as e:
            cls = err_class(e)
            last_err = (cls, str(e))
            if cls != err_cls or time.time() - err_t >= ERR_PRINT_SECS:
                err_cls, err_t = cls, time.time()
                print("[livelog] connect %s:%d %s: %s" % (ip, PORT, cls, e), flush=True)
            if down_since is not None and time.time() - down_since >= GONE_SECS:
                ended = "connection closed for %d s" % GONE_SECS
                break
            time.sleep(1)  # the game opens the server a few seconds after it starts
            continue
        connects += 1
        print("[livelog] connected" if connects == 1 else "[livelog] reconnected", flush=True)
        sock.settimeout(1)
        buf = b""
        last_data = time.time()
        while ended is None and (forever or time.time() < end):
            try:
                data = sock.recv(4096)
            except socket.timeout:
                if time.time() - last_data >= silence:
                    ended = "silent for %g s" % silence
                    break
                continue
            except OSError as e:
                print("[livelog] connection lost:", e, flush=True)
                if down_since is None:
                    down_since = time.time()
                break
            if not data:
                print("[livelog] game closed the connection", flush=True)
                if down_since is None:
                    down_since = time.time()
                break
            down_since = None
            last_data = time.time()
            buf += data.replace(b"\r", b"")
            *lines, buf = buf.split(b"\n")
            for line in lines:
                m = SEQ.match(line)
                if m:
                    seq = int(m.group(1))
                    if seq <= last_seq:
                        continue  # already received before the reconnect
                    if last_seq >= 0 and seq != last_seq + 1:
                        gaps += 1
                        emit(b"[livelog] GAP: lines #%d-#%d missing\n" % (last_seq + 1, seq - 1))
                    last_seq = seq
                emit(line + b"\n")
                if ended is None:
                    for pat, label in END_LINES:
                        if pat.search(line):
                            ended = label
                            break
        sock.close()
    if buf:
        emit(buf + b"\n")
    if not connects:
        print("[livelog] no connection to the game's log server (see the 'live log:' line on the bottom screen)")
        cls, msg = last_err or ("none", "no connect attempt")
        print("[livelog] NO LOG: last error %s: %s" % (cls, msg), flush=True)
        sys.exit(1)
    print("[livelog] done: last line #%d, %d gaps, %d connects" % (last_seq, gaps, connects), flush=True)
    print("[livelog] live log: ended (%s)" % (ended or "time limit"), flush=True)
    if ended and ended.startswith(("silent for", "connection closed")):
        sys.exit(3)


if __name__ == "__main__":
    main()
