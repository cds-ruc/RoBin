"""Shared layout of the RoBin artifact: directories, experiments, result files.

Every result is written under the file name the paper's notebooks read
(``single_thread_thp.csv``, ``multi_thread_tail_lat.csv``, ...), into one
directory per source:

* ``artifact/reference/`` - the paper's released measurements (``conf_rel``);
* ``artifact/results/``   - measurements taken on this machine.

``python3 artifact/lib/layout.py derive-space [DIR]`` writes
``memory_consumption.csv`` from the throughput files of DIR.
"""
from __future__ import annotations

import csv
import os
import sys
from pathlib import Path

ARTIFACT = Path(__file__).resolve().parents[1]
ROOT = ARTIFACT.parent
REFERENCE = ARTIFACT / "reference"
# ROBIN_RESULTS redirects this machine's results (used by the smoke test)
RESULTS = Path(os.environ.get("ROBIN_RESULTS", ARTIFACT / "results"))
FIGURES = ARTIFACT / "figures"
NOTEBOOKS = ARTIFACT / "plot" / "notebooks"

# experiment directory -> paper items and the notebooks that draw them
EXPERIMENTS = {
    "fig03_tab1_single_thread": {
        "paper": "Fig. 3, Table 1 (Findings 1-4)",
        "notebooks": ["single_thread.ipynb"],
    },
    "fig04_tab2_multi_thread": {
        "paper": "Fig. 4, Table 2 (Findings 5-6)",
        "notebooks": ["multi_thread.ipynb"],
    },
    "fig05_06_tail_latency": {
        "paper": "Fig. 5, Fig. 6 (Finding 7)",
        "notebooks": ["single_thread_tail_lat.ipynb", "multi_thread_tail_lat.ipynb"],
    },
    "fig07_space": {
        "paper": "Fig. 7 (Findings 8-9)",
        "notebooks": ["memory_consumption.ipynb"],
    },
    "fig08_sensitivity": {
        "paper": "Fig. 8a-c (Findings 10-11)",
        "notebooks": ["single_thread.ipynb"],
    },
    "fig09_out_of_bound": {
        "paper": "Fig. 9 (Finding 12)",
        "notebooks": ["analysis_alex_oob.ipynb"],
    },
    "fig10_15_root_cause": {
        "paper": "Fig. 10-15 (Section 5)",
        "notebooks": [],
    },
    "fig16_data_augmentation": {
        "paper": "Fig. 16 (Section 6.1)",
        "notebooks": ["analysis_preload.ipynb"],
    },
    "fig17_reconstruction": {
        "paper": "Fig. 17 (Section 6.2)",
        "notebooks": ["analysis_bulkload_variance.ipynb"],
    },
}

SINGLE_THP = "single_thread_thp.csv"
MULTI_THP = "multi_thread_thp.csv"
SPACE = "memory_consumption.csv"
EXTENDED = "single_thread_thp.alex_lipp_extended.csv"
# bulkload sizes of the main grid (reproduce.sh test_ro_thp_*); other sizes are
# the extended sweep of Fig. 17 (test_ro_thp_single_thread_extended_bulksize)
MAIN_BULKLOAD_SIZES = {0, 1_000_000, 2_000_000, 5_000_000, 10_000_000, 20_000_000,
                       50_000_000, 100_000_000, 200_000_000}
PRELOAD_FILES = {
    33: "single_thread_thp.alex_lipp_preload_1_100_uint64.csv",
    34: "single_thread_thp.alex_lipp_preload_1_100_dataset.csv",
}
FAIL_LOGS = {1: "single_thread_fail_case.log", 16: "multi_thread_fail_case.log"}

# columns that identify one configuration inside a result file
CONFIG_COLUMNS = ["key_path", "index_type", "init_table_size", "test_suite", "thread_num", "preload_suite"]


def target_file(case: dict) -> str:
    """Paper file name a case's records belong to."""
    threads = int(case["concurrency"])
    if case.get("tail_lat"):
        return "single_thread_tail_lat.csv" if threads == 1 else "multi_thread_tail_lat.csv"
    if case.get("out_of_bound"):
        return "single_thread_thp.alex_oob.csv"
    if case.get("preload_suite"):
        return PRELOAD_FILES[int(case["preload_suite"])]
    if threads == 1 and int(case["bulkload_size"]) not in MAIN_BULKLOAD_SIZES:
        return EXTENDED
    return SINGLE_THP if threads == 1 else MULTI_THP


def derive_space(directory: Path) -> Path | None:
    """``memory_consumption.csv`` = the records of both throughput files.

    Every run is started with ``--memory``, so each throughput record carries
    the index size; the space figure is the union of the single- and
    16-threaded throughput runs.
    """
    rows, header = [], None
    for name in (SINGLE_THP, MULTI_THP):
        path = directory / name
        if not path.exists():
            continue
        with path.open(newline="") as handle:
            reader = csv.DictReader(handle)
            header = header or reader.fieldnames
            rows += list(reader)
    if header is None:
        return None
    out = directory / SPACE
    with out.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=header, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)
    return out


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "derive-space":
        written = derive_space(Path(sys.argv[2]) if len(sys.argv) > 2 else RESULTS)
        print(f"wrote {written}" if written else "no throughput files to derive space from")
    else:
        sys.exit(__doc__)
