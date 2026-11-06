#!/usr/bin/env python3
"""bench.py: named benchmarks, a run log, a per-window sample index and area queries for comparing profiling numbers between builds.
This is the entry point for every test and profiling run (it replaces the text test catalog).

  python 3ds/tools/bench.py list [--tag T]
  python 3ds/tools/bench.py show <bench>
  python 3ds/tools/bench.py run <bench> --row <row> --why "<text>" [--label L] [--build N] [--commit H] [--speed turbo|normal]
        azahar_pic, azahar_stream: needs --why. Writes the justification line to 3ds/logs/perf/status_<row>.txt,
        then starts the scenario through azt.ps1. azt.ps1 records the run itself (ingest-last), so run does not ingest.
        host_test: runs the command, ingests its output (metric rc = exit code).
        console_prof: prints the push command only. Run `ingest` on the log after the push.
  python 3ds/tools/bench.py ingest <log> --bench <bench> --host azahar|o3ds|n3ds|host [--build N] [--commit H] [--row R] [--label L] [--note N] [--device D]
        Parses the prof windows (prof_by_scene.py) and the log_metrics regexes of the bench. One run per scene.
        Build and commit come from the log's "build N git H" line when not given. Without that line, from
        3ds/logs/perf/pushed_builds.txt (last push at or before the log's write time). Else null with a warning.
  python 3ds/tools/bench.py ingest-last --log <log> --scenario <name> --row <row> --rc <n> [--label L] [--build N] [--commit H] [--shots DIR]
        Called by azt.ps1 after each scenario start that wrote a log. A scenario with a bench records under that bench;
        any other scenario records as bench "adhoc:<scenario>".
  python 3ds/tools/bench.py build-at <commit> <bench> --row R [--why "<text>"] [--label L] [--dry-run]
        Azahar and console_prof benches. Builds <commit> as lane at_<short> (own volumes, build3ds/at_<short>/, the main folder is not changed).
        console_prof: pushes that 3dsx to the O3DS (PM_SRC3DSX) and ingests the live log (dirty false). --dry-run prints the commands only.
        runs the bench on that build, records the run with that commit and dirty false, then deletes the folder.
        The Azahar build copy is put back after the run. --why is optional: a line is written from the bench purpose.
  python 3ds/tools/bench.py import-csv                      (docs/handoff/perf_history.csv rows as runs, tag legacy_import; rows already imported are skipped)
  python 3ds/tools/bench.py where --map <name> [area filters] [--chg]
        Per build (build, commit, date, host): windows, mean fps, ft_ms, nd_ms, lg_ms, gb_ms, delta against the first row.
  python 3ds/tools/bench.py spots [--map <name>] [area filters] [--top N]
        Distinct (map, entry, end x and z rounded to 10 units) spots with run and window counts.
  python 3ds/tools/bench.py compare <bench> <buildA> <buildB> [area filters]   (build = commit prefix or build number)
  python 3ds/tools/bench.py compare <bench> --last 2 [area filters]
  python 3ds/tools/bench.py trend <bench> [<metric>] [area filters]           (no metric: the prof_record.py trend layout, fps and ft)
  python 3ds/tools/bench.py check <run_id>                    (evaluate the gates of one run)
  python 3ds/tools/bench.py rebuild                           (drop and rebuild bench.db from runs.jsonl and bench_defs.json)

Speed: every run record holds speed (turbo | normal | unknown) and frame_limit (percent, or null). azt.ps1 sets them for its start:
  -Speed, else -Turbo in the scenario's extra args (turbo), else normal. Old runs are "unknown" (not guessed). The bench default
  is "speed" in bench_defs.json. Queries take --speed turbo|normal|unknown|any. Default: the bench speed (where and spots: any).
  any prints one block per speed. Turbo and normal rows are never averaged together. A compare of two builds with different
  speed sets prints a WARNING line and no delta.
Area filters (where, spots, compare, trend): --map NAME --mode N --story N --battle N --entry N --x=LO..HI --y=LO..HI --z=LO..HI
  --device TEXT|any (run device substring, 131 or 170; default: the current device of the host, the old .131 runs are excluded).
  --host azahar|o3ds|n3ds|host --chg (include chg1 windows; default excluded). Use the = form for negative ranges (--x=-300..0).
  Position bounds filter on the window end position. Imported history rows have no window positions: a position filter drops them.

dirty (run record, true | false | null for old runs): true when git status shows a changed tracked file other than
3ds/src/map_restore_table.c and 3ds/logs/. Set at ingest time (the tree when the log is read). build-at records false.
Default: a bench with "default": false is listed after the others by list (absent = true).

Hosts: azahar, o3ds (Old 3DS, XL included; the old value o3dsxl is read as o3ds), n3ds, host (host tests).
Files: 3ds/tools/bench_defs.json (benches, tracked, hand edited). 3ds/logs/perf/runs.jsonl (one JSON line per run, append only,
the source of truth, tracked; each run holds its windows with map, area, mode, story, battle, entry, pos0, pos).
3ds/logs/perf/bench.db (SQLite index, untracked, rebuilt after every write: tables bench, build, run, metric, sample).
sample has one row per window (win "f<frame> w<size>"; ts is the run wall time; frame is the window frame number). A run with
a window count and no window records (imported history) gets one row with win "agg" and extra_json agg_windows = count.
Gate: {metric, op, value} or {metric, op, ref: baseline}. The baseline is the earliest run of the bench with that metric.
"""
import argparse, collections, csv, datetime, io, json, os, re, shutil, sqlite3, subprocess, sys, tarfile, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import prof_by_scene as pb  # prof line parser, shared with prof_record.py

REPO = HERE.parent.parent
DEFS = HERE / "bench_defs.json"
SCEN = HERE / "azt_scenarios.json"
PERF = REPO / "3ds" / "logs" / "perf"
RUNS = PERF / "runs.jsonl"
DB = PERF / "bench.db"
PUSHED = PERF / "pushed_builds.txt"
CSV = REPO / "docs" / "handoff" / "perf_history.csv"
SHOTS = REPO / "build3ds" / "azt" / "shots"
OUT = REPO / "build3ds" / "bench"
HOSTS = ("azahar", "o3ds", "n3ds", "host")
BUILD_UNTRACKED = ("assets/us", "ver/us/build")  # build.sh inputs that are not in git (build-at copies them)
HOST_ALIAS = {"o3dsxl": "o3ds"}
DEVICE = {"azahar": "azahar emulator on pc", "host": "host pc (docker)", "o3ds": "o3ds 192.168.1.170"}
# The O3DS XL at .131 was the console until 2026-11-05. Its runs keep this device string (never rewritten).
DEVICE_O3DS_OLD = "o3ds xl 192.168.1.131"
RENAME = {"fps": "fps", "ft": "ft_ms", "nd": "nd_ms", "lg": "lg_ms", "gb": "gb_ms", "dr": "draws", "tri": "tris"}
ORDER = ["fps", "ft_ms", "nd_ms", "lg_ms", "gb_ms", "draws", "tris", "windows"]
MET = ("fps", "ft_ms", "nd_ms", "lg_ms", "gb_ms")
SPEEDS = ("turbo", "normal", "unknown")
SPEED_CHOICES = SPEEDS + ("any",)
AREA = ("mode", "story", "battle", "entry")
SPOT_STEP = 10
OPS = {"==": lambda a, b: a == b, "!=": lambda a, b: a != b, "<": lambda a, b: a < b,
       "<=": lambda a, b: a <= b, ">": lambda a, b: a > b, ">=": lambda a, b: a >= b}
SCENE_KEY = re.compile(r"^(\S+) m(-?\d+) sp(-?\d+) b(-?\d+) e(-?\d+)$")
BUILD_LINE = re.compile(r"\bbuild (\d+)(?:-\S+)? git (\S+)")  # boot line (pm_main.c): "build 2175 git 19348de70 loglevel 3"; lane: "build 2175-UC1 git ..."
BUILD_H = REPO / "3ds" / "include" / "pm_build.h"  # written by build.sh for the last build (PM_BUILD_STR, PM_GIT_HASH)
HDR_BUILD = re.compile(r'#define PM_BUILD_STR "(\d+)')
HDR_GIT = re.compile(r'#define PM_GIT_HASH "(\S+)"')
PUSHED_LINE = re.compile(r"^(\d{4}-\d\d-\d\d \d\d:\d\d)\s.*?\bbuild (\d+)\s+git (\S+)")
SCHEMA = """
CREATE TABLE bench(name TEXT PRIMARY KEY, kind TEXT, purpose TEXT, defs_json TEXT);
CREATE TABLE build(build_no INTEGER, commit_hash TEXT, date TEXT);
CREATE TABLE run(id INTEGER PRIMARY KEY, bench TEXT, kind TEXT, host TEXT, device TEXT, build_no INTEGER, commit_hash TEXT, date TEXT, ts TEXT,
  row TEXT, label TEXT, result TEXT, scene TEXT, log_path TEXT, shot_path TEXT, note TEXT, tags TEXT, x_range TEXT, y_range TEXT, z_range TEXT,
  speed TEXT, frame_limit REAL, dirty INTEGER, wide INTEGER, stereo INTEGER, fps_target INTEGER);
CREATE TABLE metric(run_id INTEGER, win TEXT, key TEXT, value REAL);
CREATE TABLE sample(run_id INTEGER, win TEXT, frame INTEGER, ts TEXT, build_no INTEGER, host TEXT, map TEXT, area TEXT, mode INTEGER,
  story INTEGER, battle INTEGER, entry INTEGER, chg INTEGER, x0 REAL, y0 REAL, z0 REAL, x REAL, y REAL, z REAL,
  fps REAL, ft_ms REAL, nd_ms REAL, lg_ms REAL, gb_ms REAL, extra_json TEXT, speed TEXT, frame_limit REAL,
  wide INTEGER, stereo INTEGER, fps_target INTEGER);
CREATE INDEX run_bench ON run(bench);
CREATE INDEX metric_run ON metric(run_id);
CREATE INDEX sample_run ON sample(run_id);
CREATE INDEX sample_map ON sample(map, entry);
"""


