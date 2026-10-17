#!/usr/bin/env python3
"""Compute the RoBin claim verdicts and the paper's printed tables.

Inputs
------
``artifact/reference/*.csv``  the paper's released measurements, exported from the
                              ``conf_rel`` branch by ``artifact/data/fetch_reference.sh``.
``artifact/results/*.csv``    measurements taken on this machine, under the same
                              file names (``artifact/lib/run_cases.py`` or ``reproduce.sh``).

Records written before the ``measurement_phase`` column existed have their phase
inferred.  The phase of every record decides which aggregation it may enter:

* ``insert``   - real insert runs only; ``test_suite=10`` (bulkload the whole
  dataset, then nothing) is *never* counted as insert throughput - it is the
  ``bulkload`` phase and feeds lookup/space comparisons only.
* ``lookup``   - read-only pass over the whole dataset.
* ``bulkload`` - construction cost, reported separately.

Two verdict blocks are produced:

* **reference vs paper** - every published number recomputed from the paper's own
  measurements; this checks that the scripts compute what the paper prints.
* **results vs reference** - the same metrics on this machine's results and on
  the released measurements, both restricted to the cases present in both.

Outputs
-------
``artifact/summary.json`` / ``artifact/SUMMARY.md``.  Figures are drawn by the
paper's own notebooks through ``artifact/plot/plot.py``.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import pandas as pd

import layout

HERE = layout.ARTIFACT
REFERENCE = layout.REFERENCE
RESULTS = layout.RESULTS
DATA_BYTES = 3200 * 1024 * 1024  # 200M x 16B, the indexed payload
FULL_SIZE = 200_000_000
DATASETS = ["linear", "covid", "fb-1", "osm"]

REPORT_SINGLE_INDEXES = ["btree", "art", "alex", "lipp"]
REPORT_MULTI_INDEXES = ["btreeolc", "artolc", "alexolc", "sali"]
SINGLE_LEARNED = ["alex", "lipp", "dytis", "dili"]
MULTI_LEARNED = ["alexolc", "sali"]
OFFICIAL = {
    "btree": "B+tree", "btreeolc": "B+tree-OLC", "art": "ART", "artolc": "ART-OLC",
    "alex": "ALEX", "alexolc": "ALEX-OLC", "lipp": "LIPP", "sali": "SALI",
    "dytis": "DyTIS", "dili": "DILI", "finedex": "FINEdex",
    "datasets/linear": "Linear", "datasets/covid": "Covid", "datasets/fb-1": "Face",
    "datasets/osm": "OSM",
}

# The paper's figure and table numbers.  LaTeX never reorders floats of the
# same type, so the number is the order of the *uncommented* floats in the
# sources of robin-paper/ (verified by scanning them; the duplicate space
# figure in section 4.3 is commented out and does not count).
PAPER_MAP = {
    "single_thread_overall_thp": ("Figure 3", "fig:single-lookup-thp", "Lookup throughput of all test cases (single-threaded)."),
    "multi_thread_overall_thp": ("Figure 4", "fig:multi-lookup-thp", "Lookup throughput of all test cases (16-threads)."),
    "single_thread_lat_overall": ("Figure 5", "fig:singletail", "Tail latency (99.9th) of all test cases (single-threaded)."),
    "multi_thread_lat_overall": ("Figure 6", "fig:multitail", "Tail latency (99.9th) of all test cases (16-threaded)."),
    "memory_consumption_overall": ("Figure 7", "fig:space", "Space overhead in all test cases (3200MB data)."),
    "sensitivity_impact_of_bulkload_size": ("Figure 8a", "fig:sensitivity-bulkloadsize", "Sensitivity to bulkload size."),
    "sensitivity_impact_of_insert_pattern": ("Figure 8b", "fig:figure3b", "Sensitivity to insert pattern."),
    "sensitivity_impact_of_sampling_method": ("Figure 8c", "fig:figure3c", "Sensitivity to sampling method."),
    "alex_read_throughput_change_ratio_under_oob": ("Figure 9", "fig:CDFoob", "CDF of lookup throughput change ratio after bulkloading with an outlier."),
    "lipp_overfit_bulkload": ("Figure 10a", "fig:lipp-overfit", "LIPP root model after construction (Covid)."),
    "lipp_overfit_insert": ("Figure 10b", "fig:lipp-overfit", "LIPP root model after insertions (Covid)."),
    "alex_overfit_bulkload": ("Figure 11a", "fig:alex-overfit", "ALEX root model after construction (Face)."),
    "alex_overfit_insert": ("Figure 11b", "fig:alex-overfit", "ALEX root model after insertions (Face)."),
    "alex_treeheight_cdf": ("Figure 12a", "fig:treeheightdistribution", "ALEX tree height distribution (OSM)."),
    "lipp_treeheight_cdf": ("Figure 12b", "fig:treeheightdistribution", "LIPP tree height distribution (OSM)."),
    "alex_cmp_cdf": ("Figure 13a", "fig:SMOheuristic", "ALEX leaf comparisons, sorted vs shuffled insert (Covid)."),
    "alex_depth_smo": ("Figure 13b", "fig:SMOheuristic", "ALEX tree height and SMO count (Covid)."),
    "memory_breakdown": ("Figure 14", "fig:spacebreakdown", "Breakdown of memory consumptions (Face)."),
    "alex_memory_1": ("Figure 15a", "fig:ALEX_Memory", "ALEX key count per 100K nodes (Face)."),
    "alex_memory_2": ("Figure 15b", "fig:ALEX_Memory", "ALEX node type distribution (Face)."),
    "alex_lipp_read_throughput_change_ratio_under_preload": ("Figure 16", "fig:dataaugmentation", "CDF of lookup throughput change ratio after bulkloading with data augmentation."),
    "alex_lipp_read_throughput_better_than_trad": ("Figure 17", "fig:reconstuction", "Case ratio (learned > traditional) in single-threaded lookup throughput."),
    "tab1_single_insert_worst": ("Table 1", "tab:single-insert-thp", "Insert throughput of the slowest insert case (single-threaded)."),
    "tab2_multi_insert_worst": ("Table 2", "tab:multi-insert-thp", "Insert throughput of the slowest insert case (16-threaded)."),
}
# Section 5 figures come from the profiling notebooks (plot.py --mode profiling)
PROFILING_FIGURES = {"lipp_overfit_bulkload", "lipp_overfit_insert", "alex_overfit_bulkload",
                     "alex_overfit_insert", "alex_treeheight_cdf", "lipp_treeheight_cdf", "alex_cmp_cdf",
                     "alex_depth_smo", "memory_breakdown", "alex_memory_1", "alex_memory_2"}

# Printed values of the paper's two tables, read from 4.1 single-thp.tex (lines
# 55-66) and 4.2 multi-thp.tex (lines 39-50).  ``None`` is the paper's CRASH.
PAPER_TABLE1 = {
    "dataset": ["linear", "covid", "fb-1", "osm"],
    "indexes": ["btree", "art", "alex", "lipp", "dytis", "dili"],
    "printed": {
        "btree": [1.32, 1.32, 1.31, 1.32],
        "art": [4.60, 2.60, 2.20, 2.42],
        "alex": [2.73, 2.11, 0.98, 1.22],
        "lipp": [0.469, 0.341, 0.425, 0.407],
        "dytis": [0.00152, 1.347, 0.00201, 0.142],
        "dili": [1.10, None, 0.832, 1.12],
    },
}
# Table 1 cells whose printed value comes from a run that hit run.py's 30-minute
# limit: the paper divides the keys inserted before the kill by the elapsed time.
# A killed run writes no CSV record, so the released CSVs cannot contain them.
TIMEOUT_DERIVED = {("dytis", "linear"), ("dytis", "fb-1")}
PAPER_TABLE2 = {
    "dataset": ["linear", "covid", "fb-1", "osm"],
    "indexes": ["btree", "btreeolc", "art", "artolc", "alex", "alexolc", "lipp", "sali", "finedex"],
    "printed": {
        "btree": [1.32, 1.32, 1.31, 1.32],
        "btreeolc": [17.0, 17.0, 17.0, 17.0],
        "art": [4.60, 2.60, 2.20, 2.42],
        "artolc": [45.2, 28.2, 25.2, 28.2],
        "alex": [2.73, 2.11, 0.98, 1.22],
        "alexolc": [1.34, 24.8, 1.30, 9.15],
        "lipp": [0.469, 0.341, 0.425, 0.407],
        "sali": [1.43, 0.73, 0.45, 3.99],
        "finedex": [12.4, 11.5, 9.10, 9.55],
    },
}


# ---------------------------------------------------------------- loading ---


def official(name: str) -> str:
    return OFFICIAL.get(name, name)


def phase_of(row: pd.Series) -> str:
    phase = row.get("measurement_phase")
    if isinstance(phase, str) and phase:
        return phase
    if float(row["read_ratio"]) > 0.9999:
        return "lookup"
    if float(row["insert_ratio"]) > 0.9999:
        # test_suite 10 "insert" throughput is derived from the bulkload
        # duration: counting it as an insert measurement is the bug this
        # column and this function exist to prevent.
        return "bulkload" if str(row["test_suite"]) == "10" else "insert"
    return "mixed"


def read_csv(path: Path, source: str, *, threads: int | None, tail: bool, oob: bool, table: str) -> pd.DataFrame:
    frame = pd.read_csv(path)
    frame = frame.loc[:, [c for c in frame.columns if not str(c).startswith("Unnamed")]]
    frame["source"] = source
    frame["dataset"] = frame["key_path"].map(lambda key: key.split("/")[-1])
    frame["suite"] = frame["test_suite"].astype(str)
    frame["bulkload_m"] = (frame["init_table_size"] / 1e6).round(6)
    frame["insert_pattern"] = frame["suite"].map(lambda s: "sorted" if s in ("10", "21", "41") else "shuffled")
    frame["sampling_method"] = frame["suite"].map(
        {"10": "full", "21": "uniform", "22": "uniform", "41": "segmented", "42": "segmented"}
    )
    frame["phase"] = frame.apply(phase_of, axis=1)
    frame["tail_lat"] = tail
    frame["oob"] = oob
    frame["table"] = table
    if threads is not None:
        frame = frame[frame["thread_num"] == threads]
    frame = frame[frame["phase"] != "mixed"]
    keep = [
        "source", "dataset", "index_type", "suite", "sampling_method", "insert_pattern",
        "bulkload_m", "thread_num", "tail_lat", "oob", "table", "phase", "throughput",
        "99.9 percentile", "memory_consumption", "table_size",
    ]
    return frame[[c for c in keep if c in frame.columns]]


# result file -> threads filter, tail-latency, outlier, table it feeds.  Only
# "thp"/"oob" files feed the throughput grids, "tail" files the latency grid and
# the memory file the space grid, otherwise one case would be counted twice.
SPECS = [
    ("single_thread_thp.csv", 1, False, False, "thp"),
    ("multi_thread_thp.csv", 16, False, False, "thp"),
    ("single_thread_tail_lat.csv", 1, True, False, "tail"),
    ("multi_thread_tail_lat.csv", 16, True, False, "tail"),
    ("memory_consumption.csv", None, False, False, "space"),
    ("single_thread_thp.alex_lipp_extended.csv", 1, False, False, "extended"),
    ("single_thread_thp.alex_oob.csv", 1, False, True, "oob"),
]


def load_frames(include_reference: bool, include_fresh: bool) -> pd.DataFrame:
    frames: list[pd.DataFrame] = []
    sources = ([("reference", REFERENCE)] if include_reference else []) + \
              ([("fresh", RESULTS)] if include_fresh else [])
    for source, directory in sources:
        for name, threads, tail, oob, table in SPECS:
            path = directory / name
            if path.exists() and path.stat().st_size:
                frame = read_csv(path, source, threads=threads, tail=tail, oob=oob, table=table)
                if not frame.empty:
                    frames.append(frame)
    if not frames:
        sys.exit("no measurements found; run artifact/data/fetch_reference.sh and/or artifact/run_small.sh")
    return pd.concat(frames, ignore_index=True)


def aggregate(frame: pd.DataFrame) -> pd.DataFrame:
    keys = ["source", "dataset", "index_type", "suite", "sampling_method", "insert_pattern",
            "bulkload_m", "thread_num", "tail_lat", "table", "oob", "phase"]
    return frame.groupby(keys, dropna=False).agg(
        throughput=("throughput", "mean"),
        throughput_min=("throughput", "min"),
        throughput_max=("throughput", "max"),
        p999=("99.9 percentile", "mean"),
        memory=("memory_consumption", "mean"),
        table_size=("table_size", "max"),
    ).reset_index()


# a case: one configuration, one phase, one result table
CASE_KEYS = ["dataset", "index_type", "suite", "bulkload_m", "thread_num", "tail_lat", "table", "oob", "phase"]


def same_cases(agg: pd.DataFrame) -> pd.DataFrame:
    """Rows of both sources whose case was measured by both."""
    ref = agg[agg["source"] == "reference"][CASE_KEYS].drop_duplicates()
    fresh = agg[agg["source"] == "fresh"][CASE_KEYS].drop_duplicates()
    common = ref.merge(fresh, on=CASE_KEYS)
    return agg.merge(common, on=CASE_KEYS)


def pct(new: float, base: float) -> float:
    return (new - base) / base * 100 if base else float("nan")


# ---------------------------------------------------------------- metrics ---


def lookup_grid(agg: pd.DataFrame, source: str, threads: int) -> pd.DataFrame:
    sel = agg[
        (agg["source"] == source)
        & (agg["thread_num"] == threads)
        & (agg["phase"] == "lookup")
        & (agg["table"] == "thp")
        & (agg["oob"] == False)  # noqa: E712
    ]
    return sel


def write_grid(agg: pd.DataFrame, source: str, threads: int) -> pd.DataFrame:
    return agg[
        (agg["source"] == source)
        & (agg["thread_num"] == threads)
        & (agg["phase"] == "insert")
        & (agg["table"] == "thp")
        & (agg["oob"] == False)  # noqa: E712
    ]


def tail_grid(agg: pd.DataFrame, source: str, threads: int) -> pd.DataFrame:
    return agg[(agg["source"] == source) & (agg["table"] == "tail") & (agg["thread_num"] == threads)]


def baseline_of(grid: pd.DataFrame, dataset: str, index: str) -> float | None:
    rows = grid[(grid["dataset"] == dataset) & (grid["index_type"] == index)
                & (grid["suite"] == "22") & (grid["bulkload_m"] == 100.0)]
    return float(rows["throughput"].iloc[0]) if not rows.empty else None


def share_slower(grid: pd.DataFrame, datasets: list[str], reference_index: str, learned: list[str]) -> tuple[float | None, str]:
    worse = total = 0
    for dataset in datasets:
        base = grid[(grid["dataset"] == dataset) & (grid["index_type"] == reference_index)]
        base = base.set_index(["suite", "bulkload_m"])["throughput"]
        if base.empty:
            continue
        for index in learned:
            sub = grid[(grid["dataset"] == dataset) & (grid["index_type"] == index)]
            for _, row in sub.iterrows():
                key = (row["suite"], row["bulkload_m"])
                if key not in base.index:
                    continue
                total += 1
                if row["throughput"] < float(base.loc[key]):
                    worse += 1
    return (worse / total * 100 if total else None), f"{worse}/{total} comparable cases"


def compute_metrics(agg: pd.DataFrame, source: str, datasets: list[str]) -> dict[str, float | None]:
    single = lookup_grid(agg, source, 1)
    multi = lookup_grid(agg, source, 16)
    single_w = write_grid(agg, source, 1)
    multi_w = write_grid(agg, source, 16)
    tail_single = tail_grid(agg, source, 1)
    tail_multi = tail_grid(agg, source, 16)
    # memory_consumption.csv only: tail-latency runs also record space on this
    # machine, but the released tail-latency CSVs carry 0 there
    space = agg[(agg["source"] == source) & (agg["table"] == "space") & (agg["memory"] > 0)].drop_duplicates(
        subset=["dataset", "index_type", "suite", "bulkload_m", "thread_num"]
    )
    metrics: dict[str, float | None] = {}
    notes: dict[str, str] = {}

    def has(grid: pd.DataFrame) -> bool:
        return bool(grid[grid["dataset"].isin(datasets)].shape[0])

    # Finding 1: peak-to-peak fluctuation of single-threaded lookup throughput
    for index in REPORT_SINGLE_INDEXES + ["dytis", "dili"]:
        worst = 0.0 if has(single) else None
        seen = False
        for dataset in datasets:
            sub = single[(single["dataset"] == dataset) & (single["index_type"] == index)]
            base = baseline_of(single, dataset, index)
            if sub.empty or not base:
                continue
            seen = True
            worst = max(
                worst,
                (sub["throughput"].max() - sub["throughput"].min()) / base * 100,
            )
        metrics[f"fluct_single:{index}"] = worst if seen else None

    # Finding 2: share of cases where learned indexes lose to ART / B+tree
    for reference_index in ("art", "btree"):
        value, note = share_slower(single, datasets, reference_index, SINGLE_LEARNED)
        metrics[f"worse_than_{reference_index}_single"] = value
        notes[f"worse_than_{reference_index}_single"] = note

    # Finding 3: full-dataset construction is not the best case for ALEX
    for dataset in ("linear", "fb-1", "osm"):
        sub = single[(single["dataset"] == dataset) & (single["index_type"] == "alex")]
        full = sub[sub["suite"] == "10"]["throughput"]
        if sub.empty or full.empty:
            metrics[f"full_build_gap:alex:{dataset}"] = None
            continue
        best = float(sub["throughput"].max())
        metrics[f"full_build_gap:alex:{dataset}"] = (best - float(full.iloc[0])) / best * 100

    # Finding 4: worst insert throughput of learned indexes vs B+tree
    for index in ("lipp",):
        worst = None
        for dataset in datasets:
            sub = single_w[(single_w["dataset"] == dataset) & (single_w["index_type"] == index)]
            base = single_w[(single_w["dataset"] == dataset) & (single_w["index_type"] == "btree")]
            if sub.empty or base.empty:
                continue
            ratio = pct(float(sub["throughput"].min()), float(base["throughput"].min()))
            worst = ratio if worst is None else min(worst, ratio)
        metrics[f"insert_worst_vs_btree:{index}"] = worst

    # Finding 5: multi-threaded lookup mirrors single-threaded patterns
    for index in ("alexolc", "sali"):
        value, note = share_slower(multi, datasets, "artolc", [index])
        metrics[f"worse_than_artolc_multi:{index}"] = value
        notes[f"worse_than_artolc_multi:{index}"] = note

    # Finding 6: worst-case multi-threaded insert throughput
    for index in ("alexolc", "sali"):
        worst = None
        for dataset in datasets:
            sub = multi_w[(multi_w["dataset"] == dataset) & (multi_w["index_type"] == index)]
            base = multi_w[(multi_w["dataset"] == dataset) & (multi_w["index_type"] == "btreeolc")]
            if sub.empty or base.empty:
                continue
            ratio = pct(float(sub["throughput"].min()), float(base["throughput"].min()))
            worst = ratio if worst is None else min(worst, ratio)
        metrics[f"insert_worst_vs_btreeolc:{index}"] = worst

    # Finding 7: tail latency magnitude, relative to the traditional index
    for index, reference_index, grid in (
        ("alex", "art", tail_single), ("lipp", "art", tail_single),
        ("alexolc", "artolc", tail_multi), ("sali", "artolc", tail_multi),
    ):
        sub = grid[(grid["index_type"] == index) & (grid["phase"] == "insert") & (grid["dataset"].isin(datasets))]
        base = grid[(grid["index_type"] == reference_index) & (grid["phase"] == "insert") & (grid["dataset"].isin(datasets))]
        if sub.empty or base.empty:
            metrics[f"tail_insert_amp:{index}"] = None
            continue
        metrics[f"tail_insert_amp:{index}"] = float(sub["p999"].max()) / float(base["p999"].max())
        notes[f"tail_insert_amp:{index}"] = f"worst {sub['p999'].max()/1000:.0f} us vs {official(reference_index)} {base['p999'].max()/1000:.0f} us"

    # Finding 8/9: space overhead
    subset = space[space["dataset"].isin(datasets)]
    lipp = subset[subset["index_type"] == "lipp"]["memory"]
    metrics["lipp_space_vs_data"] = float(lipp.max()) / DATA_BYTES if not lipp.empty else None
    btree = subset[subset["index_type"] == "btree"]["memory"]
    metrics["btree_space_fluctuation"] = pct(float(btree.max()), float(btree.min())) if not btree.empty else None
    alex_face = subset[(subset["index_type"] == "alex") & (subset["dataset"] == "fb-1")]
    normal = alex_face[alex_face["suite"] == "10"]["memory"]
    metrics["alex_face_space_blowup"] = (
        float(alex_face["memory"].max()) / float(normal.iloc[0]) if not alex_face.empty and not normal.empty else None
    )
    sali = subset[subset["index_type"] == "sali"]["memory"]
    metrics["sali_space_gb"] = float(sali.max()) / 1024 ** 3 if not sali.empty else None
    art = subset[subset["index_type"] == "art"]["memory"]
    metrics["art_vs_btree_space"] = (
        pct(float(art.max()), float(btree.min())) if not art.empty and not btree.empty else None
    )

    # Finding 10/11: sensitivity to a single workload change
    for dataset in ("linear", "covid"):
        sub = single[(single["dataset"] == dataset) & (single["index_type"] == "alex")]
        shuffled = sub[(sub["suite"] == "22") & (sub["bulkload_m"] == 100.0)]["throughput"]
        sorted_ = sub[(sub["suite"] == "21") & (sub["bulkload_m"] == 100.0)]["throughput"]
        metrics[f"alex_pattern_speedup:{dataset}"] = (
            float(shuffled.iloc[0]) / float(sorted_.iloc[0]) if not shuffled.empty and not sorted_.empty else None
        )
    for dataset in ("linear", "covid", "fb-1"):
        sub = single[(single["dataset"] == dataset) & (single["index_type"] == "lipp")]
        uniform = sub[(sub["suite"] == "22") & (sub["bulkload_m"] == 100.0)]["throughput"]
        segmented = sub[(sub["suite"] == "42") & (sub["bulkload_m"] == 100.0)]["throughput"]
        metrics[f"lipp_segmented_drop:{dataset}"] = (
            pct(float(segmented.iloc[0]), float(uniform.iloc[0])) if not uniform.empty and not segmented.empty else None
        )

    # Finding 12: out-of-bounds handling, paired on segmented sampling
    paired = (
        agg["phase"].eq("lookup")
        & agg["source"].eq(source)
        & agg["index_type"].eq("alex")
        & agg["suite"].isin(["41", "42"])
        & agg["bulkload_m"].between(1.0, 100.0)
        & agg["dataset"].isin(datasets)
    )
    oob = agg[paired & agg["oob"] & agg["table"].eq("oob")]
    plain = agg[paired & agg["table"].eq("thp")]
    merged = oob.merge(plain, on=["dataset", "index_type", "suite", "bulkload_m", "thread_num"],
                       suffixes=("_oob", "_plain"))
    if merged.empty:
        metrics.update({"oob_share_slower": None, "oob_share_halved": None, "oob_worst_ratio": None})
    else:
        ratio = merged["throughput_oob"] / merged["throughput_plain"]
        metrics["oob_share_slower"] = float((ratio < 1).mean()) * 100
        metrics["oob_share_halved"] = float((ratio < 0.5).mean()) * 100
        metrics["oob_worst_ratio"] = float(ratio.min()) * 100
        notes["oob_share_slower"] = f"{int((ratio < 1).sum())}/{len(ratio)} paired cases"
        notes["oob_share_halved"] = f"{int((ratio < 0.5).sum())}/{len(ratio)} paired cases"
    return metrics, notes


# --- published values, tolerances and wording -------------------------------

PAPER = {
    "fluct_single:btree": ("Finding 1: max lookup-throughput fluctuation of B+tree (single-thread)", 9.7, 12, "%"),
    "fluct_single:art": ("Finding 1: max lookup-throughput fluctuation of ART (single-thread)", 5.5, 12, "%"),
    "fluct_single:alex": ("Finding 1: max lookup-throughput fluctuation of ALEX (single-thread)", 84.4, 12, "%"),
    "fluct_single:lipp": ("Finding 1: max lookup-throughput fluctuation of LIPP (single-thread)", 120.4, 12, "%"),
    "fluct_single:dytis": ("Finding 1: max lookup-throughput fluctuation of DyTIS (single-thread)", 40.3, 12, "%"),
    "fluct_single:dili": ("Finding 1: max lookup-throughput fluctuation of DILI (single-thread)", 186, 20, "%"),
    "worse_than_art_single": ("Finding 2: learned indexes slower than ART (single-thread)", 44.3, 12, "%"),
    "worse_than_btree_single": ("Finding 2: learned indexes slower than B+tree (single-thread)", 10.9, 12, "%"),
    "full_build_gap:alex:linear": ("Finding 3: ALEX best-vs-full-build gap, Linear", 11.14, 12, "%"),
    "full_build_gap:alex:fb-1": ("Finding 3: ALEX best-vs-full-build gap, Face", 22.96, 12, "%"),
    "full_build_gap:alex:osm": ("Finding 3: ALEX best-vs-full-build gap, OSM", 2.89, 12, "%"),
    "insert_worst_vs_btree:lipp": ("Finding 4: worst-case insert throughput of LIPP vs B+tree", -74.2, 12, "%"),
    "worse_than_artolc_multi:alexolc": ("Finding 5: ALEX-OLC lookup slower than ART-OLC (16 threads)", 58.9, 12, "%"),
    "worse_than_artolc_multi:sali": ("Finding 5: SALI lookup slower than ART-OLC (16 threads)", 93.6, 12, "%"),
    "insert_worst_vs_btreeolc:alexolc": ("Finding 6: worst 16-thread insert of ALEX-OLC vs B+tree-OLC", None, 0, "%"),
    "insert_worst_vs_btreeolc:sali": ("Finding 6: worst 16-thread insert of SALI vs B+tree-OLC", None, 0, "%"),
    "tail_insert_amp:alex": ("Finding 7: ALEX single-thread insert tail amplification vs ART", None, 0, "x"),
    "tail_insert_amp:lipp": ("Finding 7: LIPP single-thread insert tail amplification vs ART", None, 0, "x"),
    "tail_insert_amp:alexolc": ("Finding 7: ALEX-OLC 16-thread insert tail amplification vs ART-OLC", None, 0, "x"),
    "tail_insert_amp:sali": ("Finding 7: SALI 16-thread insert tail amplification vs ART-OLC", None, 0, "x"),
    "lipp_space_vs_data": ("Finding 8: LIPP worst space overhead relative to the data (3200MB)", 10, 2, "x"),
    "btree_space_fluctuation": ("Finding 8: B+tree space fluctuation (segmented sampling)", 46.6, 15, "%"),
    "art_vs_btree_space": ("Finding 8: ART space overhead vs B+tree", 96.4, 20, "%"),
    "alex_face_space_blowup": ("Section 5.4: ALEX space blow-up on Face (outlier vs full build)", 6, 2, "x"),
    "sali_space_gb": ("Finding 9: SALI worst space overhead", 50, 15, "GB"),
    "alex_pattern_speedup:linear": ("Finding 10: ALEX shuffled/sorted insert speed-up, Linear", 3.1, 0.6, "x"),
    "alex_pattern_speedup:covid": ("Finding 10: ALEX shuffled/sorted insert speed-up, Covid", 2.0, 0.6, "x"),
    "lipp_segmented_drop:linear": ("Finding 10: LIPP drop under segmented sampling, Linear", -83.6, 15, "%"),
    "lipp_segmented_drop:covid": ("Finding 10: LIPP drop under segmented sampling, Covid", -51.3, 15, "%"),
    "lipp_segmented_drop:fb-1": ("Finding 10: LIPP drop under segmented sampling, Face", -23.6, 15, "%"),
    "oob_share_slower": ("Finding 12: ALEX cases whose lookup throughput drops with an outlier", 75, 15, "%"),
    "oob_share_halved": ("Finding 12: ALEX cases losing at least 50% lookup throughput", 20, 10, "%"),
    "oob_worst_ratio": ("Finding 12: worst lookup-throughput ratio with an outlier", 20, 12, "%"),
}
# claims whose paper wording is a lower/upper bound rather than a value
BOUNDS = {
    "oob_share_slower": {"lower": 75},
    "oob_share_halved": {"lower": 15},
    "oob_worst_ratio": {"upper": 60},
    "tail_insert_amp:alex": {"lower": 10},
    "tail_insert_amp:lipp": {"lower": 10},
    "tail_insert_amp:alexolc": {"lower": 20},
    "tail_insert_amp:sali": {"lower": 20},
}


def make_check(claim: str, measured, published, tol, unit, notes="", lower=None, upper=None) -> dict:
    ok = None
    delta = None
    if isinstance(published, (int, float)) and measured is not None and measured == measured:
        ok = abs(measured - published) <= tol
        delta = measured - published
    if measured is not None and measured == measured and (lower is not None or upper is not None):
        ok = (lower is None or measured >= lower) and (upper is None or measured <= upper)
    return {
        "claim": claim,
        "measured": None if measured is None or measured != measured else round(float(measured), 4),
        "published": published,
        "tolerance": tol,
        "unit": unit,
        "within_tolerance": None if ok is None else bool(ok),
        "delta": None if delta is None else round(float(delta), 4),
        "notes": notes,
    }


def reference_checks(metrics: dict, notes: dict) -> list[dict]:
    checks = []
    for key, (claim, published, tol, unit) in PAPER.items():
        bounds = BOUNDS.get(key, {})
        checks.append(make_check(claim, metrics.get(key), published, tol, unit,
                                 notes=notes.get(key, ""), **bounds))
    return checks


def machine_factor(common: pd.DataFrame) -> tuple[float | None, int]:
    """Median throughput ratio (your results / released) over the cases both measured.

    Shown as context only: absolute throughput depends on the CPU, the claims
    are relative (learned vs traditional, best vs worst case), so no verdict is
    scaled by it.  Space and latency cases are left out.
    """
    rows = common[common["table"].isin(["thp", "oob", "extended"])]
    merged = rows[rows["source"] == "fresh"].merge(
        rows[rows["source"] == "reference"], on=CASE_KEYS, suffixes=("_fresh", "_ref"))
    merged = merged[merged["throughput_ref"] > 0]
    if merged.empty:
        return None, 0
    return float((merged["throughput_fresh"] / merged["throughput_ref"]).median()), len(merged)


def fresh_checks(metrics_fresh: dict, metrics_released: dict) -> list[dict]:
    """Each metric on your results vs the same metric on the released measurements of the same cases.

    Within 20% means relative to the released value (as Tables 1-2); a share of
    0% must stay 0%.
    """
    checks = []
    for key, (claim, _published, _tol, unit) in PAPER.items():
        measured, expected = metrics_fresh.get(key), metrics_released.get(key)
        if measured is None or expected is None or measured != measured or expected != expected:
            continue
        check = make_check(f"[fresh] {claim}", measured, round(float(expected), 4), abs(float(expected)) * 0.2, unit)
        checks.append(check)
    return checks


def paper_tables(agg: pd.DataFrame, datasets: list[str]) -> list[str]:
    """Markdown reproduction of the paper's two printed tables.

    Per cell: the value printed in the paper, the value recomputed from the
    released CSVs (the slowest released insert case) and, once
    ``artifact/results/`` holds that very case, your measurement of it.  The
    verdict compares the released CSVs with the paper.
    """
    single_thread_indexes = set(PAPER_TABLE1["indexes"])
    has_results = not (write_grid(agg, "fresh", 1).empty and write_grid(agg, "fresh", 16).empty)
    lines: list[str] = []
    for spec, threads, note in ((PAPER_TABLE1, 1, "single-threaded"), (PAPER_TABLE2, 16, "16-threaded")):
        stem = "tab1_single_insert_worst" if threads == 1 else "tab2_multi_insert_worst"
        number, label, caption = PAPER_MAP[stem]
        grids = {t: write_grid(agg, "reference", t) for t in (1, 16)}
        fresh_grids = {t: write_grid(agg, "fresh", t) for t in (1, 16)}
        intro = (f"Slowest insert case per dataset/index ({note}): the value printed in the paper "
                 "and the value recomputed from the released CSVs")
        if has_results:
            intro += (", and your measurement of that same case (`artifact/results/`, unscaled; "
                      "`-` = you have not run it)")
        lines += ["", f"### {number} - {caption}", "", intro + ".", "",
                  "| dataset / index | paper (Mops) | released CSVs |"
                  + (" your result, same case |" if has_results else "") + " verdict (released vs paper) |",
                  "| --- | --- | --- |" + (" --- |" if has_results else "") + " --- |"]
        tallies = {"yes": 0, "no": 0, "absent": 0, "crash": 0, "timeout": 0}
        for position, dataset in enumerate(spec["dataset"]):
            if dataset not in datasets:
                continue
            for index in spec["indexes"]:
                printed = spec["printed"][index][position]
                repeats_single = threads == 16 and index in single_thread_indexes
                source_thread = 1 if repeats_single else threads
                rows = grids[source_thread]
                rows = rows[(rows["dataset"] == dataset) & (rows["index_type"] == index)]
                recomputed = rerun = None
                if not rows.empty:
                    slowest = rows.loc[rows["throughput"].idxmin()]
                    recomputed = slowest["throughput"] / 1e6
                    mine = fresh_grids[source_thread]
                    mine = mine[(mine["dataset"] == dataset) & (mine["index_type"] == index)
                                & (mine["suite"] == slowest["suite"]) & (mine["bulkload_m"] == slowest["bulkload_m"])]
                    rerun = float(mine["throughput"].iloc[0]) / 1e6 if not mine.empty else None
                if printed is None:
                    verdict = "n/a - the paper prints CRASH"
                    tallies["crash"] += 1
                elif threads == 1 and (index, dataset) in TIMEOUT_DERIVED:
                    verdict = ("timeout: printed value = keys inserted before the 30-minute "
                               "limit / elapsed time; a killed run writes no CSV record")
                    tallies["timeout"] += 1
                elif recomputed is None:
                    verdict = "no measurement in the released CSVs to compare"
                    tallies["absent"] += 1
                else:
                    delta = abs(recomputed - printed) / printed * 100
                    if delta <= 20:
                        verdict = f"yes ({delta:.0f}%)"
                        tallies["yes"] += 1
                    else:
                        verdict = f"NO: released CSVs give {recomputed:.3g}"
                        tallies["no"] += 1
                    if repeats_single:
                        verdict += "; single-threaded case"
                cell = lambda value: "-" if value is None else f"{value:.3g}"  # noqa: E731
                lines.append(f"| {official('datasets/' + dataset)} / {official(index)} | "
                             f"{cell(printed)} | {cell(recomputed)} |"
                             + (f" {cell(rerun)} |" if has_results else "") + f" {verdict} |")
        total = tallies["yes"] + tallies["no"]
        lines += ["", f"Cells reproducing within 20%: {tallies['yes']}/{total}"
                      + (f"; {tallies['no']} do not" if tallies["no"] else "")
                      + (f"; {tallies['absent']} have no released measurement to compare against"
                         if tallies["absent"] else "")
                      + (f"; {tallies['crash']} are CRASH" if tallies["crash"] else "")
                      + (f"; {tallies['timeout']} are derived from a timed-out run (not in the CSVs)"
                         if tallies["timeout"] else "") + "."]
    lines += ["", "Rows marked `single-threaded case` are cells the paper repeats from its "
              "single-threaded table; the released multi-thread CSVs hold only the OLC indexes, "
              "SALI and FINEdex, so those cells are cross-checked against the single-threaded run."]
    return lines


def paper_map_lines() -> list[str]:
    lines = ["", "## Paper figures and tables", "",
             "| paper | paper label | file | content |", "| --- | --- | --- | --- |"]
    for stem, (number, label, caption) in PAPER_MAP.items():
        if number.startswith("Table"):
            target = "this file, below"
        else:
            target = f"`artifact/figures/{'profiling' if stem in PROFILING_FIGURES else 'paper'}/{stem}.pdf`"
        lines.append(f"| {number} | `{label}` | {target} | {caption} |")
    return lines


def render_summary(reference: list[dict], fresh: list[dict], selected_meta: dict,
                   path_md: Path, path_json: Path, paper_block: list[str] | None = None) -> None:
    path_json.write_text(json.dumps(
        {"checks": reference, "fresh_checks": fresh, "selected_cases": selected_meta}, indent=1
    ) + "\n")
    lines = ["# RoBin: numbers quoted in the paper, recomputed", "",
             "Written by `artifact/lib/analyze.py` (run by every entry point). Each row recomputes "
             "one number the paper's text quotes from the released measurements "
             "(`artifact/reference/`) and compares it with the printed value. This checks that "
             "the scripts compute what the paper prints; it is not a measurement of your machine. "
             "Once `artifact/results/` holds your runs, a second table compares them with the "
             "released measurements of the same cases.", "",
             "## Released measurements vs the paper", "",
             "| Claim | Recomputed | Printed | Unit | Within tolerance | Notes |",
             "| --- | --- | --- | --- | --- | --- |"]
    for item in reference:
        measured = "-" if item["measured"] is None else f"{item['measured']}"
        published = "-" if item["published"] is None else f"{item['published']}"
        ok = {True: "yes", False: "NO", None: "n/a"}[item["within_tolerance"]]
        lines.append(f"| {item['claim']} | {measured} | {published} | {item['unit']} | {ok} | {item['notes']} |")
    evaluated = [c for c in reference if c["within_tolerance"] is not None]
    reproduced = sum(1 for c in evaluated if c["within_tolerance"] is True)
    mismatched = [c for c in reference if c["within_tolerance"] is False]
    lines += ["",
              f"{reproduced}/{len(evaluated)} recomputed numbers match the printed ones; "
              f"{len(reference) - len(evaluated)} rows have no single printed number (informational); "
              f"{len(mismatched)} mismatch."]
    if mismatched:
        lines += ["", "Outside tolerance:"]
        lines += [f"* {c['claim']}: recomputed {c['measured']} vs printed {c['published']} {c['unit']}"
                  for c in mismatched]
    if fresh:
        lines += ["", "## Your results vs the released measurements, same cases", ""]
        if selected_meta.get("fresh_note"):
            lines += [selected_meta["fresh_note"], ""]
        lines += ["| Claim | Your results | Released, same cases | Unit | Within 20% |",
                  "| --- | --- | --- | --- | --- |"]
        for item in fresh:
            ok = {True: "yes", False: "NO", None: "n/a"}[item["within_tolerance"]]
            lines.append(f"| {item['claim'].replace('[fresh] ', '')} | {item['measured']} | "
                         f"{item['published']} | {item['unit']} | {ok} |")
        fresh_evaluated = [c for c in fresh if c["within_tolerance"] is not None]
        fresh_ok = sum(1 for c in fresh_evaluated if c["within_tolerance"] is True)
        lines += ["", f"{fresh_ok}/{len(fresh_evaluated)} of your results are within 20% of the released "
                      "value of the same cases. A row outside it is worth a look in the figures, "
                      "not a failed claim by itself: the claims are the trends of §1."]
    elif selected_meta.get("fresh_note"):
        lines += ["", selected_meta["fresh_note"]]
    if paper_block:
        lines += paper_block
    path_md.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--reference-only", action="store_true")
    parser.add_argument("--fresh-only", action="store_true")
    args = parser.parse_args()

    frame = load_frames(include_reference=not args.fresh_only, include_fresh=not args.reference_only)
    agg = aggregate(frame)

    cases_path = HERE / "cases.json"
    selected_meta: dict = {"runs": 0, "per_experiment": {}}
    if cases_path.exists():
        config = json.loads(cases_path.read_text())
        selected_meta["runs"] = len(config["cases"])
        for tier in ("claims", "figures", "core", "extended"):
            selected_meta[f"tier_{tier}"] = sum(1 for c in config["cases"] if tier in c["roles"])
        for case in config["cases"]:
            for experiment in case["experiments"]:
                selected_meta["per_experiment"][experiment] = selected_meta["per_experiment"].get(experiment, 0) + 1

    reference_metrics, reference_notes = compute_metrics(agg, "reference", DATASETS)
    checks = reference_checks(reference_metrics, reference_notes)

    fresh_rows = agg[agg["source"] == "fresh"]
    fresh_verdicts: list[dict] = []
    fresh_note = None
    if not fresh_rows.empty:
        full_scale = float(fresh_rows["table_size"].max()) >= FULL_SIZE
        datasets = [d for d in DATASETS if d in set(fresh_rows["dataset"])]
        indexes = sorted(set(fresh_rows["index_type"]))
        if not full_scale:
            fresh_note = (
                "Measurements in `artifact/results/` were taken on a reduced dataset "
                f"({int(fresh_rows['table_size'].max()):,} keys); "
                "they are reported but not compared against the paper."
            )
        else:
            common = same_cases(agg)
            fresh_metrics, _ = compute_metrics(common, "fresh", datasets)
            released_metrics, _ = compute_metrics(common, "reference", datasets)
            fresh_verdicts = fresh_checks(fresh_metrics, released_metrics)
            factor, pairs = machine_factor(common)
            cases = len(common[common["source"] == "fresh"][CASE_KEYS].drop_duplicates())
            selected_meta["machine_factor"] = None if factor is None else round(factor, 3)
            selected_meta["machine_factor_pairs"] = pairs
            fresh_note = (
                f"Both columns are computed over the same {cases} measured cases (datasets "
                f"{', '.join(datasets)}; indexes {', '.join(indexes)}), so a row covers only the cases "
                "you ran, not the paper's whole grid. The values are relative (shares, ratios, "
                "space), so they are compared unscaled."
                + (f" For context, your throughput is {factor:.2f}x the released one "
                   f"(median over {pairs} throughput measurements of those cases)." if factor else "")
            )
    if fresh_note:
        selected_meta["fresh_note"] = fresh_note

    paper_block: list[str] = []
    selected_meta["paper_map"] = {stem: {"number": number, "label": label, "caption": caption}
                                  for stem, (number, label, caption) in PAPER_MAP.items()}
    paper_block += paper_map_lines()
    paper_block += paper_tables(agg, DATASETS)
    render_summary(checks, fresh_verdicts, selected_meta, HERE / "SUMMARY.md", HERE / "summary.json",
                   paper_block=paper_block)

    evaluated = [c for c in checks if c["within_tolerance"] is not None]
    ok = sum(1 for c in evaluated if c["within_tolerance"] is True)
    bad = [c for c in checks if c["within_tolerance"] is False]
    print(f"reference claims: {ok}/{len(evaluated)} within tolerance "
          f"({len(checks) - len(evaluated)} informational), {len(bad)} mismatch")
    for item in bad:
        print(f"  OUT OF TOLERANCE: {item['claim']}: {item['measured']} vs {item['published']} {item['unit']}")
    if fresh_verdicts:
        fresh_eval = [c for c in fresh_verdicts if c["within_tolerance"] is not None]
        fresh_ok = sum(1 for c in fresh_eval if c["within_tolerance"] is True)
        print(f"your results vs released, same cases: {fresh_ok}/{len(fresh_eval)} within 20%")
    print(f"summary -> {HERE / 'SUMMARY.md'}")


if __name__ == "__main__":
    main()
