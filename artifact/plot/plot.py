#!/usr/bin/env python3
"""Draw every paper figure with the paper's own plotting notebooks.

There is exactly one plotting implementation: the paper's notebooks, copied
into ``artifact/plot/notebooks/{results,profiling}/``.  This driver only stages
their input files into a scratch working directory and executes their code
cells there, so the style of every output is the paper's.

Modes
-----
``paper``      the released measurements (``artifact/reference/``, exported from
               the ``conf_rel`` branch by ``artifact/data/fetch_reference.sh``).
               Regenerates the figures as printed in the paper; about a minute.
``results``    the measurements of this machine (``artifact/results/``).  No
               released row is mixed in.  Every figure is stamped with the
               configurations and datasets it was drawn from; ``MANIFEST.md``
               lists per notebook which figures the results cover so far.
``profiling``  the Section 5 root-cause notebooks on the logs written by
               ``run_case_profiling.sh``.

Outputs go to ``artifact/figures/<mode>/`` under the file names the paper uses.

Usage::

    python3 artifact/plot/plot.py --mode paper
    python3 artifact/plot/plot.py --mode results [--experiments fig09_out_of_bound]
    python3 artifact/plot/plot.py --mode profiling --log-dir artifact/results/profiling_log
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shutil
import sys
import tempfile
import traceback
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.figure  # noqa: E402
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "lib"))
import layout  # noqa: E402

REFERENCE = layout.REFERENCE
RESULTS = layout.RESULTS
FIGURES = layout.FIGURES
RESULT_NOTEBOOK_DIR = layout.NOTEBOOKS / "results"
PROFILING_NOTEBOOK_DIR = layout.NOTEBOOKS / "profiling"
PROFILING_LOG = RESULTS / "profiling_log"

# notebook -> CSV files it reads (relative to its working directory)
RESULT_NOTEBOOKS = {
    "single_thread.ipynb": ["single_thread_thp.csv"],
    "multi_thread.ipynb": ["multi_thread_thp.csv"],
    "single_thread_tail_lat.ipynb": ["single_thread_tail_lat.csv"],
    "multi_thread_tail_lat.ipynb": ["multi_thread_tail_lat.csv"],
    "memory_consumption.ipynb": ["memory_consumption.csv"],
    "analysis_alex_oob.ipynb": ["single_thread_thp.alex_oob.csv", "single_thread_thp.csv"],
    "analysis_preload.ipynb": [
        "single_thread_thp.alex_lipp_preload_1_100_uint64.csv",
        "single_thread_thp.alex_lipp_preload_1_100_dataset.csv",
        "single_thread_thp.csv",
    ],
    "analysis_bulkload_variance.ipynb": ["single_thread_thp.csv", "single_thread_thp.alex_lipp_extended.csv"],
}
PROFILING_NOTEBOOKS = [
    "analysis_overfit.ipynb",
    "analysis_depth.ipynb",
    "analysis_smo.ipynb",
    "analysis_memory.ipynb",
]

# notebook output name -> file name used by the paper (robin-paper/figures/)
PAPER_NAMES = {
    "single_thread_overall.pdf": "single_thread_overall_thp.pdf",
    "multi_thread_overall.pdf": "multi_thread_overall_thp.pdf",
    "single_thread_impact_of_bulkload_size.pdf": "sensitivity_impact_of_bulkload_size.pdf",
    "single_thread_impact_of_insert_pattern.pdf": "sensitivity_impact_of_insert_pattern.pdf",
    "single_thread_impact_of_sampling_method.pdf": "sensitivity_impact_of_sampling_method.pdf",
    "lipp_model_dist_bulkload.pdf": "lipp_overfit_bulkload.pdf",
    "lipp_model_dist_insert.pdf": "lipp_overfit_insert.pdf",
    "alex_depth_cdf.pdf": "alex_treeheight_cdf.pdf",
    "lipp_depth_cdf.pdf": "lipp_treeheight_cdf.pdf",
}

CONFIG_KEYS = ["key_path", "index_type", "init_table_size", "test_suite", "thread_num"]


# ------------------------------------------------------------------ staging --


def stage_results(stage: Path) -> None:
    """Copy this machine's result files; a file not produced yet is staged empty."""
    if not (RESULTS / layout.SPACE).exists():
        layout.derive_space(RESULTS)
    for name in sorted({name for names in RESULT_NOTEBOOKS.values() for name in names}):
        source = RESULTS / name
        if source.exists():
            shutil.copy2(source, stage / name)
        else:
            pd.read_csv(REFERENCE / name, nrows=0).to_csv(stage / name, index=False)
    for name in layout.FAIL_LOGS.values():
        if (RESULTS / name).exists():
            shutil.copy2(RESULTS / name, stage / name)