def now_iso():
    return datetime.datetime.now().isoformat(timespec="seconds")


def today():
    return datetime.date.today().isoformat()


def git_head():
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=REPO, text=True).strip()
    except Exception:
        return "?"


def git_dirty():
    """True when a tracked file has changes, except 3ds/src/map_restore_table.c and 3ds/logs/. None when git fails."""
    try:
        out = subprocess.check_output(["git", "status", "--porcelain"], cwd=REPO, text=True)
    except Exception:
        return None
    for ln in out.splitlines():
        if not ln.strip() or ln[:2] in ("??", "!!"):
            continue
        path = ln[3:].split(" -> ")[-1].strip().strip('"')
        if path == "3ds/src/map_restore_table.c" or path.startswith("3ds/logs/"):
            continue
        return True
    return False


def md5_of(p):
    import hashlib
    return hashlib.md5(Path(p).read_bytes()).hexdigest()


def posix(p):
    s = str(p)
    return "/" + s[0].lower() + s[2:].replace("\\", "/") if len(s) > 2 and s[1] == ":" else s.replace("\\", "/")


def norm_host(h):
    return HOST_ALIAS.get(h or "", h or "")


def device_note(host):
    if host in DEVICE:
        return DEVICE[host]
    ip = (load_defs().get("consoles") or {}).get(host)
    return ("%s %s" % (host, ip)) if ip else (host or "")


def device_sql(f, alias):
    """SQL and args for the run device (alias = the run table alias in the query). f["device"]: --device TEXT, a substring
    of the device (131, 170), or "any" (no device filter). Else the current device of the host: with --host, that host's
    device; without --host, an o3ds run counts only on the current O3DS (the old O3DS XL .131 runs are another device)."""
    if f.get("device") == "any":
        return "1=1", []
    if f.get("device"):
        return "%s.device LIKE ?" % alias, ["%" + f["device"] + "%"]
    if f.get("host"):
        return "%s.device = ?" % alias, [device_note(f["host"])]
    return "(%s.host != 'o3ds' OR %s.device = ?)" % (alias, alias), [device_note("o3ds")]


def has_area(f):
    """True when f holds a filter other than the device (the query then reads the sample rows, see filtered_runs)."""
    return any(k not in ("device", "disp") for k in f)


def load_defs():
    return json.loads(DEFS.read_text(encoding="utf-8"))


def bench_def(name):
    for b in load_defs()["benches"]:
        if b["name"] == name:
            return b
    return None


def scenario(name):
    for s in json.loads(SCEN.read_text(encoding="utf-8"))["scenarios"]:
        if s["name"] == name:
            return s
    return None


def read_runs():
    if not RUNS.exists():
        return []
    out = []
    for x in RUNS.read_text(encoding="utf-8").splitlines():
        if not x.strip():
            continue
        r = json.loads(x)
        r["host"] = norm_host(r.get("host"))
        r.setdefault("device", "")
        r.setdefault("speed", "unknown")
        r.setdefault("frame_limit", None)
        out.append(r)
    return out


def append_runs(recs):
    PERF.mkdir(parents=True, exist_ok=True)
    n = len(read_runs())
    with RUNS.open("a", encoding="utf-8", newline="\n") as f:
        for r in recs:
            n += 1
            r["run_id"] = n
            r["host"] = norm_host(r.get("host"))
            r["device"] = r.get("device") or device_note(r["host"])
            f.write(json.dumps(r, separators=(",", ":")) + "\n")
    return rebuild_db()


def scene_fields(scene):
    """Map and mode/story/battle/entry of a run's scene key (`<map> m<mode> sp<story> b<battle> e<entry>`). Other keys: map untagged."""
    m = SCENE_KEY.match(scene or "")
    if not m:
        return {"map": "untagged", "area": None, "mode": None, "story": None, "battle": None, "entry": None}
    return {"map": m.group(1), "area": None, "mode": int(m.group(2)), "story": int(m.group(3)), "battle": int(m.group(4)), "entry": int(m.group(5))}


def run_disp_tuple(r):
    d = r.get("disp") or {}
    return (d.get("wide"), d.get("stereo"), d.get("fps_target"))


def sample_rows(r):
    """Sample rows of one run record (the sample table, see the module docstring)."""
    rid, ts, bn, host = r["run_id"], r.get("ts"), r.get("build_no"), r.get("host")
    base = scene_fields(r.get("scene"))
    rows = []
    dsp = []
    for w in r.get("windows", []):
        f = {k: w.get(k) for k in ("map", "area", "mode", "story", "battle", "entry")}
        if w.get("map") is None:
            f = base
        p0 = w.get("pos0") or [None, None, None]
        p = w.get("pos") or [None, None, None]
        m = w.get("m", {})
        rows.append((rid, w["win"], w.get("frame"), ts, bn, host, f["map"], f["area"], f["mode"], f["story"], f["battle"], f["entry"],
                     1 if w.get("chg") else 0, p0[0], p0[1], p0[2], p[0], p[1], p[2],
                     m.get("fps"), m.get("ft_ms"), m.get("nd_ms"), m.get("lg_ms"), m.get("gb_ms"), json.dumps({"wsize": w.get("wsize")})))
        dsp.append(tuple(w["disp"]) if w.get("disp") else run_disp_tuple(r))
    nw = int((r.get("run") or {}).get("windows") or 0)
    if not r.get("windows") and nw > 0:
        m = r.get("run", {})
        rows.append((rid, "agg", None, ts, bn, host, base["map"], base["area"], base["mode"], base["story"], base["battle"], base["entry"], 0,
                     None, None, None, None, None, None,
                     m.get("fps"), m.get("ft_ms"), m.get("nd_ms"), m.get("lg_ms"), m.get("gb_ms"), json.dumps({"agg_windows": nw})))
        dsp.append(run_disp_tuple(r))
    sp, fl = r.get("speed") or "unknown", r.get("frame_limit")
    return [x + (sp, fl) + d for x, d in zip(rows, dsp)]


def rebuild_db():
    defs = load_defs()
    PERF.mkdir(parents=True, exist_ok=True)
    if DB.exists():
        DB.unlink()
    con = sqlite3.connect(DB)
    con.executescript(SCHEMA)
    for b in defs["benches"]:
        con.execute("INSERT INTO bench VALUES (?,?,?,?)", (b["name"], b["kind"], b["purpose"], json.dumps(b, sort_keys=True)))
    runs = read_runs()
    nm = ns = 0
    for r in runs:
        con.execute("INSERT INTO run VALUES (%s)" % ",".join("?" * 26),
                    (r["run_id"], r["bench"], r.get("kind"), r.get("host"), r.get("device", ""), r.get("build_no"), r.get("commit"), r.get("date"),
                     r.get("ts"), r.get("row"), r.get("label"), r.get("result"), r.get("scene", ""), r.get("log_path"), r.get("shot_path"),
                     r.get("note"), ",".join(r.get("tags", [])), r.get("x_range", ""), r.get("y_range", ""), r.get("z_range", ""),
                     r.get("speed") or "unknown", r.get("frame_limit"), r.get("dirty")) + run_disp_tuple(r))
        rows = [(r["run_id"], "run", k, float(v)) for k, v in r.get("run", {}).items()]
        for w in r.get("windows", []):
            rows += [(r["run_id"], w["win"], k, float(v)) for k, v in w["m"].items()]
        con.executemany("INSERT INTO metric VALUES (?,?,?,?)", rows)
        nm += len(rows)
        srows = sample_rows(r)
        con.executemany("INSERT INTO sample VALUES (%s)" % ",".join("?" * 30), srows)
        ns += len(srows)
    con.execute("INSERT INTO build SELECT MAX(build_no), commit_hash, MIN(date) FROM run GROUP BY commit_hash")
    con.commit()
    con.close()
    return len(defs["benches"]), len(runs), nm, ns


