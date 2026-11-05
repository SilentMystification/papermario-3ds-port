#!/usr/bin/env python3
"""bench.py: named benchmarks, a run log and an index for comparing profiling numbers between builds.
This is the entry point for every test and profiling run (it replaces the text test catalog).

  python 3ds/tools/bench.py list [--tag T]
  python 3ds/tools/bench.py show <bench>
  python 3ds/tools/bench.py run <bench> --row <row> --why "<text>" [--label L] [--build N] [--commit H]
        azahar_pic, azahar_stream: needs --why. Writes the justification line to 3ds/logs/perf/status_<row>.txt,
        then starts the scenario through azt.ps1 and ingests build3ds/last_log.txt.
        host_test: runs the command, ingests its output (metric rc = exit code).
        console_prof: prints the push command only. Run `ingest` on the log after the push.
  python 3ds/tools/bench.py ingest <log> --bench <bench> --host azahar|o3dsxl|n3ds [--build N] [--commit H] [--row R] [--label L] [--note N]
        Parses the prof lines (prof_by_scene.py) and the log_metrics regexes of the bench. One run per scene.
  python 3ds/tools/bench.py import-csv
        Appends docs/handoff/perf_history.csv rows as runs (tag legacy_import). Rows already imported are skipped.
  python 3ds/tools/bench.py compare <bench> <buildA> <buildB>      (build = commit prefix or build number)
  python 3ds/tools/bench.py compare <bench> --last 2
  python 3ds/tools/bench.py trend <bench> [<metric>]          (no metric: the prof_record.py trend layout, fps and ft)
  python 3ds/tools/bench.py check <run_id>                    (evaluate the gates of one run)
  python 3ds/tools/bench.py rebuild                           (drop and rebuild bench.db from runs.jsonl and bench_defs.json)

Files: 3ds/tools/bench_defs.json (benches, tracked, hand edited). 3ds/logs/perf/runs.jsonl (one JSON line per run, append only,
the source of truth, tracked). 3ds/logs/perf/bench.db (SQLite index, untracked, rebuilt after every write).
Gate: {metric, op, value} or {metric, op, ref: baseline}. The baseline is the earliest run of the bench with that metric.
"""
import argparse, csv, datetime, json, math, os, re, sqlite3, subprocess, sys
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
CSV = REPO / "docs" / "handoff" / "perf_history.csv"
LAST_LOG = REPO / "build3ds" / "last_log.txt"
SHOTS = REPO / "build3ds" / "azt" / "shots"
OUT = REPO / "build3ds" / "bench"
HOSTS = ("azahar", "o3dsxl", "n3ds")
RENAME = {"fps": "fps", "ft": "ft_ms", "nd": "nd_ms", "lg": "lg_ms", "gb": "gb_ms", "dr": "draws", "tri": "tris"}
ORDER = ["fps", "ft_ms", "nd_ms", "lg_ms", "gb_ms", "draws", "tris", "windows"]
OPS = {"==": lambda a, b: a == b, "!=": lambda a, b: a != b, "<": lambda a, b: a < b,
       "<=": lambda a, b: a <= b, ">": lambda a, b: a > b, ">=": lambda a, b: a >= b}
