#!/usr/bin/env python3
"""Run cases of artifact/cases.json and collect them into the paper's result files.

Every case is one ``run.py`` invocation at full scale (200M keys).  Its records
are appended to the file the paper's notebooks read, in ``artifact/results/``:

    single_thread_thp.csv, multi_thread_thp.csv, single_thread_tail_lat.csv,
    multi_thread_tail_lat.csv, single_thread_thp.alex_oob.csv,
    single_thread_thp.alex_lipp_preload_1_100_{uint64,dataset}.csv

plus ``memory_consumption.csv`` (derived from the two throughput files) and the
``*_fail_case.log`` lists of run.py's "Failed command:" lines.  Per-case stdout
stays in ``artifact/results/logs/``; ``artifact/results/status.json`` records
each case as ok / failed / timeout.

Cases run one at a time, pinned to one physical core (16 for the 16-thread
cases) of the NUMA node memory is bound to (see run.py): concurrent cases slow
each other down by different amounts.
``--repeat N`` runs every case N times and records the mean of the N runs per
phase (the paper averages its runs; the default is one run per case).  A run
that fails or times out ends the case and its own records are kept.
A finished case is skipped on the next invocation (with ``--repeat N``, a case
recorded with fewer than N runs is run again), so an interrupted run resumes;
``--force`` runs the selected cases again and replaces their records.

Examples
--------
    python3 artifact/lib/run_cases.py --tier claims
    python3 artifact/lib/run_cases.py --tier claims --experiments fig09_out_of_bound
    python3 artifact/lib/run_cases.py --tier figures --datasets covid
    python3 artifact/lib/run_cases.py --tier claims --repeat 3
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import shlex
import signal
import subprocess
import sys
import time

import layout

RESULTS = layout.RESULTS
LOGS = RESULTS / "logs"
STATUS = RESULTS / "status.json"
BINARY = layout.ROOT / "build" / "microbench"
FULL_SIZE = 200_000_000


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--cases", default=str(layout.ARTIFACT / "cases.json"))
    parser.add_argument("--tier", choices=["claims", "figures", "core", "extended", "all"], default="claims")
    parser.add_argument("--experiments", nargs="+", default=None, choices=list(layout.EXPERIMENTS),
                        help="only the cases of these experiments")
    parser.add_argument("--datasets", nargs="+", default=None, help="subset of datasets (linear covid fb-1 osm)")
    parser.add_argument("--indexes", nargs="+", default=None, help="subset of index names")
    parser.add_argument("--max-runs", type=int, default=None, help="stop after N runs")
    parser.add_argument("--force", action="store_true", help="run finished cases again and replace their records")
    parser.add_argument("--dry-run", action="store_true", help="print the commands instead of executing them")
    parser.add_argument("--numa-node", default=os.environ.get("numanode", "0"))
    parser.add_argument("--timeout", type=int, default=3600,
                        help="wall-clock limit of one run, seconds (run.py itself stops microbench at 30 min)")
    parser.add_argument("--repeat", type=int, default=1,
                        help="runs per case; the records are the mean of the runs (default 1)")
    args = parser.parse_args()
    if args.repeat < 1:
        parser.error("--repeat must be at least 1")
    return args


def select_cases(config: dict, args: argparse.Namespace) -> list[dict]:
    cases = config["cases"]
    if args.tier != "all":
        cases = [c for c in cases if args.tier in c["roles"]]
    if args.experiments:
        cases = [c for c in cases if set(c["experiments"]) & set(args.experiments)]
    if args.datasets:
        cases = [c for c in cases if c["dataset"] in args.datasets]
    if args.indexes:
        cases = [c for c in cases if c["index"] in args.indexes]
    return cases[: args.max_runs] if args.max_runs is not None else cases


def safe_name(run_id: str) -> str:
    return run_id.replace("|", "__")


def build_command(case: dict, out_csv) -> list[str]:
    cmd = [
        "python3", "run.py",
        f"--index={case['index']}",
        f"--dataset={case['dataset']}",
        f"--concurrency={case['concurrency']}",
        f"--sampling_method={case['sampling_method']}",
        f"--bulkload_size={case['bulkload_size']}",
        f"--insert_pattern={case['insert_pattern']}",
        f"--dataset_size={FULL_SIZE}",
        "--table_size=-1",
        f"--output_path={out_csv}",
    ]
    if case["tail_lat"]:
        cmd += ["--tail_lat=True", f"--latency_sample_ratio={case['latency_sample_ratio']}"]
    if case["out_of_bound"]:
        cmd.append("--out_of_bound_inject=True")
    if case.get("preload_suite"):
        cmd.append(f"--preload_suite={case['preload_suite']}")
    cmd.append(f"--taskset=1-{case['concurrency']}")
    return cmd


def estimate_seconds(case: dict) -> float:
    """Operation time of one run, from the paper's own throughput for the case."""
    reference = case.get("reference") or {}
    lookup, insert = reference.get("lookup_throughput"), reference.get("insert_throughput")
    seconds = FULL_SIZE / lookup if lookup else 0.0
    if insert:
        seconds += (FULL_SIZE - int(case["bulkload_size"])) / insert
    return seconds


