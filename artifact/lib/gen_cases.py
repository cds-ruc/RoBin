#!/usr/bin/env python3
"""Generate artifact/cases.json: the evaluation case set for RoBin.

The case set is deliberately small: for every index/dataset it keeps the
baseline case, the "constructed on the entire dataset" case, the best and the
worst lookup case, and the worst insert case.  Those are the feature points the
paper's findings are read off from; the remaining grid points only widen the
strip the reference figures draw.

One case is one configuration, run once.  A configuration several experiments
need (e.g. a baseline that is also a sensitivity reference) appears once, with
all of them in its ``experiments`` list.

Usage:
    python3 artifact/lib/gen_cases.py            # writes artifact/cases.json
"""

from __future__ import annotations

import csv
import json
import statistics
import sys
from pathlib import Path

import layout

HERE = layout.ARTIFACT
REFERENCE = layout.REFERENCE

# case families of this generator -> experiment directories of the artifact
FAMILY_EXPERIMENT = {
    "thp_single": "fig03_tab1_single_thread",
    "thp_multi": "fig04_tab2_multi_thread",
    "tail_single": "fig05_06_tail_latency",
    "tail_multi": "fig05_06_tail_latency",
    "space": "fig07_space",
    "sensitivity": "fig08_sensitivity",
    "oob": "fig09_out_of_bound",
    "preload": "fig16_data_augmentation",
}

DATASETS = ["linear", "covid", "fb-1", "osm"]
CORE_SINGLE = ["btree", "art", "alex", "lipp"]
EXT_SINGLE = ["dytis", "dili"]
CORE_MULTI = ["btreeolc", "artolc", "alexolc"]
EXT_MULTI = ["sali", "finedex"]

FULL_SIZE = 200_000_000
BASELINE_BULK = 100_000_000

SUITE_NAME = {
    "10": ("full", "sorted"),
    "21": ("uniform", "sorted"),
    "22": ("uniform", "shuffled"),
    "41": ("segmented", "sorted"),
    "42": ("segmented", "shuffled"),
}


def phase_of(row: dict) -> str:
    """Phase a record actually measures (mirrors benchmark.h after the fix)."""
    if row.get("measurement_phase"):
        return row["measurement_phase"]
    if float(row["read_ratio"]) > 0.9999:
        return "lookup"
    if float(row["insert_ratio"]) > 0.9999:
        # test_suite 10 records "insert" throughput that is derived from the
        # bulkload duration; it must never be read as an insert measurement.
        return "bulkload" if row["test_suite"] == "10" else "insert"
    return "mixed"


def load_reference(name: str) -> list[dict]:
    path = REFERENCE / name
    if not path.exists():
        sys.exit(
            f"missing {path}; run artifact/data/fetch_reference.sh first "
            "(it exports the reference CSVs from the conf_rel branch)"
        )
    with path.open(newline="") as fh:
        return list(csv.DictReader(fh))


def num(row: dict, field: str) -> float:
    try:
        return float(row[field])
    except (KeyError, TypeError, ValueError):
        return float("nan")


def case_key(row: dict) -> tuple:
    return (
        row["key_path"].split("/")[-1],
        row["index_type"],
        row["test_suite"],
        row["init_table_size"],
        row["thread_num"],
    )


def aggregate(rows: list[dict]) -> dict[tuple, dict]:
    """Mean of every metric per (dataset, index, suite, bulkload, threads)."""
    buckets: dict[tuple, dict] = {}
    for row in rows:
        key = case_key(row)
        entry = buckets.setdefault(
            key,
            {
                "lookup_throughput": [],
                "insert_throughput": [],
                "bulkload_throughput": [],
                "memory": [],
                "lookup_999": [],
                "insert_999": [],
                "insert_avg": [],
            },
        )
        phase = phase_of(row)
        if phase == "lookup":
            entry["lookup_throughput"].append(num(row, "throughput"))
            entry["lookup_999"].append(num(row, "99.9 percentile"))
        elif phase == "insert":
            entry["insert_throughput"].append(num(row, "throughput"))
            entry["insert_999"].append(num(row, "99.9 percentile"))
            entry["insert_avg"].append(num(row, "avg"))
        elif phase == "bulkload":
            entry["bulkload_throughput"].append(num(row, "throughput"))
        memory = num(row, "memory_consumption")
        if memory:
            entry["memory"].append(memory)

    out: dict[tuple, dict] = {}
    for key, entry in buckets.items():
        summary = {}
        for name, values in entry.items():
            clean = [v for v in values if v == v]  # drop NaN
            summary[name] = statistics.fmean(clean) if clean else None
        out[key] = summary
    return out