def stage_paper(stage: Path) -> None:
    for name in sorted({name for names in RESULT_NOTEBOOKS.values() for name in names}):
        source = REFERENCE / name
        if not source.exists():
            raise SystemExit(f"missing {source}; run artifact/data/fetch_reference.sh first")
        shutil.copy2(source, stage / name)


def configurations(path: Path) -> set[tuple]:
    if not path.exists():
        return set()
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    keys = CONFIG_KEYS + (["preload_suite"] if rows and "preload_suite" in rows[0] else [])
    return {tuple(row.get(key) for key in keys) for row in rows}


# ---------------------------------------------------------------- execution --


def code_cells(path: Path) -> list[str]:
    payload = json.loads(path.read_text())
    return ["".join(cell.get("source", [])) for cell in payload["cells"] if cell.get("cell_type") == "code"]


DATASETS_LITERAL = re.compile(r"^(\s*)datasets\s*=\s*\[([^\]]*)\]", re.M)


def with_datasets(source: str, names: list[str]) -> str:
    return DATASETS_LITERAL.sub(lambda m: f"{m.group(1)}datasets = {names!r}", source, count=1)


def complete_groups(frame: pd.DataFrame) -> pd.DataFrame:
    """Keep the (dataset, index) groups that hold the baseline and the full build.

    The overall figures pair every index's baseline <uniform, 100M, shuffled>
    with its <full, 200M> case by position, so a group missing either one would
    shift the pairing of every later index.
    """
    keep = []
    for _, group in frame.groupby(["key_path", "index_type"]):
        baseline = ((group["test_suite"] == 22) & (group["init_table_size"] == 100_000_000)).any()
        full = (group["test_suite"] == 10).any()
        if baseline and full:
            keep.append(group)
    return pd.concat(keep) if keep else frame.iloc[0:0]


def restrict_to_present(source: str, present: set[str]) -> str:
    """Results mode: plot only what was measured.

    * indexes without a measured row are dropped from the cell's ``index_types``;
    * the overall figures keep only (dataset, index) groups holding both the
      baseline and the full-build case they draw as reference lines;
    * Fig. 17 leaves out the bulkload fractions you have not measured (the
      notebook would draw them as 0%) and marks the measured ones.

    The paper mode never calls this.
    """
    if "savefig" not in source:
        return source

    def keep(match: re.Match) -> str:
        names = re.findall(r"[\"']([^\"']+)[\"']", match.group(1))
        return "index_types = " + repr([name for name in names if name in present])

    source = re.sub(r"index_types\s*=\s*\[([^\]]*)\]", keep, source)
    source = source.replace(
        'df_dataset = df_new[df_new["key_path"] == dataset]',
        'df_dataset = df_new[(df_new["key_path"] == dataset) & df_new["index_type"].isin(index_types)]',
    )
    source = source.replace(
        'df_dataset = transformed_df[(transformed_df["key_path"] == dataset)',
        'df_dataset = transformed_df[(transformed_df["key_path"] == dataset) & '
        'transformed_df["index_type"].isin(index_types)',
    )
    # hard-coded CRASH annotations of the released DILI cases
    if "dili" not in present:
        source = re.sub(r"\n    # dili crash.*?(?=\n# Create legend)", "", source, flags=re.S)
    frame = re.search(r"df_dataset = (\w+)\[\(?\1\[\"key_path\"\]", source)
    if "read_optimal =" in source and frame:
        name = frame.group(1)
        source = (f"__ae_full = {name}\n{name} = __ae_complete({name})\ntry:\n"
                  + "".join("    " + line + "\n" for line in source.splitlines())
                  + f"finally:\n    {name} = __ae_full\n")
    if "1.5*btree_mean_read_throughput" in source:
        source = source.replace("ratios.append(0)", "ratios.append(float('nan'))")
        source = source.replace("label=get_official_name(key_path))", "label=get_official_name(key_path), marker='o')")
    return source


