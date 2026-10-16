"""Stand-in for the Rust crunch64 CLI: `crunch64 compress yay0 IN OUT`, via the crunch64 pip package."""
import sys
from pathlib import Path

import crunch64.yay0

Path(sys.argv[4]).write_bytes(crunch64.yay0.compress(Path(sys.argv[3]).read_bytes()))