def config_of(row: dict) -> tuple:
    return tuple(str(row.get(column, "") or "0") for column in layout.CONFIG_COLUMNS)


# measured columns of a record; the others identify the case and are taken from the first run
MEASURED = ["throughput", "memory_consumption", "min", "50 percentile", "90 percentile", "99 percentile",
            "99.9 percentile", "99.99 percentile", "max", "avg", "latency_variance"]


def phase_key(row: dict) -> tuple:
    return config_of(row) + (row.get("measurement_phase") or f"{row.get('read_ratio')}/{row.get('insert_ratio')}",)


def mean_of_runs(runs: list[list[dict]]) -> list[dict]:
    """One record per (configuration, phase): the first run's record with the mean of every measured column."""
    groups: dict[tuple, list[dict]] = {}
    for rows in runs:
        for row in rows:
            groups.setdefault(phase_key(row), []).append(row)
    merged = []
    for rows in groups.values():
        record = dict(rows[0])
        for column in MEASURED:
            values = [float(r[column]) for r in rows if r.get(column) not in (None, "")]
            if values:
                mean = sum(values) / len(values)
                record[column] = str(round(mean)) if column in ("throughput", "memory_consumption") else f"{mean:g}"
        merged.append(record)
    return merged


def read_records(path) -> tuple[list[str], list[dict]]:
    if not path.exists() or not path.stat().st_size:
        return [], []
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        return list(reader.fieldnames or []), list(reader)


def merge(fresh_header: list[str], fresh: list[dict], target, replace: set[tuple]) -> int:
    """Append records to their result file, replacing earlier records of the same case."""
    if not fresh_header:
        return 0  # nothing measured: leave the result file as it is
    kept, header = [], fresh_header
    if target.exists() and target.stat().st_size:
        with target.open(newline="") as handle:
            reader = csv.DictReader(handle)
            header = reader.fieldnames or fresh_header
            kept = [row for row in reader if config_of(row) not in replace]
    # write beside the target and rename: an interrupt never leaves a truncated file
    partial = target.with_name(target.name + ".partial")
    with partial.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=header, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(kept + fresh)
    os.replace(partial, target)
    return len(fresh)


def stop(proc: subprocess.Popen) -> None:
    """Stop run.py and everything it started (numactl, taskset, microbench): its whole process group."""
    for sig, grace in ((signal.SIGTERM, 10), (signal.SIGKILL, None)):
        try:
            os.killpg(proc.pid, sig)
        except ProcessLookupError:
            return
        try:
            proc.wait(timeout=grace)
            return
        except subprocess.TimeoutExpired:
            continue