def run_notebook(path: Path, workdir: Path, transform=None, stamp: str | None = None,
                 probe_datasets: bool = False) -> list[str]:
    """Execute the notebook's code cells in ``workdir``; return per-cell notes.

    ``probe_datasets`` (results mode): a figure cell that loops over a literal
    ``datasets = [...]`` list is first tried with each dataset alone, without
    saving, and then drawn with the datasets whose measured cases cover
    everything the cell reads.  The figure is stamped with that dataset list.
    """
    notes: list[str] = []
    original_show, original_savefig = plt.show, matplotlib.figure.Figure.savefig
    state = {"save": True, "stamp": stamp}

    def savefig(figure, *args, **kwargs):
        if not state["save"]:
            return None
        if state["stamp"]:
            figure.text(0.99, 0.005, state["stamp"], ha="right", va="bottom", fontsize=7, color="#555555")
        return original_savefig(figure, *args, **kwargs)

    def execute(source: str, namespace: dict, label: str) -> None:
        try:
            exec(compile(source, label, "exec"), namespace)
        finally:
            plt.close("all")

    old_cwd = Path.cwd()
    os.chdir(workdir)
    plt.show = lambda *args, **kwargs: None
    matplotlib.figure.Figure.savefig = savefig
    namespace = {"__name__": "__main__", "display": print, "__ae_complete": complete_groups}
    try:
        for number, source in enumerate(code_cells(path)):
            label = f"{path.name}:cell{number}"
            if transform:
                source = transform(source)
            literal = DATASETS_LITERAL.search(source) if probe_datasets and "savefig" in source else None
            if literal:
                names = re.findall(r"[\"']([^\"']+)[\"']", literal.group(2))

                def works(subset: list[str]) -> bool:
                    trial = {key: value.copy() if isinstance(value, pd.DataFrame) else value
                             for key, value in namespace.items()}
                    try:
                        execute(with_datasets(source, subset), trial, label)
                        return True
                    except Exception:  # noqa: BLE001 - a dataset lacks a case this figure reads
                        return False

                state["save"] = False
                # some figures (the Fig. 8a heatmap) need all their datasets at once
                usable = names if works(names) else [name for name in names if works([name])]
                state["save"] = True
                if not usable:
                    notes.append(f"cell {number}: not drawn yet, its cases are not in artifact/results")
                    continue
                source = with_datasets(source, usable)
                shown = ", ".join(name.split("/")[-1] for name in usable)
                state["stamp"] = f"{stamp}; datasets: {shown}"
                if len(usable) < len(names):
                    notes.append(f"cell {number}: drawn for {shown} only")
            missing_log = re.search(r"open\(\s*['\"](\w+_fail_case\.log)['\"]", source)
            if missing_log and not (workdir / missing_log.group(1)).exists():
                # run.py's "Failed command:" lines were never released with the paper's CSVs
                notes.append(f"cell {number}: failed-case table skipped, {missing_log.group(1)} not released")
                continue
            try:
                execute(source, namespace, label)
            except Exception as error:  # noqa: BLE001 - a failing cell must not hide the others
                notes.append(f"cell {number}: {type(error).__name__}: {error}")
                if os.environ.get("AE_PLOT_DEBUG"):
                    traceback.print_exc()
            finally:
                state["stamp"] = stamp
    finally:
        plt.show, matplotlib.figure.Figure.savefig = original_show, original_savefig
        os.chdir(old_cwd)
    return notes


def collect(figdir: Path, destination: Path) -> list[str]:
    names = []
    for path in sorted(figdir.glob("*.pdf")):
        target = PAPER_NAMES.get(path.name, path.name)
        shutil.copy2(path, destination / target)
        names.append(target)
    for path in figdir.glob("*.pdf"):
        path.unlink()
    return names


def fresh_dir(path: Path) -> Path:
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True)
    return path


# -------------------------------------------------------------------- modes --


def run_result_notebooks(mode: str, destination: Path, notebooks: list[str]) -> list[str]:
    stage = Path(tempfile.mkdtemp(prefix=f"robin-figures-{mode}-"))
    try:
        return _run_result_notebooks(mode, stage, destination, notebooks)
    finally:
        shutil.rmtree(stage, ignore_errors=True)