def open_db():
    if not DB.exists():
        rebuild_db()
    con = sqlite3.connect(DB)
    con.row_factory = sqlite3.Row
    if "fps_target" not in {x[1] for x in con.execute("PRAGMA table_info(sample)")}:
        con.close()
        rebuild_db()  # a DB from before the disp columns
        con = sqlite3.connect(DB)
        con.row_factory = sqlite3.Row
    return con


def mean_vals(vals):
    vals = [v for v in vals if v is not None]
    return sum(vals) / len(vals) if vals else None


def wmean(rows, k):
    """Mean of column k over sample rows. Each row counts n windows (an agg row counts its windows)."""
    num = den = 0.0
    for r in rows:
        v = r.get(k)
        if v is None:
            continue
        num += v * r["n"]
        den += r["n"]
    return num / den if den else None


def bench_runs(name, f=None):
    """Run-level records of one bench, in run order, filtered by the bench scene_match (when set)."""
    bd = bench_def(name) or {}
    sm = bd.get("scene_match")
    con = open_db()
    dsql, dargs = device_sql(f or {}, "r")
    rs = con.execute("SELECT r.id, r.date, r.commit_hash, r.build_no, r.scene, r.host, r.result, r.row, r.label, r.x_range, r.z_range, r.note, "
                     "r.speed, r.frame_limit, r.device, r.wide, r.stereo, r.fps_target, "
                     "m.key, m.value FROM run r LEFT JOIN metric m ON m.run_id = r.id AND m.win = 'run' WHERE r.bench = ? AND " + dsql
                     + " ORDER BY r.id, m.key", (name, *dargs)).fetchall()
    con.close()
    out = {}
    for x in rs:
        if sm and sm not in (x["scene"] or ""):
            continue
        d = out.setdefault(x["id"], {"id": x["id"], "date": x["date"], "commit": x["commit_hash"], "build_no": x["build_no"], "scene": x["scene"] or "",
                                     "host": x["host"], "result": x["result"], "row": x["row"], "label": x["label"] or "", "x_range": x["x_range"] or "",
                                     "z_range": x["z_range"] or "", "note": x["note"] or "", "m": {},
                                     "speed": x["speed"] or "unknown", "frame_limit": x["frame_limit"], "device": x["device"] or "",
                                     "disp": dmode(x["wide"], x["stereo"], x["fps_target"]), "rid": x["id"]})
        if x["key"] is not None:
            d["m"][x["key"]] = x["value"]
    return list(out.values())


def select_samples(con, f, bench=None):
    """Sample rows that pass the area filter f (see flt_of). Each row gets n, its window count."""
    wh, args = [], []
    if bench:
        wh.append("r.bench = ?")
        args.append(bench)
    if "map" in f:
        wh.append("s.map = ?")
        args.append(f["map"])
    for k in AREA:
        if k in f:
            wh.append("s.%s = ?" % k)
            args.append(f[k])
    for ax in ("x", "y", "z"):
        if ax in f:
            wh.append("s.%s BETWEEN ? AND ?" % ax)
            args += list(f[ax])
    if "host" in f:
        wh.append("s.host = ?")
        args.append(f["host"])
    dsql, dargs = device_sql(f, "r")
    wh.append(dsql)
    args += dargs
    if not f.get("chg"):
        wh.append("s.chg = 0")
    sql = ("SELECT s.run_id, s.win, s.ts, s.build_no, s.host, s.map, s.area, s.mode, s.story, s.battle, s.entry, s.chg, "
           "s.x0, s.y0, s.z0, s.x, s.y, s.z, s.fps, s.ft_ms, s.nd_ms, s.lg_ms, s.gb_ms, s.extra_json, s.speed, s.frame_limit, "
           "s.wide, s.stereo, s.fps_target, r.bench, r.date, r.commit_hash, r.scene FROM sample s JOIN run r ON r.id = s.run_id")
    if wh:
        sql += " WHERE " + " AND ".join(wh)
    out = []
    for x in con.execute(sql, args).fetchall():
        d = dict(x)
        d["n"] = json.loads(d.pop("extra_json") or "{}").get("agg_windows", 1)
        d["disp"] = dmode(d.pop("wide"), d.pop("stereo"), d.pop("fps_target"))
        d["rid"] = d["run_id"]
        out.append(d)
    return out


def filtered_runs(name, f):
    """Run records of one bench, built from the sample rows that pass f. Same shape as bench_runs()."""
    bd = bench_def(name) or {}
    sm = bd.get("scene_match")
    con = open_db()
    rows = select_samples(con, f, bench=name)
    info = {x["id"]: dict(x) for x in con.execute("SELECT id, date, commit_hash, build_no, scene, host, result, row, label, x_range, z_range, note, "
                                                  "speed, frame_limit, wide, stereo, fps_target "
                                                  "FROM run WHERE bench = ?", (name,))}
    con.close()
    by = collections.OrderedDict()
    for r in rows:
        by.setdefault(r["run_id"], []).append(r)
    out = []
    for rid in sorted(by):
        i = info.get(rid)
        if not i or (sm and sm not in (i["scene"] or "")):
            continue
        m = {k: v for k, v in ((k, wmean(by[rid], k)) for k in MET) if v is not None}
        m["windows"] = float(sum(r["n"] for r in by[rid]))
        out.append({"id": rid, "date": i["date"], "commit": i["commit_hash"], "build_no": i["build_no"], "scene": i["scene"] or "",
                    "host": i["host"], "result": i["result"], "row": i["row"], "label": i["label"] or "", "x_range": i["x_range"] or "",
                    "z_range": i["z_range"] or "", "note": i["note"] or "", "m": m,
                    "speed": i["speed"] or "unknown", "frame_limit": i["frame_limit"],
                    "disp": dmode(i["wide"], i["stereo"], i["fps_target"]), "rid": rid})
    return out


def baseline(runs, metric):
    for r in runs:
        if metric in r["m"]:
            return r["m"][metric]
    return None


def gate_lines(bd, runs, vals):
    """vals: metric -> value. Returns (ok, lines) for the gates of the bench."""
    ok_all, lines = True, []
    for g in bd.get("gates", []):
        v = vals.get(g["metric"])
        ref = "ref" in g
        want = baseline(runs, g["metric"]) if ref else g["value"]
        if v is None or want is None:
            ok_all = False
            lines.append("MISSING %s (no value or no baseline)" % g["metric"])
            continue
        ok = OPS[g["op"]](v, want)
        ok_all = ok_all and ok
        lines.append("%s %s %.4g %s %.4g%s" % ("PASS" if ok else "GATE FAIL", g["metric"], v, g["op"], want, " (baseline)" if ref else ""))
    return ok_all, lines


def log_metric_values(bd, text):
    out = {}
    for lm in (bd or {}).get("log_metrics", []):
        ms = re.findall(lm["regex"], text)
        if ms:
            v = ms[-1] if lm.get("pick", "last") == "last" else ms[0]
            if isinstance(v, tuple):
                v = v[0]
            out[lm["name"]] = float(v)
    return out


def win_metrics(w):
    m = {}
    for k, v in w.get("fields", {}).items():
        m[RENAME.get(k, k)] = v
    for k in ("fps", "ft", "nd", "lg", "gb"):
        if k in w:
            m[RENAME[k]] = w[k]
    return m


def window_rec(w):
    """One window of a run record: the scene line (map, area, mode, story, battle, entry, pos0, pos) and the metrics."""
    return {"win": "f%d w%d" % (w["frame"], w.get("wsize", 0)), "frame": w["frame"], "wsize": w.get("wsize", 0), "chg": bool(w["chg"]),
            "map": w["map"], "area": w["area"], "mode": w["mode"], "story": w["story"], "battle": w["battle"], "entry": w["entry"],
            "pos0": list(w["pos0"]) if w["pos0"] else None, "pos": list(w["pos"]) if w["pos"] else None, "m": win_metrics(w),
            "disp": list(w["disp"]) if w.get("disp") else None}


def pos_ranges(ws):
    ps = [q for w in ws for q in (w.get("pos0"), w.get("pos")) if q]
    if not ps:
        return "", "", ""

    def rng(i):
        return "%d..%d" % (min(p[i] for p in ps), max(p[i] for p in ps))
    return rng(0), rng(1), rng(2)


def make_run(bench, kind, host, build, commit, row, label, result, scene, log, shot, note, tags, run, windows, x="", z="", y="", device="", date=None,
             speed="unknown", frame_limit=None, dirty=None):
    host = norm_host(host)
    return {"bench": bench, "kind": kind, "host": host, "device": device or device_note(host), "build_no": build,
            "commit": commit, "date": date or today(), "ts": now_iso(), "row": row, "label": label, "result": result, "scene": scene,
            "log_path": log, "shot_path": shot, "note": note, "tags": tags, "x_range": x, "y_range": y, "z_range": z,
            "speed": speed, "frame_limit": frame_limit, "dirty": dirty, "run": run, "windows": windows}