SCHEMA = """
CREATE TABLE bench(name TEXT PRIMARY KEY, kind TEXT, purpose TEXT, defs_json TEXT);
CREATE TABLE build(build_no INTEGER, commit_hash TEXT, date TEXT);
CREATE TABLE run(id INTEGER PRIMARY KEY, bench TEXT, kind TEXT, host TEXT, build_no INTEGER, commit_hash TEXT, date TEXT, ts TEXT,
  row TEXT, label TEXT, result TEXT, scene TEXT, log_path TEXT, shot_path TEXT, note TEXT, tags TEXT, x_range TEXT, z_range TEXT);
CREATE TABLE metric(run_id INTEGER, win TEXT, key TEXT, value REAL);
CREATE INDEX run_bench ON run(bench);
CREATE INDEX metric_run ON metric(run_id);
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


def posix(p):
    s = str(p)
    return "/" + s[0].lower() + s[2:].replace("\\", "/") if len(s) > 2 and s[1] == ":" else s.replace("\\", "/")


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
    return [json.loads(x) for x in RUNS.read_text(encoding="utf-8").splitlines() if x.strip()]


def append_runs(recs):
    PERF.mkdir(parents=True, exist_ok=True)
    n = len(read_runs())
    with RUNS.open("a", encoding="utf-8", newline="\n") as f:
        for r in recs:
            n += 1
            r["run_id"] = n
            f.write(json.dumps(r, separators=(",", ":")) + "\n")
    return rebuild_db()


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
    nm = 0
    for r in runs:
        con.execute("INSERT INTO run VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
                    (r["run_id"], r["bench"], r.get("kind"), r.get("host"), r.get("build_no"), r.get("commit"), r.get("date"), r.get("ts"),
                     r.get("row"), r.get("label"), r.get("result"), r.get("scene", ""), r.get("log_path"), r.get("shot_path"), r.get("note"),
                     ",".join(r.get("tags", [])), r.get("x_range", ""), r.get("z_range", "")))
        rows = [(r["run_id"], "run", k, float(v)) for k, v in r.get("run", {}).items()]
        for w in r.get("windows", []):
            rows += [(r["run_id"], w["win"], k, float(v)) for k, v in w["m"].items()]
        con.executemany("INSERT INTO metric VALUES (?,?,?,?)", rows)
        nm += len(rows)
    con.execute("INSERT INTO build SELECT MAX(build_no), commit_hash, MIN(date) FROM run GROUP BY commit_hash")
    con.commit()
    con.close()
    return len(defs["benches"]), len(runs), nm


def open_db():
    if not DB.exists():
        rebuild_db()
    con = sqlite3.connect(DB)
    con.row_factory = sqlite3.Row
    return con


def bench_runs(name):
    """Run-level records of one bench, in run order, filtered by the bench scene_match (when set)."""
    bd = bench_def(name) or {}
    sm = bd.get("scene_match")
    con = open_db()
    rs = con.execute("SELECT r.id, r.date, r.commit_hash, r.build_no, r.scene, r.host, r.result, r.row, r.label, r.x_range, r.z_range, r.note, "
                     "m.key, m.value FROM run r LEFT JOIN metric m ON m.run_id = r.id AND m.win = 'run' WHERE r.bench = ? ORDER BY r.id, m.key",
                     (name,)).fetchall()
    con.close()
    out = {}
    for x in rs:
        if sm and sm not in (x["scene"] or ""):
            continue
        d = out.setdefault(x["id"], {"id": x["id"], "date": x["date"], "commit": x["commit_hash"], "build_no": x["build_no"], "scene": x["scene"] or "",
                                     "host": x["host"], "result": x["result"], "row": x["row"], "label": x["label"] or "", "x_range": x["x_range"] or "",
                                     "z_range": x["z_range"] or "", "note": x["note"] or "", "m": {}})
        if x["key"] is not None:
            d["m"][x["key"]] = x["value"]
    return list(out.values())


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


def mean_of(ws, k):
    v = [w["m"][k] for w in ws if k in w["m"]]
    return sum(v) / len(v) if v else None


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


def pos_ranges(ws):
    ps = [q for w in ws for q in (w.get("pos0"), w.get("pos")) if q]
    if not ps:
        return "", ""
    return ("%d..%d" % (min(p[0] for p in ps), max(p[0] for p in ps)), "%d..%d" % (min(p[2] for p in ps), max(p[2] for p in ps)))


def make_run(bench, kind, host, build, commit, row, label, result, scene, log, shot, note, tags, run, windows, x="", z="", date=None):
    return {"bench": bench, "kind": kind, "host": host, "build_no": build, "commit": commit, "date": date or today(), "ts": now_iso(),
            "row": row, "label": label, "result": result, "scene": scene, "log_path": log, "shot_path": shot, "note": note, "tags": tags,
            "x_range": x, "z_range": z, "run": run, "windows": windows}


def ingest_file(path, bench, host, build, commit, row, label, result="imported", note="", shot=None, tags=None, extra=None):
    text = Path(path).read_text(encoding="utf-8", errors="replace")
    by, dropped = pb.parse(str(path))
    bd = bench_def(bench) or {}
    lm = log_metric_values(bd, text)
    lm.update(extra or {})
    kind = bd.get("kind", "ingest")
    recs = []
    if not by:
        by = {"": []}
    for scene, ws in by.items():
        wins = [{"win": "f%d w%d" % (w["frame"], w.get("wsize", 0)), "m": win_metrics(w)} for w in ws]
        keys = sorted({k for w in wins for k in w["m"]})
        run = {k: mean_of(wins, k) for k in keys}
        run = {k: v for k, v in run.items() if v is not None}
        run["windows"] = float(len(ws))
        run.update(lm)
        x, z = pos_ranges(ws)
        recs.append(make_run(bench, kind, host, build, commit, row, label, result, scene, posix(Path(path).resolve().relative_to(REPO)) if REPO in Path(path).resolve().parents else str(path),
                             shot, note, tags or [], run, wins, x, z))
    n = append_runs(recs)
    print("ingested %s: %d scene run(s), scene-change windows dropped %d, run ids up to %d" % (path, len(recs), dropped, n[1]))
    for r in recs:
        print("  scene %-30s windows %3d fps %s ft %s" % (r["scene"] or "-", int(r["run"]["windows"]),
              "%.1f" % r["run"]["fps"] if "fps" in r["run"] else "-", "%.1f" % r["run"]["ft_ms"] if "ft_ms" in r["run"] else "-"))
    return 0


def cmd_list(a):
    defs = load_defs()
    con = open_db()
    n = 0
    print("%-16s %-14s %5s %5s  %s" % ("bench", "kind", "cap", "runs", "purpose"))
    for b in defs["benches"]:
        if a.tag and a.tag not in b.get("tags", []):
            continue
        runs = con.execute("SELECT COUNT(*) FROM run WHERE bench = ?", (b["name"],)).fetchone()[0]
        print("%-16s %-14s %5s %5d  %s" % (b["name"], b["kind"], b["cap"], runs, b["purpose"]))
        n += 1
    con.close()
    print("%d bench(es)" % n)
    return 0


def push_cmd(bd):
    ip = load_defs()["consoles"][bd["host"]]
    return "PM_ROW=PUSH sh %s/3ds/push_3ds.sh --no-build --ip %s %s %s" % (posix(REPO), ip, load_defs()["push_base"], bd["recipe"])


def cmd_show(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s. See: bench.py list" % a.bench)
        return 4
    print("%s (%s)" % (bd["name"], bd["kind"]))
    print("purpose: %s" % bd["purpose"])
    print("cap: %s s   until: %s" % (bd["cap"], bd.get("until", "-")))
    if bd["kind"].startswith("azahar"):
        s = scenario(bd["scenario"])
        tryf = " -Try" if s and s["status"] == "unverified" else ""
        print("recipe: scenario %s (status %s)" % (bd["scenario"], s["status"] if s else "MISSING"))
        print("run: python 3ds/tools/bench.py run %s --row <row> --why \"<text>\"" % bd["name"])
        print("     (azt.ps1 %s -Row <row>%s)" % (bd["scenario"], tryf))
    elif bd["kind"] == "host_test":
        print("command: %s" % bd["command"])
    else:
        print("recipe: %s" % bd["recipe"])
        print("push: %s" % push_cmd(bd))
    for g in bd.get("gates", []):
        print("gate: %s %s %s" % (g["metric"], g["op"], "baseline" if "ref" in g else g["value"]))
    for m in bd.get("log_metrics", []):
        print("log metric: %s  /%s/" % (m["name"], m["regex"]))
    runs = bench_runs(bd["name"])
    print("last runs (%d total, scene filter %s):" % (len(runs), bd.get("scene_match") or "none"))
    print("  %-4s %-10s %-10s %-26s %-8s %6s %6s %s" % ("id", "date", "commit", "scene", "result", "fps", "ft", "label"))
    for r in runs[-5:]:
        print("  %-4d %-10s %-10s %-26s %-8s %6s %6s %s" % (r["id"], r["date"], r["commit"] or "-", r["scene"][:26] or "-", (r["result"] or "-")[:8],
              "%.1f" % r["m"]["fps"] if "fps" in r["m"] else "-", "%.1f" % r["m"]["ft_ms"] if "ft_ms" in r["m"] else "-", r["label"][:60]))
    return 0


def cmd_run(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s. See: bench.py list" % a.bench)
        return 4
    kind = bd["kind"]
    commit = a.commit or git_head()
    env = dict(os.environ, PM_ROW=a.row)
    if kind.startswith("azahar"):
        if not a.why:
            print("run needs --why \"<text>\": the justification line is written before the start. Nothing was run.")
            return 5
        s = scenario(bd["scenario"])
        if not s:
            print("scenario %s is not in azt_scenarios.json. Nothing was run." % bd["scenario"])
            return 5
        measured = list(dict.fromkeys([m["name"] for m in bd.get("log_metrics", [])] + [g["metric"] for g in bd.get("gates", [])])) or ["picture"]
        PERF.mkdir(parents=True, exist_ok=True)
        with (PERF / ("status_%s.txt" % a.row)).open("a", encoding="utf-8", newline="\n") as f:
            f.write("%s %s scenario %s: measures %s; path azt.ps1 -> run_azahar.ps1; why %s\n" % (now_iso(), bd["name"], s["name"], ", ".join(measured), a.why))
        cmd = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(REPO / "3ds" / "tools" / "azt.ps1"), s["name"], "-Row", a.row]
        if s["status"] == "unverified":
            cmd.append("-Try")
        rc = subprocess.run(cmd, cwd=REPO, env=env).returncode
        result = "log_pass" if rc == 0 else "fail:%d" % rc
        note = "picture: a person reads the shot" if kind == "azahar_pic" else ""
        shot = str(SHOTS / s["name"])
        if not LAST_LOG.exists():
            print("no log at %s. Nothing ingested." % LAST_LOG)
            return 1
        ingest_file(LAST_LOG, bd["name"], "azahar", a.build, commit, a.row, a.label or s["name"], result, note, shot, [])
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
        recs = [make_run(bd["name"], kind, "host", a.build, commit, a.row, a.label or bd["name"], "pass" if rc == 0 else "fail:%d" % rc,
                         "", posix(log.relative_to(REPO)), None, "", [], text_metrics, [])]
        n = append_runs(recs)
        print("run %d: %s rc %d, %s" % (n[1], bd["name"], rc, "PASS" if rc == 0 else "FAIL"))
        return rc
    print("console_prof: push, then ingest the live log. Run this in Git Bash:")
    print(push_cmd(bd))
    ip = load_defs()["consoles"][bd["host"]]
    print("then: python 3ds/tools/bench.py ingest build3ds/hw_live_log_%s.txt --bench %s --host %s --commit %s --row %s --label \"%s\""
          % (ip.split(".")[-1], bd["name"], bd["host"], commit, a.row, a.label or bd["name"]))
    return 0


def cmd_ingest(a):
    if a.host not in HOSTS:
        print("host must be one of %s" % ", ".join(HOSTS))
        return 5
    p = Path(a.log)
    if not p.exists():
        print("no such log: %s" % a.log)
        return 5
    return ingest_file(p, a.bench, a.host, a.build, a.commit or git_head(), a.row, a.label, "imported", a.note, None, [])


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
            rec = make_run(bench, "console_prof", row["console"], None, row["commit"], "legacy_import", row["label"], "imported", row["scene"],
                           None, None, row["note"], ["legacy_import"], run, [], row["x_range"], row["z_range"], date=row["date"])
            rec["ts"] = row["date"] + "T00:00:00"
            rec["src"] = src
            recs.append(rec)
    if not recs:
        print("nothing to import: all rows of %s are in runs.jsonl" % CSV.name)
        return 0
    n = append_runs(recs)
    print("imported %d row(s) from %s as runs (run ids up to %d)" % (len(recs), CSV.name, n[1]))
    return 0


def pick_builds(runs, a, b):
    def sel(key):
        return [r for r in runs if key == str(r["build_no"]) or (r["commit"] or "").startswith(key) and len(key) >= 4]
    return sel(a), sel(b)


def cmd_compare(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s" % a.bench)
        return 4
    runs = bench_runs(a.bench)
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
        print("no runs for %s or %s in %s (scene filter %s)" % (ka, kb, a.bench, bd.get("scene_match") or "none"))
        return 1

    def mean_m(rs):
        keys = sorted({k for r in rs for k in r["m"]})
        return {k: sum(r["m"][k] for r in rs if k in r["m"]) / len([r for r in rs if k in r["m"]]) for k in keys}
    ma, mb = mean_m(A), mean_m(B)
    print("compare %s: %s (%d run(s)) vs %s (%d run(s)); delta = B - A" % (a.bench, ka, len(A), kb, len(B)))
    keys = [k for k in ORDER if k in ma and k in mb] + sorted(k for k in ma if k in mb and k not in ORDER)
    print("  %-14s %10s %10s %10s %9s" % ("metric", ka[:10], kb[:10], "delta", "pct"))
    for k in keys:
        va, vb = ma[k], mb[k]
        pct = (vb - va) / va * 100 if va else float("nan")
        print("  %-14s %10.2f %10.2f %+10.2f %+8.1f%%" % (k, va, vb, vb - va, pct))
    ok, lines = gate_lines(bd, runs, mb)
    print("gates on %s:" % kb)
    for ln in lines or ["none"]:
        print("  " + ln)
    return 0


def cmd_trend(a):
    bd = bench_def(a.bench)
    if not bd:
        print("no bench named %s" % a.bench)
        return 4
    runs = bench_runs(a.bench)
    if a.metric:
        print("trend %s %s" % (a.bench, a.metric))
        prev = None
        for r in runs:
            if a.metric not in r["m"]:
                continue
            v = r["m"][a.metric]
            d = "" if prev is None else "  d %+.2f" % (v - prev)
            print("  %s %-10s %10.2f%s" % (r["date"], r["commit"] or "-", v, d))
            prev = v
        return 0
    scenes = []
    for r in runs:
        if r["scene"] not in scenes:
            scenes.append(r["scene"])
    for sc in scenes:
        print(sc)
        prev = None
        for r in [x for x in runs if x["scene"] == sc]:
            m = r["m"]
            fps, ft = m.get("fps", float("nan")), m.get("ft_ms", float("nan"))
            d = "" if prev is None else "  d fps %+.1f d ft %+.1f" % (fps - prev[0], ft - prev[1])
            print("  %s %-8s w%-2s fps %5s ft %5s nd %5s lg %5s gb %5s x %-10s z %-10s %s%s" % (
                r["date"], r["commit"], int(m.get("windows", 0)), "%.1f" % fps, "%.1f" % ft, "%.1f" % m.get("nd_ms", float("nan")),
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
    runs = bench_runs(row["bench"])
    vals = {k: v for k, v in con.execute("SELECT key, value FROM metric WHERE run_id = ? AND win = 'run'", (a.run_id,)).fetchall()}
    con.close()
    ok, lines = gate_lines(bd, runs, vals)
    print("run %d %s (%s)" % (a.run_id, row["bench"], row["scene"] or "-"))
    for ln in lines or ["no gates"]:
        print("  " + ln)
    print("result: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def cmd_rebuild(a):
    nb, nr, nm = rebuild_db()
    print("rebuilt %s: %d bench(es), %d run(s), %d metric row(s)" % (DB.relative_to(REPO), nb, nr, nm))
    return 0


def main(argv):
    p = argparse.ArgumentParser(description="named benches, run log and index")
    sub = p.add_subparsers(dest="cmd", required=True)
    x = sub.add_parser("list"); x.add_argument("--tag"); x.set_defaults(f=cmd_list)
    x = sub.add_parser("show"); x.add_argument("bench"); x.set_defaults(f=cmd_show)
    x = sub.add_parser("run"); x.add_argument("bench"); x.add_argument("--row", required=True)
    x.add_argument("--why", default=""); x.add_argument("--label", default=""); x.add_argument("--build", type=int); x.add_argument("--commit")
    x.set_defaults(f=cmd_run)
    x = sub.add_parser("ingest"); x.add_argument("log"); x.add_argument("--bench", required=True); x.add_argument("--host", required=True)
    x.add_argument("--build", type=int); x.add_argument("--commit"); x.add_argument("--row", default="manual"); x.add_argument("--label", default="")
    x.add_argument("--note", default=""); x.set_defaults(f=cmd_ingest)
    x = sub.add_parser("import-csv"); x.set_defaults(f=cmd_import_csv)
    x = sub.add_parser("compare"); x.add_argument("bench"); x.add_argument("A", nargs="?"); x.add_argument("B", nargs="?")
    x.add_argument("--last", type=int); x.set_defaults(f=cmd_compare)
    x = sub.add_parser("trend"); x.add_argument("bench"); x.add_argument("metric", nargs="?"); x.set_defaults(f=cmd_trend)
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