def _run_result_notebooks(mode: str, stage: Path, destination: Path, notebooks: list[str]) -> list[str]:
    (stage / "fig").mkdir()
    (stage_paper if mode == "paper" else stage_results)(stage)

    lines = []
    for name in notebooks:
        inputs = RESULT_NOTEBOOKS[name]
        transform, stamp, coverage = None, None, ""
        if mode == "results":
            present = set()
            measured_total, released_total = 0, 0
            # the figure's own inputs; single_thread_thp.csv is only the
            # comparison baseline when it is not listed first (outlier / augmentation)
            own = inputs if inputs[0] == "single_thread_thp.csv" else \
                [csv_name for csv_name in inputs if csv_name != "single_thread_thp.csv"]
            empty = []
            for csv_name in own:
                measured = len(configurations(stage / csv_name))
                measured_total += measured
                released_total += len(configurations(REFERENCE / csv_name))
                if measured == 0:
                    empty.append(csv_name)
            for csv_name in inputs:
                frame = pd.read_csv(stage / csv_name)
                present |= set(frame["index_type"]) if "index_type" in frame else set()
            coverage = f"{measured_total}/{released_total}"
            stamp = (f"reproduced: {measured_total} of {released_total} configurations "
                     f"of the paper figure (artifact/results)")
            transform = (lambda source, present=present: restrict_to_present(source, present))
            if empty:
                # e.g. Fig. 17 needs single_thread_thp.alex_lipp_extended.csv, written by the full set only
                lines.append(f"| `{name}` | {coverage} | - | not drawn yet: no measured case in {', '.join(empty)} |")
                continue
        errors = run_notebook(RESULT_NOTEBOOK_DIR / name, stage, transform, stamp, probe_datasets=mode == "results")
        produced = collect(stage / "fig", destination)
        note = "; ".join(errors) if errors else "ok"
        lines.append(f"| `{name}` | {coverage or 'all'} | {', '.join(produced) or '-'} | {note} |")
    return lines


def run_profiling_notebooks(log_dir: Path, destination: Path) -> list[str]:
    stage = Path(tempfile.mkdtemp(prefix="robin-figures-profiling-"))
    try:
        (stage / "log").symlink_to(log_dir.resolve(), target_is_directory=True)
        workdir = stage / "notebooks"
        (workdir / "fig").mkdir(parents=True)
        lines = []
        for name in PROFILING_NOTEBOOKS:
            errors = run_notebook(PROFILING_NOTEBOOK_DIR / name, workdir)
            produced = collect(workdir / "fig", destination)
            note = "; ".join(errors) if errors else "ok"
            lines.append(f"| `{name}` | {log_dir} | {', '.join(produced) or '-'} | {note} |")
        return lines
    finally:
        shutil.rmtree(stage, ignore_errors=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("paper", "results", "profiling"), required=True)
    parser.add_argument("--experiments", nargs="+", choices=list(layout.EXPERIMENTS), default=None,
                        help="only the figures of these experiments (default: all)")
    parser.add_argument("--log-dir", type=Path, default=PROFILING_LOG,
                        help="profiling mode: directory written by run_case_profiling.sh")
    parser.add_argument("--output", type=Path, default=None, help="default: artifact/figures/<mode>")
    args = parser.parse_args()

    destination = args.output or FIGURES / args.mode
    if args.experiments:
        destination.mkdir(parents=True, exist_ok=True)
    else:
        fresh_dir(destination)
    if args.mode == "profiling":
        if not args.log_dir.is_dir():
            raise SystemExit(f"log directory {args.log_dir} does not exist; run run_case_profiling.sh first")
        header = "| notebook | log directory | figures | cell errors |"
        lines = run_profiling_notebooks(args.log_dir, destination)
    else:
        notebooks = list(RESULT_NOTEBOOKS)
        if args.experiments:
            wanted = {name for experiment in args.experiments for name in layout.EXPERIMENTS[experiment]["notebooks"]}
            notebooks = [name for name in notebooks if name in wanted]
        header = "| notebook | configurations (results/paper) | figures | notes |"
        lines = run_result_notebooks(args.mode, destination, notebooks)

    manifest = [f"# Figures, mode `{args.mode}`", "",
                "Drawn by the paper's notebooks, unmodified apart from the index list of "
                "results mode (indexes without a measured row are left out).", "",
                header, "| --- | --- | --- | --- |", *lines, ""]
    name = "MANIFEST.md" if not args.experiments else f"MANIFEST.{'+'.join(args.experiments)}.md"
    (destination / name).write_text("\n".join(manifest))
    print("\n".join(manifest))
    print(f"figures -> {destination}")


if __name__ == "__main__":
    main()