# The paper records the latency of every operation; 1.0 keeps that
# (run.py --latency_sample_ratio).
TAIL_LATENCY_SAMPLE_RATIO = 1.0


def add(cases: dict[str, dict], **kwargs) -> dict:
    dataset = kwargs["dataset"]
    index = kwargs["index"]
    suite = kwargs["suite"]
    bulk = kwargs["bulkload_size"]
    concurrency = kwargs.get("concurrency", 1)
    tail = kwargs.get("tail_lat", False)
    oob = kwargs.get("out_of_bound", False)
    preload = int(kwargs.get("preload_suite", 0))
    experiment = FAMILY_EXPERIMENT[kwargs["family"]]
    sampling, insert_pattern = SUITE_NAME[suite]
    run_id = "|".join(
        [
            dataset,
            index,
            sampling,
            str(bulk),
            insert_pattern,
            f"t{concurrency}",
            "lat" if tail else "thp",
            "oob" if oob else (f"pre{preload}" if preload else "plain"),
        ]
    )
    entry = cases.get(run_id)
    if entry is None:
        entry = {
            "run_id": run_id,
            "experiments": [],
            "dataset": dataset,
            "index": index,
            "sampling_method": sampling,
            "bulkload_size": bulk,
            "insert_pattern": insert_pattern,
            "concurrency": concurrency,
            "tail_lat": tail,
            "latency_sample_ratio": kwargs.get(
                "latency_sample_ratio", TAIL_LATENCY_SAMPLE_RATIO if tail else 1
            ),
            "out_of_bound": oob,
            "preload_suite": preload,
            "suite": suite,
            "roles": [],
            "purpose": [],
        }
        entry["result_file"] = layout.target_file(entry)
        cases[run_id] = entry
    if experiment not in entry["experiments"]:
        entry["experiments"].append(experiment)
    for role in kwargs["roles"]:
        if role not in entry["roles"]:
            entry["roles"].append(role)
    if kwargs.get("purpose") and kwargs["purpose"] not in entry["purpose"]:
        entry["purpose"].append(kwargs["purpose"])
    return entry


def pick(summary: dict, predicate) -> tuple | None:
    candidates = [(key, value) for key, value in summary.items() if predicate(key, value)]
    if not candidates:
        return None
    return min(candidates, key=lambda item: item[1])[0]


def pick_extreme(summary: dict, predicate, field: str, best: bool) -> tuple | None:
    candidates = [
        (key, value[field])
        for key, value in summary.items()
        if predicate(key, value) and value.get(field) is not None
    ]
    if not candidates:
        return None
    chooser = max if best else min
    return chooser(candidates, key=lambda item: item[1])[0]