DISP = re.compile(r"prof f(\d+) w\d+ at \S.* disp w([01]) s([01]) f(\d+)")  # scene line tail: " disp w<0|1> s<0|1> f<fps target>"


def parse_disp(text):
    """Display mode of each prof window of a log: {frame: (wide, stereo, fps_target)}. Old logs (no disp part) have no entry."""
    out = {}
    for ln in text.splitlines():
        m = DISP.search(ln)
        if m:
            out[int(m.group(1))] = (int(m.group(2)), int(m.group(3)), int(m.group(4)))
    return out


def dmode(wide, stereo, fps):
    """Display mode label: w1s0f30 (16:9 wide, stereo off, fps target 30). "-" is the null mode (old logs, no disp part)."""
    if wide is None or stereo is None or fps is None:
        return "-"
    return "w%ds%df%d" % (wide, stereo, fps)


def disp_arg(s):
    if not re.fullmatch(r"any|-|w[01]s[01]f\d+", s):
        raise argparse.ArgumentTypeError("--disp takes any, -, or w<0|1>s<0|1>f<fps>, e.g. w1s0f30 (got %s)" % s)
    return s


def disp_select(rows, f):
    """Rows of one display mode, never averaged across modes. --disp MODE picks it; any keeps all; default: the mode of the
    newest row (highest run id). Prints one line with the count of rows of other modes that the query drops."""
    want = f.get("disp")
    if want == "any" or not rows:
        return rows
    if not want:
        want = max(rows, key=lambda r: r["rid"])["disp"]
    kept = [r for r in rows if r["disp"] == want]
    if len(kept) < len(rows):
        print("disp %s: %d row(s) of other display modes dropped (--disp any keeps them)" % (want, len(rows) - len(kept)))
    return kept


def disp_label(rows):
    modes = sorted({r["disp"] for r in rows})
    return modes[0] if len(modes) == 1 else "mixed"


def log_build(text):
    m = BUILD_LINE.search(text)
    return (int(m.group(1)), m.group(2)) if m else (None, None)


def pushed_build(path):
    """Build and hash of the last push line at or before the log's write time (pushed_builds.txt)."""
    if not PUSHED.exists():
        return None, None
    t = datetime.datetime.fromtimestamp(Path(path).stat().st_mtime)
    best = (None, None)
    for ln in PUSHED.read_text(encoding="utf-8", errors="replace").splitlines():
        m = PUSHED_LINE.match(ln)
        if m and datetime.datetime.strptime(m.group(1), "%Y-%m-%d %H:%M") <= t:
            best = (int(m.group(2)), m.group(3))
    return best


def azahar_3dsx():
    """The 3dsx azt.ps1 ran: build3ds/lane-<PM_LANE>/pm_3ds.3dsx (azt.ps1 sets PM_LANE for a lane), else build3ds/pm_3ds.3dsx."""
    lane = os.environ.get("PM_LANE")
    return (REPO / "build3ds" / ("lane-" + lane) if lane else REPO / "build3ds") / "pm_3ds.3dsx"


def header_build():
    """Build and hash from 3ds/include/pm_build.h, valid for the 3dsx that ran only when the header is not newer than that 3dsx
    (build.sh writes the header first and the 3dsx last). (None, None) when the header or the 3dsx is missing, or the header is newer."""
    dx = azahar_3dsx()
    try:
        h = BUILD_H.read_text(encoding="utf-8")
    except OSError:
        h = ""
    bm, gm = HDR_BUILD.search(h), HDR_GIT.search(h)
    if not bm or not gm or not dx.exists():
        print("WARNING: no build header for %s (%s missing or 3dsx missing). build left null." % (dx, BUILD_H))
        return None, None
    if BUILD_H.stat().st_mtime > dx.stat().st_mtime:
        print("WARNING: %s is newer than %s (a later build that failed or is not the 3dsx that ran). build left null." % (BUILD_H, dx))
        return None, None
    return int(bm.group(1)), gm.group(1)


def resolve_build(path, text, build, commit, host="host"):
    """Build number and commit of a log. azahar: the arguments, the log's build line, then pm_build.h of the 3dsx that ran
    (never pushed_builds.txt, it lists console pushes only). Other hosts: the arguments, the log's build line, then
    pushed_builds.txt (last push at or before the log's write time), then git HEAD."""
    lb, lc = log_build(text)
    if host == "azahar":
        hb, hc = (None, None) if (lb is not None or build is not None) else header_build()
        b = build if build is not None else (lb if lb is not None else hb)
        c = commit or lc or hc or git_head()
        if b is None:
            print("WARNING: no build line in %s. build left null (pushed_builds.txt is not used for azahar)." % path)
        return b, c
    pbld, pc = (None, None) if (lb is not None or build is not None) else pushed_build(path)
    b = build if build is not None else (lb if lb is not None else pbld)
    c = commit or lc or pc or git_head()
    if b is None:
        print("WARNING: no build number in %s and none in %s. build left null." % (path, PUSHED.name))
    return b, c


def ingest_file(path, bench, host, build, commit, row, label, result="imported", note="", shot=None, tags=None, device="", kind=None,
                speed="unknown", frame_limit=None, dirty=None):
    p = Path(path)
    dirty = git_dirty() if dirty is None else dirty
    text = p.read_text(encoding="utf-8", errors="replace")
    wins = pb.parse_windows(str(p))
    dmap = parse_disp(text)
    for w in wins:
        w["disp"] = dmap.get(w["frame"])
    bd = bench_def(bench) or {}
    lm = log_metric_values(bd, text)
    kind = kind or bd.get("kind", "ingest")
    rel = posix(p.resolve().relative_to(REPO)) if REPO in p.resolve().parents else str(path)
    by = collections.OrderedDict()
    for w in wins:
        if "fps" in w:
            by.setdefault(w["key"], []).append(w)
    dropped = sum(1 for w in wins if "fps" in w and w["chg"])
    if not by:
        by = {"": []}
    recs = []
    for scene, ws in by.items():
        good = [w for w in ws if not w["chg"]]
        gm = [win_metrics(w) for w in good]
        keys = sorted({k for m in gm for k in m})
        run = {k: mean_vals([m[k] for m in gm if k in m]) for k in keys}
        run = {k: v for k, v in run.items() if v is not None}
        run["windows"] = float(len(good))
        run.update(lm)
        x, y, z = pos_ranges(good)
        rec = make_run(bench, kind, host, build, commit, row, label, result, scene, rel, shot, note, tags or [], run,
                       [window_rec(w) for w in ws], x, z, y=y, device=device, speed=speed, frame_limit=frame_limit, dirty=dirty)
        dl = collections.Counter(tuple(w["disp"]) for w in (good or ws) if w.get("disp"))
        if dl:  # the run's display mode: the most common of its windows (old logs: none)
            wide, stereo, fps = dl.most_common(1)[0][0]
            rec["disp"] = {"wide": wide, "stereo": stereo, "fps_target": fps}
        recs.append(rec)
    n = append_runs(recs)
    print("ingested %s: %d scene run(s), scene-change windows dropped %d, run ids up to %d, build %s, commit %s" % (
        path, len(recs), dropped, n[1], build if build is not None else "-", commit))
    for r in recs:
        print("  scene %-30s windows %3d fps %s ft %s" % (r["scene"] or "-", int(r["run"]["windows"]),
              "%.1f" % r["run"]["fps"] if "fps" in r["run"] else "-", "%.1f" % r["run"]["ft_ms"] if "ft_ms" in r["run"] else "-"))
    return 0


def cmd_list(a):
    defs = load_defs()
    con = open_db()
    n = 0
    print("%-16s %-14s %5s %5s %-4s %s" % ("bench", "kind", "cap", "runs", "dflt", "purpose"))
    for b in sorted(defs["benches"], key=lambda x: x.get("default", True) is False):
        if a.tag and a.tag not in b.get("tags", []):
            continue
        runs = con.execute("SELECT COUNT(*) FROM run WHERE bench = ?", (b["name"],)).fetchone()[0]
        print("%-16s %-14s %5s %5d %-4s %s" % (b["name"], b["kind"], b["cap"], runs, "yes" if b.get("default", True) else "no", b["purpose"]))
        n += 1
    con.close()
    print("%d bench(es)" % n)
    return 0


def push_cmd(bd):
    ip = load_defs()["consoles"][bd["host"]]
    parts = [load_defs()["push_base"], bd["recipe"]]
    return "PM_ROW=PUSH sh %s/3ds/push_3ds.sh --no-build --ip %s %s" % (posix(REPO), ip, " ".join(p for p in parts if p))


