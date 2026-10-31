"""quitgame.py - ask the running game to quit through its normal exit path (TCP 17492).

Usage: py -3 -u 3ds/tools/quitgame.py <3ds-ip> [seconds]
Sends one line, "quit", to the game's live log server. The game sets its exit flag, the main
loop ends, and main returns (log lines "quit received", then "exit: audio job done", ...).
After the send, the tool reads and discards the game's output until the game closes the
connection or the wait (default 10 s) ends. Reading is needed: closing with unread data
makes the game's socket reset.
"""
import socket
import sys
import time


def main():
    ip = sys.argv[1]
    wait = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0
    try:
        sock = socket.create_connection((ip, 17492), timeout=2)
    except OSError as e:
        print("[quitgame] no connection to %s:17492: %s" % (ip, e))
        sys.exit(1)
    sock.sendall(b"quit\n")
    print("[quitgame] sent: quit", flush=True)
    sock.settimeout(1)
    end = time.time() + wait
    closed = False
    while time.time() < end:
        try:
            data = sock.recv(4096)
        except socket.timeout:
            continue
        except OSError as e:
            print("[quitgame] connection lost: %s" % e, flush=True)
            closed = True
            break
        if not data:
            closed = True
            break
    sock.close()
    print("[quitgame] game closed the connection" if closed else "[quitgame] no close within %d s" % wait, flush=True)


if __name__ == "__main__":
    main()
