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

Cases run one at a time, pinned to CPU 1 (CPUs 1-16 for the 16-thread cases), as
the paper ran them: concurrent cases slow each other down by different amounts.
A finished case is skipped on the next invocation, so an interrupted run
resumes; ``--force`` runs the selected cases again and replaces their records.

Examples
--------
    python3 artifact/lib/run_cases.py --tier claims
    python3 artifact/lib/run_cases.py --tier claims --experiments fig09_out_of_bound
    python3 artifact/lib/run_cases.py --tier figures --datasets covid
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import shlex
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
    parser.add_argument("--timeout", type=int, default=3600, help="per-case wall-clock limit of the runner, seconds")
    return parser.parse_args()


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


def merge(fresh_csv, target, replace: set[tuple]) -> int:
    """Append the records of one run to its result file, replacing earlier records of the same case."""
    with open(fresh_csv, newline="") as handle:
        reader = csv.DictReader(handle)
        fresh_header, fresh = reader.fieldnames or [], list(reader)
    kept, header = [], fresh_header
    if target.exists():
        with target.open(newline="") as handle:
            reader = csv.DictReader(handle)
            header = reader.fieldnames or fresh_header
            kept = [row for row in reader if config_of(row) not in replace]
    with target.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=header, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(kept + fresh)
    return len(fresh)


def run_case(case: dict, args: argparse.Namespace) -> dict:
    name = safe_name(case["run_id"])
    tmp_csv, log_path = LOGS / f"{name}.csv", LOGS / f"{name}.log"
    tmp_csv.unlink(missing_ok=True)
    cmd = build_command(case, tmp_csv)
    env = dict(os.environ, numanode=args.numa_node)
    with log_path.open("w") as log:
        log.write(" ".join(shlex.quote(part) for part in cmd) + "\n")
        log.flush()
        proc = subprocess.Popen(cmd, cwd=layout.ROOT, stdout=log, stderr=subprocess.STDOUT, env=env)
        try:
            code = proc.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            code = None
    text = log_path.read_text(errors="replace")
    if code is None or "Subprocess timeout" in text:
        status = "timeout"
    elif code != 0 or "Failed command:" in text or not tmp_csv.exists():
        status = "failed"
    else:
        status = "ok"
    records = 0
    target = RESULTS / case["result_file"]
    if tmp_csv.exists():
        # a crashed or timed-out run keeps the phases it finished, as in the paper's runs
        with tmp_csv.open(newline="") as handle:
            configs = {config_of(row) for row in csv.DictReader(handle)}
        records = merge(tmp_csv, target, configs)
        tmp_csv.unlink()
    return {"run_id": case["run_id"], "experiments": case["experiments"], "status": status,
            "exit_code": code, "result_file": case["result_file"], "records": records,
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


def hms(seconds: float) -> str:
    seconds = int(seconds)
    return f"{seconds // 3600}h{seconds % 3600 // 60:02d}m" if seconds >= 3600 else f"{seconds // 60}m{seconds % 60:02d}s"


def main() -> None:
    args = parse_args()
    config = json.loads(open(args.cases).read())
    cases = select_cases(config, args)
    if not cases:
        sys.exit("no cases selected; check --tier / --experiments / --datasets")
    status = json.loads(STATUS.read_text()) if STATUS.exists() else {}
    todo = [case for case in cases if args.force or case["run_id"] not in status]

    if args.dry_run:
        for case in todo:
            print(" ".join(shlex.quote(part) for part in build_command(case, "<tmp>.csv")), "->", case["result_file"])
        print(f"# {len(todo)} of {len(cases)} selected cases to run, "
              f"~{hms(sum(map(estimate_seconds, todo)))} of operations", file=sys.stderr)
        return
    if not BINARY.exists():
        sys.exit(f"{BINARY} not found; build first: bash build.sh release")
    if not (layout.ROOT / "datasets" / "linear").exists():
        sys.exit("datasets not found; run: bash artifact/data/fetch.sh")

    LOGS.mkdir(parents=True, exist_ok=True)
    remaining = sum(map(estimate_seconds, todo))
    print(f"{len(cases)} cases selected, {len(cases) - len(todo)} already done, {len(todo)} to run "
          f"(~{hms(remaining)} of operations, plus loading)", flush=True)
    started = time.time()
    for number, case in enumerate(todo, 1):
        estimate = estimate_seconds(case)
        print(f"[{number}/{len(todo)}] {case['run_id']}  (~{hms(estimate)}, ~{hms(remaining)} left)", flush=True)
        entry = run_case(case, args)
        status[case["run_id"]] = entry
        STATUS.write_text(json.dumps(status, indent=1) + "\n")
        remaining = max(0.0, remaining - estimate)
        print(f"          {entry['status']} -> {entry['result_file']} (+{entry['records']} records)", flush=True)
    write_fail_logs(status, config)
    layout.derive_space(RESULTS)

    counts: dict[str, int] = {}
    for case in cases:
        counts[status[case["run_id"]]["status"]] = counts.get(status[case["run_id"]]["status"], 0) + 1
    print(f"done in {hms(time.time() - started)}: " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
    print(f"results in {RESULTS}")


if __name__ == "__main__":
    main()