def run_once(case: dict, args: argparse.Namespace, tmp_csv, log) -> tuple[str, int | None, list[str], list[dict]]:
    tmp_csv.unlink(missing_ok=True)
    cmd = build_command(case, tmp_csv)
    log.write(" ".join(shlex.quote(part) for part in cmd) + "\n")
    log.flush()
    start = os.fstat(log.fileno()).st_size  # the child writes to the same file: offsets from its size
    # own session: a timeout or an interrupt stops microbench too, not only run.py
    proc = subprocess.Popen(cmd, cwd=layout.ROOT, stdout=log, stderr=subprocess.STDOUT,
                            env=dict(os.environ, numanode=args.numa_node), start_new_session=True)
    try:
        code = proc.wait(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        stop(proc)
        code = None
    except BaseException:
        stop(proc)
        raise
    with open(log.name, "rb") as handle:
        handle.seek(start)
        text = handle.read().decode(errors="replace")
    header, rows = read_records(tmp_csv)
    tmp_csv.unlink(missing_ok=True)
    if code is None or "Subprocess timeout" in text:
        status = "timeout"
    elif code != 0 or "Failed command:" in text or not rows:
        status = "failed"
    else:
        status = "ok"
    return status, code, header, rows


def run_case(case: dict, args: argparse.Namespace) -> dict:
    name = safe_name(case["run_id"])
    tmp_csv, log_path = LOGS / f"{name}.csv", LOGS / f"{name}.log"
    runs: list[list[dict]] = []
    header: list[str] = []
    status, code = "ok", 0
    log_path.unlink(missing_ok=True)
    with log_path.open("a") as log:  # append mode: the parent's lines never overwrite the child's
        for attempt in range(1, args.repeat + 1):
            if args.repeat > 1:
                log.write(f"# run {attempt}/{args.repeat}\n")
            status, code, run_header, rows = run_once(case, args, tmp_csv, log)
            header = header or run_header
            if status != "ok":
                # a crashed or timed-out run keeps the phases it finished, as in the paper's runs
                runs = [rows]
                break
            runs.append(rows)
    fresh = mean_of_runs(runs) if status == "ok" else runs[0]
    records = merge(header, fresh, RESULTS / case["result_file"], {config_of(row) for row in fresh})
    return {"run_id": case["run_id"], "experiments": case["experiments"], "status": status,
            "exit_code": code, "result_file": case["result_file"], "records": records,
            "repeat": len(runs) if status == "ok" else 1,
            "log": str(log_path.relative_to(RESULTS))}


def write_fail_logs(status: dict, config: dict) -> None:
    by_id = {case["run_id"]: case for case in config["cases"]}
    for threads, name in layout.FAIL_LOGS.items():
        lines = []
        for run_id, entry in status.items():
            case = by_id.get(run_id)
            if not case or case["result_file"] not in (layout.SINGLE_THP, layout.MULTI_THP):
                continue
            if int(case["concurrency"]) != threads:
                continue
            log = RESULTS / entry["log"]
            if log.exists():
                lines += [line for line in log.read_text(errors="replace").splitlines() if "Failed command:" in line]
        (RESULTS / name).write_text("".join(line + "\n" for line in lines))


def done_before(entry: dict | None, repeat: int) -> bool:
    """A recorded case counts as done unless it is ok with fewer runs than --repeat asks for."""
    if entry is None:
        return False
    return entry["status"] != "ok" or entry.get("repeat", 1) >= repeat


def hms(seconds: float) -> str:
    seconds = int(seconds)
    return f"{seconds // 3600}h{seconds % 3600 // 60:02d}m" if seconds >= 3600 else f"{seconds // 60}m{seconds % 60:02d}s"


def main() -> None:
    args = parse_args()
    # SIGTERM/SIGHUP end the runner like Ctrl-C, so the running case is stopped too
    for sig in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, lambda signum, _frame: sys.exit(128 + signum))
    config = json.loads(open(args.cases).read())
    cases = select_cases(config, args)
    if not cases:
        sys.exit("no cases selected; check --tier / --experiments / --datasets")
    status = json.loads(STATUS.read_text()) if STATUS.exists() else {}
    todo = [case for case in cases if args.force or not done_before(status.get(case["run_id"]), args.repeat)]
    runs = f" x {args.repeat} runs" if args.repeat > 1 else ""

    if args.dry_run:
        for case in todo:
            print(" ".join(shlex.quote(part) for part in build_command(case, "<tmp>.csv")), "->", case["result_file"])
        print(f"# {len(todo)} of {len(cases)} selected cases to run{runs}, "
              f"~{hms(args.repeat * sum(map(estimate_seconds, todo)))} of operations", file=sys.stderr)
        return
    if not BINARY.exists():
        sys.exit(f"{BINARY} not found; build first: bash build.sh release")
    if not (layout.ROOT / "datasets" / "linear").exists():
        sys.exit("datasets not found; run: bash artifact/data/fetch.sh")

    LOGS.mkdir(parents=True, exist_ok=True)
    remaining = args.repeat * sum(map(estimate_seconds, todo))
    print(f"{len(cases)} cases selected, {len(cases) - len(todo)} already done, {len(todo)} to run{runs} "
          f"(~{hms(remaining)} of operations, plus loading)", flush=True)
    started = time.time()
    for number, case in enumerate(todo, 1):
        estimate = args.repeat * estimate_seconds(case)
        print(f"[{number}/{len(todo)}] {case['run_id']}  (~{hms(estimate)}, ~{hms(remaining)} left)", flush=True)
        entry = run_case(case, args)
        status[case["run_id"]] = entry
        STATUS.write_text(json.dumps(status, indent=1) + "\n")
        remaining = max(0.0, remaining - estimate)
        mean = f", mean of {entry['repeat']} runs" if entry["repeat"] > 1 else ""
        print(f"          {entry['status']} -> {entry['result_file']} (+{entry['records']} records{mean})", flush=True)
    write_fail_logs(status, config)
    layout.derive_space(RESULTS)

    counts: dict[str, int] = {}
    for case in cases:
        counts[status[case["run_id"]]["status"]] = counts.get(status[case["run_id"]]["status"], 0) + 1
    print(f"done in {hms(time.time() - started)}: " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
    print(f"results in {RESULTS}")


if __name__ == "__main__":
    main()