def cmd_show(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s. See: bench.py list" % a.bench)
        return 4
    print("%s (%s)" % (bd["name"], bd["kind"]))
    print("purpose: %s" % bd["purpose"])
    print("cap: %s s   until: %s" % (bd["cap"], bd.get("until", "-")))
    print("speed: %s (bench default; run --speed turbo|normal)" % bd.get("speed", "normal"))
    if bd["kind"].startswith("azahar"):
        s = scenario(bd["scenario"])
        tryf = " -Try" if s and s["status"] == "unverified" else ""
        print("recipe: scenario %s (status %s)" % (bd["scenario"], s["status"] if s else "MISSING"))
        print("run: python 3ds/tools/bench.py run %s --row <row> --why \"<text>\"" % bd["name"])
        print("     (azt.ps1 %s -Row <row>%s)" % (bd["scenario"], tryf))
        print("build-at: python 3ds/tools/bench.py build-at <commit> %s --row <row>" % bd["name"])
    elif bd["kind"] == "host_test":
        print("command: %s" % bd["command"])
    else:
        print("recipe: %s" % bd["recipe"])
        print("push: %s" % push_cmd(bd))
    for g in bd.get("gates", []):
        print("gate: %s %s %s" % (g["metric"], g["op"], "baseline" if "ref" in g else g["value"]))
    for m in bd.get("log_metrics", []):
        print("log metric: %s  /%s/" % (m["name"], m["regex"]))
    runs = bench_runs(bd["name"], {"device": "any"})
    counts = collections.Counter((r.get("device") or "", r.get("host")) for r in runs)
    for (dev, h), n in sorted(counts.items()):
        print("device: %s, %d runs%s" % (dev or "(none)", n, "" if dev == device_note(h) else " (other device: not in default queries, --device picks it)"))
    print("last runs (%d total, scene filter %s):" % (len(runs), bd.get("scene_match") or "none"))
    print("  %-4s %-10s %-10s %-8s %-26s %-8s %6s %6s %s" % ("id", "date", "commit", "speed", "scene", "result", "fps", "ft", "label"))
    for r in runs[-5:]:
        print("  %-4d %-10s %-10s %-8s %-26s %-8s %6s %6s %s" % (r["id"], r["date"], r["commit"] or "-", r["speed"], r["scene"][:26] or "-", (r["result"] or "-")[:8],
              "%.1f" % r["m"]["fps"] if "fps" in r["m"] else "-", "%.1f" % r["m"]["ft_ms"] if "ft_ms" in r["m"] else "-", r["label"][:60]))
    return 0


def cmd_run(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s. See: bench.py list" % a.bench)
        return 4
    kind = bd["kind"]
    env = dict(os.environ, PM_ROW=a.row)
    if kind.startswith("azahar"):
        if not a.why:
            print("run needs --why \"<text>\": the justification line is written before the start. Nothing was run.")
            return 5
        s = scenario(bd["scenario"])
        if not s:
            print("scenario %s is not in azt_scenarios.json. Nothing was run." % bd["scenario"])
            return 5
        speed = a.speed or bd.get("speed", "normal")
        measured = list(dict.fromkeys([m["name"] for m in bd.get("log_metrics", [])] + [g["metric"] for g in bd.get("gates", [])])) or ["picture"]
        PERF.mkdir(parents=True, exist_ok=True)
        with (PERF / ("status_%s.txt" % a.row)).open("a", encoding="utf-8", newline="\n") as f:
            f.write("%s %s scenario %s: measures %s; path azt.ps1 -> run_azahar.ps1; speed %s; why %s\n" % (now_iso(), bd["name"], s["name"], ", ".join(measured), speed, a.why))
        for k in ("PM_BENCH_LABEL", "PM_BENCH_BUILD", "PM_BENCH_COMMIT"):
            env.pop(k, None)
        env["PM_BENCH_LABEL"] = a.label or bd["name"]
        if a.build is not None:
            env["PM_BENCH_BUILD"] = str(a.build)
        if a.commit:
            env["PM_BENCH_COMMIT"] = a.commit
        cmd = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(REPO / "3ds" / "tools" / "azt.ps1"), s["name"], "-Row", a.row,
               "-Speed", speed]
        if s["status"] == "unverified":
            cmd.append("-Try")
        rc = subprocess.run(cmd, cwd=REPO, env=env).returncode
        print("bench %s: azt.ps1 exit %d. azt.ps1 wrote the run record (ingest-last) to 3ds/logs/perf/runs.jsonl." % (bd["name"], rc))
        return rc
    if kind == "host_test":
        OUT.mkdir(parents=True, exist_ok=True)
        try:
            p = subprocess.run(bd["command"].split(), cwd=REPO, env=env, capture_output=True, text=True, timeout=bd["cap"])
            out, rc = p.stdout + p.stderr, p.returncode
        except subprocess.TimeoutExpired:
            out, rc = "timeout after %s s" % bd["cap"], 6
        log = OUT / ("%s_%s.txt" % (bd["name"], datetime.datetime.now().strftime("%Y%m%d%H%M%S")))
        log.write_text(out, encoding="utf-8")
        print(out[-2000:])
        text_metrics = log_metric_values(bd, out)
        text_metrics["rc"] = float(rc)
        commit = a.commit or git_head()
        recs = [make_run(bd["name"], kind, "host", a.build, commit, a.row, a.label or bd["name"], "pass" if rc == 0 else "fail:%d" % rc,
                         "", posix(log.relative_to(REPO)), None, "", [], text_metrics, [], speed="normal", dirty=git_dirty())]
        n = append_runs(recs)
        print("run %d: %s rc %d, %s" % (n[1], bd["name"], rc, "PASS" if rc == 0 else "FAIL"))
        return rc
    print("console_prof: push, then ingest the live log. Run this in Git Bash:")
    print(push_cmd(bd))
    ip = load_defs()["consoles"][bd["host"]]
    print("then: python 3ds/tools/bench.py ingest build3ds/hw_live_log_%s.txt --bench %s --host %s --row %s --label \"%s\""
          % (ip.split(".")[-1], bd["name"], bd["host"], a.row, a.label or bd["name"]))
    return 0


def app_3dsx():
    """The 3dsx that Azahar loads (build.sh copies it here)."""
    return Path(os.environ.get("APPDATA", "")) / "Azahar" / "sdmc" / "3ds" / "PaperMario" / "pm_3ds.3dsx"


def build_at_numbers(commit):
    """Build number build.sh writes for commit: V0 + (commit count - C0), V0 and C0 from 3ds/build_base.txt."""
    v0, c0 = (int(x) for x in (REPO / "3ds" / "build_base.txt").read_text(encoding="utf-8").split()[:2])
    return v0 + int(subprocess.check_output(["git", "rev-list", "--count", commit], cwd=REPO, text=True).strip()) - c0


def push_switches(bd):
    """The bench switches of a console bench (push_base, then the recipe), as argv words."""
    return [w for w in (load_defs().get("push_base", "") + " " + (bd.get("recipe") or "")).split() if w]


def scratch_build(commit, gitdir, scratch, lane, row):
    """Archive commit into the scratch folder and build it as lane `lane`: own docker volumes and output
    (scratch/build3ds/lane-<lane>/), never the shared build volumes. Returns (build.sh rc, build number or None)."""
    shutil.rmtree(scratch, ignore_errors=True)
    scratch.mkdir(parents=True)
    arc = subprocess.run(["git", "archive", commit], cwd=REPO, capture_output=True, check=True).stdout
    tarfile.open(fileobj=io.BytesIO(arc)).extractall(scratch)
    gen = scratch / "3ds" / "src" / "map_restore_table.c"
    shutil.copy2(REPO / "3ds" / "src" / "map_restore_table.c", gen)
    subprocess.run([sys.executable, str(scratch / "3ds" / "tools" / "gen_map_restore.py")], cwd=scratch, check=True,
                   stdout=subprocess.DEVNULL)
    # The build inputs that git does not track (ROM-derived assets) come from the main folder. Read only.
    for rel in BUILD_UNTRACKED:
        if not (scratch / rel).exists() and (REPO / rel).exists():
            shutil.copytree(REPO / rel, scratch / rel)
    env = dict(os.environ, GIT_DIR=gitdir, PM_BUILD_REV=commit, PM_ROW=row, PM_LANE=lane)
    rc = subprocess.run([shutil.which("sh") or "sh", "3ds/build.sh"], cwd=scratch, env=env).returncode
    if rc != 0:
        return rc, None
    hdr = (scratch / "3ds" / "include" / "pm_build.h").read_text(encoding="utf-8")
    bstr = re.search(r'PM_BUILD_STR "(\d+)', hdr)
    return 0, (int(bstr.group(1)) if bstr else None)


def build_at_dry(bd, a, commit, short, lane, gitdir, scratch, src3dsx):
    """Prints the build, push and ingest commands of build-at for a console bench. Runs nothing."""
    ip = load_defs()["consoles"]["o3ds"]
    print("build-at %s %s --dry-run: nothing is built, pushed or recorded" % (short, bd["name"]))
    print("build:  cd %s && PM_LANE=%s PM_BUILD_REV=%s GIT_DIR=%s PM_ROW=%s sh 3ds/build.sh" % (
        posix(scratch), lane, commit, posix(gitdir), a.row))
    print("push:   PM_ROW=%s PM_SRC3DSX=%s sh 3ds/push_3ds.sh --no-build --ip %s %s" % (
        a.row, posix(src3dsx), ip, " ".join(push_switches(bd))))
    print("ingest: PM_BENCH_DIRTY=0 python 3ds/tools/bench.py ingest build3ds/hw_live_log_%s.txt --bench %s --host o3ds --build %d "
          "--commit %s --row %s --label \"build-at %s\" --device \"%s\" --speed normal"
          % (ip.split(".")[-1], bd["name"], build_at_numbers(commit), commit, a.row, short, DEVICE["o3ds"]))
    return 0


def build_at_console(bd, a, commit, short, gitdir, scratch, src3dsx, lane):
    """Console build-at: build the commit as a lane, push its 3dsx to the O3DS, ingest the live log with the scratch build."""
    ip = load_defs()["consoles"]["o3ds"]
    try:
        rc, build_no = scratch_build(commit, gitdir, scratch, lane, a.row)
        if rc != 0:
            print("build-at %s: build.sh exit %d. Nothing was pushed." % (short, rc))
            return rc
        if not src3dsx.exists():
            print("build-at %s: no %s after the build. Nothing was pushed." % (short, posix(src3dsx)))
            return 5
        hdr = (scratch / "3ds" / "include" / "pm_build.h").read_text(encoding="utf-8")
        sent_b = re.search(r'PM_BUILD_STR "([^"]+)"', hdr).group(1)  # full: lane suffix included, as the boot line prints it
        sent_h = re.search(r'PM_GIT_HASH "([^"]+)"', hdr).group(1)
        env = dict(os.environ, PM_ROW=a.row, PM_SRC3DSX=str(src3dsx), PM_SENT_BUILD=sent_b, PM_SENT_GIT=sent_h)
        t0 = time.time()
        print("build-at %s: build %s pushed to %s" % (short, build_no, ip))
        prc = subprocess.run([shutil.which("sh") or "sh", "3ds/push_3ds.sh", "--no-build", "--ip", ip] + push_switches(bd),
                             cwd=REPO, env=env).returncode
        log = REPO / "build3ds" / ("hw_live_log_%s.txt" % ip.split(".")[-1])
        lb, lc = log_build(log.read_text(encoding="utf-8", errors="replace")) if log.exists() else (None, None)
        if log.exists() and log.stat().st_mtime >= t0 and lb is not None and (lb != build_no or not same_commit(commit, lc)):
            print("build-at %s: STALE BINARY: log build %s git %s, sent %s git %s. Nothing ingested." % (short, lb, lc, build_no, sent_h))
        elif log.exists() and log.stat().st_mtime >= t0:
            ingest_file(log, bd["name"], "o3ds", build_no, commit, a.row, a.label or ("build-at " + short),
                        "log_pass" if prc == 0 else "fail:%d" % prc, "", None, [], device=DEVICE["o3ds"], kind=bd["kind"],
                        speed="normal", dirty=False)
        else:
            print("build-at %s: no live log from this push at %s. Nothing ingested." % (short, posix(log)))
        return prc
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def cmd_build_at(a):
    bd = bench_def(a.bench)
    console = bool(bd) and bd["kind"] == "console_prof"
    if not bd or not (bd["kind"].startswith("azahar") or console):
        print("build-at runs an Azahar bench (azahar_pic, azahar_stream) or a console_prof bench. %s is not one. Nothing was built." % a.bench)
        return 5
    r = subprocess.run(["git", "rev-parse", "--verify", "--quiet", a.commit + "^{commit}"], cwd=REPO, capture_output=True, text=True)
    if r.returncode != 0:
        print("no commit %s. Nothing was built." % a.commit)
        return 5
    commit = r.stdout.strip()
    short = commit[:9]
    lane = "at_" + short  # own docker volumes and output folder, never the shared build volumes
    gitdir = subprocess.check_output(["git", "rev-parse", "--absolute-git-dir"], cwd=REPO, text=True).strip()
    scratch = REPO / "build3ds" / lane
    src3dsx = scratch / "build3ds" / ("lane-" + lane) / "pm_3ds.3dsx"
    if a.dry_run:
        if not console:
            print("--dry-run prints the console commands only (console_prof benches). Nothing was run.")
            return 5
        return build_at_dry(bd, a, commit, short, lane, gitdir, scratch, src3dsx)
    if console:
        return build_at_console(bd, a, commit, short, gitdir, scratch, src3dsx, lane)
    main_map = REPO / "3ds" / "src" / "map_restore_table.c"
    md5_before = md5_of(main_map)
    dst = app_3dsx()
    prev = scratch.parent / ("at_%s_prev.3dsx" % short)
    had_prev = dst.exists()
    if had_prev:
        shutil.copy2(dst, prev)
    rc = 0
    try:
        rc, build_no = scratch_build(commit, gitdir, scratch, lane, a.row)
        if rc != 0:
            print("build-at %s: build.sh exit %d. Nothing was run." % (short, rc))
            return rc
        # The lane build does not copy its 3dsx to %APPDATA% (build.sh does that for the main build only): copy it for this run.
        shutil.copy2(src3dsx, dst)
        why = a.why or ("build-at %s of %s: the committed code at %s, no working tree change, picture on an older build"
                        % (short, a.bench, short))
        os.environ["PM_BENCH_DIRTY"] = "0"
        try:
            rc = cmd_run(argparse.Namespace(bench=a.bench, row=a.row, why=why, label=a.label or ("build-at " + short),
                                            build=build_no, commit=commit, speed=None))
        finally:
            os.environ.pop("PM_BENCH_DIRTY", None)
    finally:
        if had_prev:
            shutil.copy2(prev, dst)
            prev.unlink()
        elif dst.exists():
            dst.unlink()
        shutil.rmtree(scratch, ignore_errors=True)
    print("build-at %s: map_restore_table.c md5 before %s after %s (%s)" % (
        short, md5_before, md5_of(main_map), "unchanged" if md5_of(main_map) == md5_before else "CHANGED"))
    return rc


def same_commit(a, b):
    """True when two commit ids (full or short, at least 7 characters) name the same commit."""
    return bool(a) and bool(b) and len(min(a, b, key=len)) >= 7 and (a.startswith(b) or b.startswith(a))


def cmd_ingest(a):
    host = norm_host(a.host)
    if host not in HOSTS:
        print("host must be one of %s (got %s)" % (", ".join(HOSTS), a.host))
        return 5
    p = Path(a.log)
    if not p.exists():
        print("no such log: %s" % a.log)
        return 5
    text = p.read_text(encoding="utf-8", errors="replace")
    if host in ("o3ds", "n3ds"):
        # A console log with a boot line is the binary that wrote it: a different --build or --commit is refused, nothing is written.
        lb, lc = log_build(text)
        if lb is not None:
            bad = []
            if a.build is not None and a.build != lb:
                bad.append("--build %s" % a.build)
            if a.commit and not same_commit(a.commit, lc):
                bad.append("--commit %s" % a.commit)
            if bad:
                print("ERROR: %s boot line is build %d git %s, %s differs. Nothing written." % (a.log, lb, lc, " and ".join(bad)))
                return 5
    b, c = resolve_build(p, text, a.build, a.commit, host)
    speed = a.speed or ("unknown" if host == "azahar" else "normal")
    dv = os.environ.get("PM_BENCH_DIRTY")  # set by build-at (lane build of a commit: dirty false)
    return ingest_file(p, a.bench, host, b, c, a.row, a.label, "imported", a.note, None, [], device=a.device or device_note(host), speed=speed,
                       dirty=None if dv is None else dv == "1")


def cmd_ingest_last(a):
    p = Path(a.log)
    if not p.exists():
        print("no log at %s. Nothing ingested." % a.log)
        return 0
    s = a.scenario
    bd = next((b for b in load_defs()["benches"] if b.get("scenario") == s and b["kind"].startswith("azahar")), None)
    bench = bd["name"] if bd else "adhoc:" + s
    text = p.read_text(encoding="utf-8", errors="replace")
    b, c = resolve_build(p, text, a.build, a.commit, "azahar")
    result = "log_pass" if a.rc == 0 else "fail:%d" % a.rc
    note = "picture: a person reads the shot" if bd and bd["kind"] == "azahar_pic" else ""
    shot = a.shots or str(SHOTS / s)
    dv = os.environ.get("PM_BENCH_DIRTY")
    return ingest_file(p, bench, "azahar", b, c, a.row, a.label or s, result, note, shot, [], device=DEVICE["azahar"],
                       kind=bd["kind"] if bd else "adhoc", speed=a.speed, frame_limit=a.frame_limit,
                       dirty=None if dv is None else dv == "1")


def cmd_import_csv(a):
    existing = {r.get("src") for r in read_runs()}
    recs = []
    with CSV.open(newline="", encoding="utf-8") as f:
        for i, row in enumerate(csv.DictReader(f)):
            src = "perf_history.csv:%d" % (i + 2)
            if src in existing:
                continue
            bench = "toad_standing" if "mac_01 m4 sp-107" in row["scene"] else "untagged"
            run = {k: float(row[c]) for k, c in (("fps", "fps"), ("ft_ms", "ft_ms"), ("nd_ms", "nd_ms"), ("lg_ms", "lg_ms"), ("gb_ms", "gb_ms")) if row[c]}
            run["windows"] = float(row["windows"])
            host = norm_host(row["console"])
            rec = make_run(bench, "console_prof", host, None, row["commit"], "legacy_import", row["label"], "imported", row["scene"],
                           None, None, row["note"], ["legacy_import"], run, [], row["x_range"], row["z_range"],
                           device=DEVICE_O3DS_OLD if host == "o3ds" else device_note(host), date=row["date"],
                           speed="unknown" if host == "azahar" else "normal")
            rec["ts"] = row["date"] + "T00:00:00"
            rec["src"] = src
            recs.append(rec)
    if not recs:
        print("nothing to import: all rows of %s are in runs.jsonl" % CSV.name)
        return 0
    n = append_runs(recs)
    print("imported %d row(s) from %s as runs (run ids up to %d)" % (len(recs), CSV.name, n[1]))
    return 0


def flt_of(a):
    """Area filter dict from the parsed arguments (where, spots, compare, trend)."""
    f = {}
    if getattr(a, "map", None):
        f["map"] = a.map
    for k in AREA:
        v = getattr(a, k, None)
        if v is not None:
            f[k] = int(v)
    for ax in ("x", "y", "z"):
        v = getattr(a, ax, None)
        if v:
            lo, hi = v.split("..", 1)
            f[ax] = (float(lo), float(hi))
    if getattr(a, "host", None):
        f["host"] = norm_host(a.host)
    if getattr(a, "chg", False):
        f["chg"] = True
    if getattr(a, "device", None):
        f["device"] = a.device
    if getattr(a, "disp", None):
        f["disp"] = a.disp
    return f


def add_area(x):
    x.add_argument("--map")
    for k in AREA:
        x.add_argument("--" + k, type=int)
    for ax in ("x", "y", "z"):
        x.add_argument("--" + ax, help="LO..HI; use the = form for negatives: --%s=-300..0" % ax)
    x.add_argument("--host", choices=HOSTS + tuple(HOST_ALIAS))
    x.add_argument("--chg", action="store_true")
    x.add_argument("--disp", type=disp_arg, help="display mode w<0|1>s<0|1>f<fps> (w1s0f30), - (old logs), or any. Default: the mode of the newest row")
    x.add_argument("--device", help="run device, a substring (131 = old O3DS XL, 170 = O3DS) or any. Default: the current device of the host")
    x.add_argument("--speed", choices=SPEED_CHOICES, help="turbo | normal | unknown | any (default: the bench speed; where, spots: any)")


def fmt(v, spec):
    return "-" if v is None else format(v, spec)


def area_desc(f):
    return " ".join("%s=%s" % (k, "%g..%g" % v if isinstance(v, tuple) else v) for k, v in f.items()) or "all"


def speed_arg(a, bd):
    """Speed of a query: --speed, else the bench speed (bench_defs.json). Without a bench (where, spots): any."""
    if getattr(a, "speed", None):
        return a.speed
    return "any" if bd is None else bd.get("speed", "normal")


def speed_blocks(arg, rows):
    """Speeds to print, in order. One block per speed: turbo and normal rows are never averaged together."""
    if arg != "any":
        return [arg]
    have = {r["speed"] for r in rows}
    return [s for s in SPEEDS if s in have] + sorted(s for s in have if s not in SPEEDS)


def speed_note(rs):
    fl = sorted({r.get("frame_limit") for r in rs if r.get("frame_limit") is not None})
    return "frame_limit %s" % ",".join("%g" % v for v in fl) if fl else "frame_limit -"


def where_block(rs):
    groups = collections.OrderedDict()
    for r in rs:
        groups.setdefault((r["build_no"], r["commit_hash"], r["date"], r["host"]), []).append(r)
    order = sorted(groups.items(), key=lambda kv: min(x["ts"] or "" for x in kv[1]))
    print("  %-6s %-10s %-10s %-6s %-7s %7s %6s %7s %6s %6s %6s %7s %7s" % ("build", "commit", "date", "host", "disp", "windows", "fps", "ft_ms", "nd_ms", "lg_ms", "gb_ms", "d_fps", "d_ft"))
    first = None
    for (bn, commit, date, host), g in order:
        m = {k: wmean(g, k) for k in MET}
        d_fps = d_ft = ""
        if first is None:
            first = m
        else:
            d_fps = "" if m["fps"] is None or first["fps"] is None else "%+.1f" % (m["fps"] - first["fps"])
            d_ft = "" if m["ft_ms"] is None or first["ft_ms"] is None else "%+.1f" % (m["ft_ms"] - first["ft_ms"])
        print("  %-6s %-10s %-10s %-6s %-7s %7d %6s %7s %6s %6s %6s %7s %7s" % (
            fmt(bn, "d"), (commit or "-")[:10], date or "-", host, disp_label(g), sum(r["n"] for r in g),
            fmt(m["fps"], ".1f"), fmt(m["ft_ms"], ".1f"), fmt(m["nd_ms"], ".1f"), fmt(m["lg_ms"], ".1f"), fmt(m["gb_ms"], ".1f"), d_fps, d_ft))


def cmd_where(a):
    f = flt_of(a)
    con = open_db()
    rows = disp_select(select_samples(con, f), f)
    con.close()
    desc = area_desc(f)
    if not rows:
        print("where %s: no samples" % desc)
        return 1
    print("where %s (chg windows %s)" % (desc, "included" if f.get("chg") else "excluded"))
    for s in speed_blocks(speed_arg(a, None), rows):
        rs = [r for r in rows if r["speed"] == s]
        print("speed %s, %s" % (s, speed_note(rs)))
        if rs:
            where_block(rs)
        else:
            print("  no samples at speed %s" % s)
    return 0


def spots_block(rs, top):
    spots = collections.OrderedDict()
    for r in rs:
        if r["x"] is None or r["z"] is None:
            continue
        key = (r["map"], r["entry"], int(round(r["x"] / SPOT_STEP)) * SPOT_STEP, int(round(r["z"] / SPOT_STEP)) * SPOT_STEP)
        spots.setdefault(key, []).append(r)
    if not spots:
        print("  no spots: no window with a position in the selection (imported history has none)")
        return False
    ranked = sorted(spots.items(), key=lambda kv: (-len({r["run_id"] for r in kv[1]}), -sum(r["n"] for r in kv[1])))[:top]
    print("  spots (end position rounded to %d units; %d spot(s) in all)" % (SPOT_STEP, len(spots)))
    print("  %-14s %5s %6s %6s %-7s %5s %8s %6s %7s" % ("map", "entry", "x", "z", "disp", "runs", "windows", "fps", "ft_ms"))
    for (mp, en, x, z), g in ranked:
        print("  %-14s %5s %6d %6d %-7s %5d %8d %6s %7s" % (mp, en, x, z, disp_label(g), len({r["run_id"] for r in g}), sum(r["n"] for r in g),
              fmt(wmean(g, "fps"), ".1f"), fmt(wmean(g, "ft_ms"), ".1f")))
    return True


def cmd_spots(a):
    f = flt_of(a)
    con = open_db()
    rows = disp_select(select_samples(con, f), f)
    con.close()
    if not rows:
        print("no spots: no window in the selection")
        return 1
    print("spots, area %s" % area_desc(f))
    ok = False
    for s in speed_blocks(speed_arg(a, None), rows):
        rs = [r for r in rows if r["speed"] == s]
        print("speed %s, %s" % (s, speed_note(rs)))
        ok = spots_block(rs, a.top) or ok
    return 0 if ok else 1


def pick_builds(runs, a, b):
    def sel(key):
        return [r for r in runs if key == str(r["build_no"]) or (r["commit"] or "").startswith(key) and len(key) >= 4]
    return sel(a), sel(b)


def cmd_compare(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s" % a.bench)
        return 4
    f = flt_of(a)
    runs = disp_select(filtered_runs(a.bench, f) if has_area(f) else bench_runs(a.bench, f), f)
    if a.last:
        seen = []
        for r in runs:
            if r["commit"] not in seen:
                seen.append(r["commit"])
        if len(seen) < 2:
            print("need two builds for %s, found %d" % (a.bench, len(seen)))
            return 1
        ka, kb = seen[-2], seen[-1]
    else:
        ka, kb = a.A, a.B
    A, B = pick_builds(runs, ka, kb)
    if not A or not B:
        print("no runs for %s or %s in %s (scene filter %s, area %s)" % (ka, kb, a.bench, bd.get("scene_match") or "none", f or "none"))
        return 1

    def mean_m(rs):
        keys = sorted({k for r in rs for k in r["m"]})
        return {k: sum(r["m"][k] for r in rs if k in r["m"]) / len([r for r in rs if k in r["m"]]) for k in keys}
    sa = sorted({r["speed"] for r in A})
    sb = sorted({r["speed"] for r in B})
    same = sa == sb
    print("compare %s: %s vs %s; delta = B - A; area %s; speed %s; disp %s" % (a.bench, ka, kb, f or "all", speed_arg(a, bd),
          disp_label(A + B) if A + B else "-"))
    if not same:
        print("WARNING: different speeds. %s runs: %s. %s runs: %s. No delta is printed." % (ka, ", ".join(sa), kb, ", ".join(sb)))
    for s in speed_blocks(speed_arg(a, bd), A + B):
        As = [r for r in A if r["speed"] == s]
        Bs = [r for r in B if r["speed"] == s]
        if not As and not Bs:
            print("speed %s: no runs of %s or %s" % (s, ka, kb))
            continue
        ma, mb = mean_m(As), mean_m(Bs)
        print("speed %s, %s: %d run(s) vs %d run(s)" % (s, speed_note(As + Bs), len(As), len(Bs)))
        print("  %-14s %10s %10s %10s %9s" % ("metric", ka[:10], kb[:10], "delta", "pct"))
        keys = [k for k in ORDER if k in ma or k in mb] + sorted(k for k in set(ma) | set(mb) if k not in ORDER)
        for k in keys:
            va, vb = ma.get(k), mb.get(k)
            if same and va is not None and vb is not None:
                pct = (vb - va) / va * 100 if va else float("nan")
                print("  %-14s %10.2f %10.2f %+10.2f %+8.1f%%" % (k, va, vb, vb - va, pct))
            else:
                print("  %-14s %10s %10s %10s %9s" % (k, fmt(va, ".2f"), fmt(vb, ".2f"), "-", "-"))
        if same and Bs:
            _, lines = gate_lines(bd, [r for r in runs if r["speed"] == s], mb)
            print("gates on %s:" % kb)
            for ln in lines or ["none"]:
                print("  " + ln)
    return 0


def cmd_trend(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s" % a.bench)
        return 4
    f = flt_of(a)
    runs = disp_select(filtered_runs(a.bench, f) if has_area(f) else bench_runs(a.bench, f), f)
    for s in speed_blocks(speed_arg(a, bd), runs):
        rs = [r for r in runs if r["speed"] == s]
        print("speed %s, %s" % (s, speed_note(rs)))
        if a.metric:
            print("trend %s %s (area %s)" % (a.bench, a.metric, f or "all"))
            prev = None
            for r in rs:
                if a.metric not in r["m"]:
                    continue
                v = r["m"][a.metric]
                d = "" if prev is None else "  d %+.2f" % (v - prev)
                print("  %s %-10s %-7s %10.2f%s" % (r["date"], r["commit"] or "-", r["disp"], v, d))
                prev = v
            continue
        scenes = []
        for r in rs:
            if r["scene"] not in scenes:
                scenes.append(r["scene"])
        for sc in scenes:
            print(sc)
            prev = None
            for r in [x for x in rs if x["scene"] == sc]:
                m = r["m"]
                fps, ft = m.get("fps", float("nan")), m.get("ft_ms", float("nan"))
                d = "" if prev is None else "  d fps %+.1f d ft %+.1f" % (fps - prev[0], ft - prev[1])
                print("  %s %-8s %-7s w%-2s fps %5s ft %5s nd %5s lg %5s gb %5s x %-10s z %-10s %s%s" % (
                    r["date"], r["commit"], r["disp"], int(m.get("windows", 0)), "%.1f" % fps, "%.1f" % ft, "%.1f" % m.get("nd_ms", float("nan")),
                    "%.1f" % m.get("lg_ms", float("nan")), "%.1f" % m.get("gb_ms", float("nan")), r["x_range"], r["z_range"], r["label"], d))
                prev = (fps, ft)
    return 0


def cmd_check(a):
    con = open_db()
    row = con.execute("SELECT * FROM run WHERE id = ?", (a.run_id,)).fetchone()
    if not row:
        print("no run %d" % a.run_id)
        return 4
    bd = bench_def(row["bench"]) or {}
    sp = row["speed"] or "unknown"
    runs = [r for r in bench_runs(row["bench"]) if r["speed"] == sp]
    vals = {k: v for k, v in con.execute("SELECT key, value FROM metric WHERE run_id = ? AND win = 'run'", (a.run_id,)).fetchall()}
    con.close()
    ok, lines = gate_lines(bd, runs, vals)
    print("run %d %s (%s) speed %s" % (a.run_id, row["bench"], row["scene"] or "-", sp))
    for ln in lines or ["no gates"]:
        print("  " + ln)
    print("result: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def cmd_rebuild(a):
    nb, nr, nm, ns = rebuild_db()
    print("rebuilt %s: %d bench(es), %d run(s), %d metric row(s), %d sample row(s)" % (DB.relative_to(REPO), nb, nr, nm, ns))
    return 0


def main(argv):
    p = argparse.ArgumentParser(description="named benches, run log, sample index and area queries")
    sub = p.add_subparsers(dest="cmd", required=True)
    x = sub.add_parser("list"); x.add_argument("--tag"); x.set_defaults(f=cmd_list)
    x = sub.add_parser("show"); x.add_argument("bench"); x.set_defaults(f=cmd_show)
    x = sub.add_parser("run"); x.add_argument("bench"); x.add_argument("--row", required=True)
    x.add_argument("--why", default=""); x.add_argument("--label", default=""); x.add_argument("--build", type=int); x.add_argument("--commit")
    x.add_argument("--speed", choices=("turbo", "normal"))
    x.set_defaults(f=cmd_run)
    x = sub.add_parser("ingest"); x.add_argument("log"); x.add_argument("--bench", required=True); x.add_argument("--host", required=True)
    x.add_argument("--build", type=int); x.add_argument("--commit"); x.add_argument("--row", default="manual"); x.add_argument("--label", default="")
    x.add_argument("--note", default=""); x.add_argument("--device", default=""); x.add_argument("--speed", choices=SPEEDS)
    x.set_defaults(f=cmd_ingest)
    x = sub.add_parser("ingest-last"); x.add_argument("--log", required=True); x.add_argument("--scenario", required=True)
    x.add_argument("--row", default="azt"); x.add_argument("--rc", type=int, required=True); x.add_argument("--label", default="")
    x.add_argument("--build", type=int); x.add_argument("--commit"); x.add_argument("--shots", default="")
    x.add_argument("--speed", choices=SPEEDS, default="unknown"); x.add_argument("--frame-limit", type=float)
    x.set_defaults(f=cmd_ingest_last)
    x = sub.add_parser("build-at"); x.add_argument("commit"); x.add_argument("bench"); x.add_argument("--row", required=True)
    x.add_argument("--why", default=""); x.add_argument("--label", default=""); x.add_argument("--dry-run", action="store_true")
    x.set_defaults(f=cmd_build_at)
    x = sub.add_parser("import-csv"); x.set_defaults(f=cmd_import_csv)
    x = sub.add_parser("where"); add_area(x); x.set_defaults(f=cmd_where)
    x = sub.add_parser("spots"); add_area(x); x.add_argument("--top", type=int, default=20); x.set_defaults(f=cmd_spots)
    x = sub.add_parser("compare"); x.add_argument("bench"); x.add_argument("A", nargs="?"); x.add_argument("B", nargs="?")
    x.add_argument("--last", type=int); add_area(x); x.set_defaults(f=cmd_compare)
    x = sub.add_parser("trend"); x.add_argument("bench"); x.add_argument("metric", nargs="?"); add_area(x); x.set_defaults(f=cmd_trend)
    x = sub.add_parser("check"); x.add_argument("run_id", type=int); x.set_defaults(f=cmd_check)
    x = sub.add_parser("rebuild"); x.set_defaults(f=cmd_rebuild)
    a = p.parse_args(argv)
    if a.cmd == "compare" and not a.last and not (a.A and a.B):
        p.error("compare needs <buildA> <buildB> or --last 2")
    if a.cmd == "compare" and a.last:
        a.last = True
    return a.f(a)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