def main() -> None:
    single_rows = load_reference("single_thread_thp.csv")
    multi_rows = load_reference("multi_thread_thp.csv")
    single_tail = load_reference("single_thread_tail_lat.csv")
    multi_tail = load_reference("multi_thread_tail_lat.csv")
    memory_rows = load_reference("memory_consumption.csv")
    oob_rows = load_reference("single_thread_thp.alex_oob.csv")

    single = aggregate(single_rows)
    multi = aggregate(multi_rows)
    single_lat = aggregate(single_tail)
    multi_lat = aggregate(multi_tail)
    memory = aggregate(memory_rows)
    oob_table = aggregate(oob_rows)
    preload_tables = {suite: aggregate(load_reference(name)) for suite, name in layout.PRELOAD_FILES.items()}

    cases: dict[str, dict] = {}

    # ---- Finding 1/2/3/4: single-threaded throughput ---------------------
    for dataset in DATASETS:
        for index in CORE_SINGLE + EXT_SINGLE:
            tiers = ["core"] if index in CORE_SINGLE else ["extended"]
            add(
                cases,
                family="thp_single",
                dataset=dataset,
                index=index,
                suite="22",
                bulkload_size=BASELINE_BULK,
                roles=["baseline"] + tiers,
                purpose="Finding 1/2/4 baseline <uniform, 100M, shuffled>",
            )
            add(
                cases,
                family="thp_single",
                dataset=dataset,
                index=index,
                suite="10",
                bulkload_size=FULL_SIZE,
                roles=["full_build"] + tiers,
                purpose="Finding 3: constructed on the entire dataset",
            )
            for role, field, best in (
                ("lookup_best", "lookup_throughput", True),
                ("lookup_worst", "lookup_throughput", False),
                ("insert_worst", "insert_throughput", False),
            ):
                key = pick_extreme(
                    single,
                    lambda k, v, d=dataset, i=index, f=field: k[0] == d
                    and k[1] == i
                    and k[4] == "1"
                    and v.get(f) is not None
                    and (f != "insert_throughput" or k[2] != "10"),
                    field,
                    best,
                )
                if key is None:
                    continue
                suite, bulk = key[2], key[3]
                add(
                    cases,
                    family="thp_single",
                    dataset=dataset,
                    index=index,
                    suite=suite,
                    bulkload_size=int(bulk),
                    roles=[role] + tiers,
                    purpose=f"Finding 1/2/4: reference {role}",
                )

    # ---- Finding 5/6: multi-threaded throughput --------------------------
    for dataset in DATASETS:
        for index in CORE_MULTI + EXT_MULTI:
            tiers = ["core"] if index in CORE_MULTI else ["extended"]
            add(
                cases,
                family="thp_multi",
                dataset=dataset,
                index=index,
                suite="22",
                bulkload_size=BASELINE_BULK,
                concurrency=16,
                roles=["baseline"] + tiers,
                purpose="Finding 5/6 baseline <uniform, 100M, shuffled>",
            )
            add(
                cases,
                family="thp_multi",
                dataset=dataset,
                index=index,
                suite="10",
                bulkload_size=FULL_SIZE,
                concurrency=16,
                roles=["full_build"] + tiers,
                purpose="Finding 5: constructed on the entire dataset (16 threads)",
            )
            for role, field, best in (
                ("lookup_best", "lookup_throughput", True),
                ("lookup_worst", "lookup_throughput", False),
                ("insert_worst", "insert_throughput", False),
            ):
                key = pick_extreme(
                    multi,
                    lambda k, v, d=dataset, i=index, f=field: k[0] == d
                    and k[1] == i
                    and k[4] == "16"
                    and v.get(f) is not None
                    and (f != "insert_throughput" or k[2] != "10"),
                    field,
                    best,
                )
                if key is None:
                    continue
                suite, bulk = key[2], key[3]
                add(
                    cases,
                    family="thp_multi",
                    dataset=dataset,
                    index=index,
                    suite=suite,
                    bulkload_size=int(bulk),
                    concurrency=16,
                    roles=[role] + tiers,
                    purpose=f"Finding 5/6: reference {role}",
                )

    # ---- Finding 7: tail latency ----------------------------------------
    for family, table, threads, indexes in (
        ("tail_single", single_lat, 1, CORE_SINGLE),
        ("tail_multi", multi_lat, 16, CORE_MULTI + ["sali"]),
    ):
        for dataset in DATASETS:
            for index in indexes:
                tiers = ["core"] if index in CORE_SINGLE + CORE_MULTI else ["extended"]
                add(
                    cases,
                    family=family,
                    dataset=dataset,
                    index=index,
                    suite="22",
                    bulkload_size=BASELINE_BULK,
                    concurrency=threads,
                    tail_lat=True,
                    roles=["baseline"] + tiers,
                    purpose="Finding 7 baseline tail latency",
                )
                for role, field, best in (
                    ("lookup_tail_worst", "lookup_999", True),
                    ("insert_tail_worst", "insert_999", True),
                ):
                    key = pick_extreme(
                        table,
                        lambda k, v, d=dataset, i=index, f=field: k[0] == d
                        and k[1] == i
                        and k[4] == str(threads)
                        and v.get(f)
                        and (f != "insert_999" or k[2] != "10"),
                        field,
                        best,
                    )
                    if key is None:
                        continue
                    add(
                        cases,
                        family=family,
                        dataset=dataset,
                        index=index,
                        suite=key[2],
                        bulkload_size=int(key[3]),
                        concurrency=threads,
                        tail_lat=True,
                        roles=[role] + tiers,
                        purpose=f"Finding 7: reference {role}",
                    )

    # ---- Finding 8/9: space overhead ------------------------------------
    for dataset in DATASETS:
        for index in ("lipp", "sali", "alex"):
            key = pick_extreme(
                memory,
                lambda k, v, d=dataset, i=index: k[0] == d and k[1] == i,
                "memory",
                True,
            )
            if key is None:
                continue
            threads = 1 if index in CORE_SINGLE else 16
            tiers = ["core"] if index in ("lipp", "alex") else ["extended"]
            add(
                cases,
                family="space",
                dataset=dataset,
                index=index,
                suite=key[2],
                bulkload_size=int(key[3]),
                concurrency=threads,
                roles=["space_worst"] + tiers,
                purpose="Finding 8/9: worst observed space overhead",
            )
    # the abnormal ALEX case analysed in Section 5.4 (Face, segmented 100M)
    add(
        cases,
        family="space",
        dataset="fb-1",
        index="alex",
        suite="42",
        bulkload_size=BASELINE_BULK,
        roles=["space_abnormal", "core"],
        purpose="Section 5.4: ALEX excessive cascading splits on Face",
    )

    # ---- Finding 10/11: sensitivity to one workload dimension -----------
    for dataset in ("linear", "covid"):
        for index in ("alex", "lipp"):
            for role, suite, bulk, purpose in (
                ("bulkload_sweep_0", "22", 0, "Finding 11: no initial data"),
                ("bulkload_sweep_1m", "22", 1_000_000, "Finding 10/11: bulkload size sweep"),
                ("bulkload_sweep_20m", "22", 20_000_000, "Finding 10/11: bulkload size sweep"),
                ("insert_pattern", "21", BASELINE_BULK, "Finding 10: sorted vs shuffled insert"),
                ("sampling_uniform", "22", BASELINE_BULK, "Finding 10: uniform sampling"),
                ("sampling_segmented", "41", BASELINE_BULK, "Finding 10: segmented sampling"),
            ):
                add(
                    cases,
                    family="sensitivity",
                    dataset=dataset,
                    index=index,
                    suite=suite,
                    bulkload_size=bulk,
                    roles=[role, "core"],
                    purpose=purpose,
                )
    for dataset in ("linear", "covid"):
        for role, suite, bulk in (
            ("insert_pattern", "21", BASELINE_BULK),
            ("sampling_segmented", "41", BASELINE_BULK),
            ("bulkload_sweep_1m", "22", 1_000_000),
        ):
            add(
                cases,
                family="sensitivity",
                dataset=dataset,
                index="art",
                suite=suite,
                bulkload_size=bulk,
                roles=[role, "core"],
                purpose="Finding 10: traditional-index reference for the same workload change",
            )

    # ---- Finding 12: out-of-bounds handling ------------------------------
    for dataset in DATASETS:
        for suite in ("41", "42"):
            add(
                cases,
                family="oob",
                dataset=dataset,
                index="alex",
                suite=suite,
                bulkload_size=BASELINE_BULK,
                out_of_bound=True,
                roles=["oob_outlier", "core"],
                purpose="Finding 12: segmented sampling with a single outlier in bulkload",
            )
            add(
                cases,
                family="oob",
                dataset=dataset,
                index="alex",
                suite=suite,
                bulkload_size=BASELINE_BULK,
                out_of_bound=False,
                roles=["oob_reference", "core"],
                purpose="Finding 12: same case without the outlier",
            )
    # the worst released pairs (lookup down to ~21% / ~43% of the plain run)
    for dataset, suite in (("linear", "41"), ("fb-1", "42")):
        for outlier in (True, False):
            add(
                cases,
                family="oob",
                dataset=dataset,
                index="alex",
                suite=suite,
                bulkload_size=1_000_000,
                out_of_bound=outlier,
                roles=["oob_worst", "core"],
                purpose="Finding 12: worst released outlier pair (segmented, 1M bulkload)",
            )

    # Finding 9: SALI against LIPP on the same workload (largest released ratio band)
    for index, threads in (("sali", 16), ("lipp", 1)):
        add(
            cases,
            family="space",
            dataset="linear",
            index=index,
            suite="42",
            bulkload_size=2_000_000,
            concurrency=threads,
            roles=["space_concurrency", "core"],
            purpose="Finding 9: concurrent SALI vs single-threaded LIPP, same workload",
        )

    # ---- Section 6.1: data augmentation (1% artificial keys) ------------
    # preload_suite 33 samples the uint64 key space, 34 the dataset key space.
    # Each augmented run is paired with the same case without augmentation.
    for dataset, index, suite, bulk, preload, purpose in (
        ("linear", "lipp", "41", 50_000_000, 33, "Section 6.1: LIPP's largest gain (uint64 key space, +694%)"),
        ("linear", "lipp", "41", 100_000_000, 34, "Section 6.1: LIPP's largest gain (dataset key space, +632%)"),
        ("fb-1", "alex", "42", 1_000_000, 34, "Section 6.1: ALEX's largest loss (dataset key space, -53%)"),
    ):
        for suite_value in (preload, 0):
            add(
                cases,
                family="preload",
                dataset=dataset,
                index=index,
                suite=suite,
                bulkload_size=bulk,
                preload_suite=suite_value,
                roles=["augmentation", "core"],
                purpose=purpose if suite_value else "Section 6.1: same case without augmentation",
            )

    # ---- "figures" tier: every case the Fig. 5/6/8 notebooks read --------
    # Those notebooks draw per-index reference lines (baseline, full build,
    # bulkload 0) and the Fig. 8 panels on all four datasets, so they need a
    # fixed case list rather than extremes.  `add` merges a configuration that
    # another experiment already defines.
    figure_tail = (("22", BASELINE_BULK), ("10", FULL_SIZE), ("22", 0))
    for dataset in DATASETS:
        for index in CORE_SINGLE:
            for suite, bulk in figure_tail:
                add(cases, family="tail_single", dataset=dataset, index=index, suite=suite,
                    bulkload_size=bulk, tail_lat=True, roles=["figures"],
                    purpose="Fig. 5: reference cases of the tail-latency figure")
        for index in ("btreeolc", "artolc", "alexolc", "sali"):
            for suite, bulk in figure_tail:
                add(cases, family="tail_multi", dataset=dataset, index=index, suite=suite,
                    bulkload_size=bulk, concurrency=16, tail_lat=True, roles=["figures"],
                    purpose="Fig. 6: reference cases of the tail-latency figure")
        for index in ("alex", "lipp"):
            for bulk in (0, 1, 2, 5, 10, 20, 50, 100):
                add(cases, family="sensitivity", dataset=dataset, index=index, suite="22",
                    bulkload_size=bulk * 1_000_000, roles=["figures"], purpose="Fig. 8a: bulkload size sweep")
            for suite in ("21", "42"):
                add(cases, family="sensitivity", dataset=dataset, index=index, suite=suite,
                    bulkload_size=BASELINE_BULK, roles=["figures"],
                    purpose="Fig. 8b/8c: insert pattern, sampling method")
            add(cases, family="sensitivity", dataset=dataset, index=index, suite="10", bulkload_size=FULL_SIZE,
                roles=["figures"], purpose="Fig. 8: constructed on the entire dataset")

    # ---- AE re-measurement slice ("claims" tier) -------------------------
    # One run per case, chosen so that every finding has at least one
    # measurement of its own on this machine.  The full 124-case grid is never
    # re-run: its results are the `reference` measurements in cases.json.
    claims_selection = [
        # Finding 1-4: extremes and worst-case insert, single-threaded
        ("thp_single", "linear", ("btree", "art", "alex", "lipp"),
         ("baseline", "full_build", "lookup_worst", "insert_worst")),
        ("thp_single", "covid", ("alex", "lipp"), ("baseline", "full_build", "lookup_worst", "insert_worst")),
        # Finding 5-6: 16-threaded throughput and worst-case insert
        ("thp_multi", "linear", ("btreeolc", "artolc", "alexolc"),
         ("baseline", "full_build", "lookup_worst", "insert_worst")),
        ("thp_multi", "covid", ("artolc", "alexolc"), ("insert_worst",)),
        # Finding 7: tail latency
        ("tail_single", "covid", ("btree", "art", "alex", "lipp"), ("baseline", "insert_tail_worst")),
        ("tail_multi", "covid", ("artolc", "alexolc"), ("baseline", "insert_tail_worst")),
        # Finding 8-9 and Section 5.4: space blow-ups
        ("space", "fb-1", ("alex",), ("space_abnormal",)),
        ("space", "osm", ("lipp",), ("space_worst",)),
        ("space", "linear", ("sali", "lipp"), ("space_concurrency",)),
        # Finding 10-11: one-dimension sensitivity
        ("sensitivity", "linear", ("alex", "lipp"),
         ("insert_pattern", "sampling_segmented", "bulkload_sweep_0")),
        # Finding 12: out-of-bounds handling
        ("oob", "fb-1", ("alex",), ("oob_outlier", "oob_reference", "oob_worst")),
        ("oob", "linear", ("alex",), ("oob_worst",)),
        # Section 6.1: data augmentation
        ("preload", "linear", ("lipp",), ("augmentation",)),
        ("preload", "fb-1", ("alex",), ("augmentation",)),
    ]
    for family, dataset, index_pattern, roles in claims_selection:
        experiment = FAMILY_EXPERIMENT[family]
        for index in sorted(set(index_pattern)):
            for role in roles:
                for entry in cases.values():
                    if (
                        experiment in entry["experiments"]
                        and entry["dataset"] == dataset
                        and entry["index"] == index
                        and role in entry["roles"]
                    ):
                        if "claims" not in entry["roles"]:
                            entry["roles"].append("claims")

    # attach the reference (paper-run) metrics of every case, when known
    for entry in cases.values():
        key = (
            entry["dataset"],
            entry["index"],
            entry["suite"],
            str(entry["bulkload_size"]),
            str(entry["concurrency"]),
        )
        if entry["tail_lat"]:
            summary = (single_lat if entry["concurrency"] == 1 else multi_lat).get(key)
        elif entry["out_of_bound"]:
            summary = oob_table.get(key)
        elif entry["preload_suite"]:
            summary = preload_tables[entry["preload_suite"]].get(key)
        else:
            summary = dict((single if entry["concurrency"] == 1 else multi).get(key) or {})
            space = (memory.get(key) or {}).get("memory")
            if space:
                summary["memory"] = space
        entry["reference"] = summary if summary else None

    order = list(layout.EXPERIMENTS)
    ordered = sorted(
        cases.values(),
        key=lambda e: (
            min(order.index(name) for name in e["experiments"]),
            e["dataset"],
            e["index"],
            int(e["bulkload_size"]),
            e["suite"],
            e["run_id"],
        ),
    )
    payload = {
        "generated_from": "origin/conf_rel result CSVs (paper runs)",
        "dataset_size": FULL_SIZE,
        "baseline": {"sampling_method": "uniform", "bulkload_size": BASELINE_BULK, "insert_pattern": "shuffled"},
        "experiments": {name: spec["paper"] for name, spec in layout.EXPERIMENTS.items()},
        "tail_latency_sampling": (
            f"tail-latency cases sample {TAIL_LATENCY_SAMPLE_RATIO:.0%} of operations; "
            "the paper samples every operation"
        ),
        "tiers": {
            "claims": "recommended AE slice: one run per case, every finding covered",
            "figures": "every case the Fig. 5/6/8 notebooks read, to redraw them from new runs",
            "core": "all cases of the indexes printed in the paper figures",
            "extended": "plus the crash/timeout-prone indexes (dytis, dili, sali, finedex)",
        },
        "cases": ordered,
    }
    out_path = HERE / "cases.json"
    with out_path.open("w") as fh:
        json.dump(payload, fh, indent=1, sort_keys=False)
        fh.write("\n")
    print(f"wrote {out_path} with {len(ordered)} runs")


if __name__ == "__main__":
    main()
