# RoBin Artifact Evaluation

Artifact of the SIGMOD 2026 paper *Understanding Robustness Issues of
Updatable Learned Indexes: [Experiments & Analysis]*
([PACMMOD, DOI 10.1145/3749188](https://dl.acm.org/doi/abs/10.1145/3749188)),
prepared for the [SIGMOD ARI guidelines](https://reproducibility.sigmod.org/#Guidelines).
This guide is self-contained: it starts from an empty directory and every
command after the clone runs from the repository root.

* Repository: https://github.com/cds-ruc/RoBin
* Benchmark: `run.py` driving `./build/microbench` (C++17), with the compared
  indexes as git submodules from https://github.com/cds-ruc/IndexRepo.
* The paper's measurements: the repository's `conf_rel` branch. The scripts
  use them to redraw the paper's figures and as the comparison point for your
  runs. Nothing else in the checkout is a measurement; your runs fill
  `artifact/results/`.

## At a glance

**Key results.** Figures 3-17 and Tables 1-2 (Findings 1-12, Sections 5 and
6). Figures 1 (design diagram) and 2 (dataset CDFs) are descriptive and back
no claim. §1 maps every key result to its experiment directory.

**Software.** Linux x86-64 (developed on Ubuntu 22.04), `git`, `bash`,
Python ≥ 3.11 with `pip`. To build and run cases also: CMake ≥ 3.14, a C++17
compiler (GCC 11 used), `apt` for the libraries `build.sh` installs, `wget`
and `sha256sum` for the datasets, and optionally `numactl`.

**Setup.**

```bash
# 1. clone the repository (every entry point)
git clone https://github.com/cds-ruc/RoBin.git
cd RoBin

# 2. Python packages (every entry point)
pip install -r artifact/requirements.txt  # pandas, numpy, matplotlib, seaborn, pinned

# 3. build and datasets (entry points that run cases)
bash build.sh release                     # fetches the 15 index submodules, installs libtbb, libmkl,
                                          # libjemalloc, libboost with apt (run as root or preinstall
                                          # them), then builds ./build/microbench
bash artifact/data/fetch.sh               # the datasets (five files, 8.0 GB; the cases use linear, covid,
                                          # fb-1, osm; fb only builds fb-1), checksums verified
export numanode=0                         # NUMA node the runs bind to; the artifact scripts default to 0,
                                          # run.py/reproduce.sh called directly need it when numactl is installed
```

A full clone already includes the `conf_rel` branch, and the scripts read it
from there; there is nothing to download separately. A shallow (`--depth`)
or `--single-branch` clone lacks it; add it with
`git fetch origin conf_rel:refs/remotes/origin/conf_rel`. The index
submodules are needed only to build, so step 3 fetches them; cloning with
`--recursive` instead is equivalent.

**Entry points.** Every case loads all 200M keys, so the paper's full grid is
~150 h of single-core work. Pick one:

| command | what it reproduces | time | needs |
| --- | --- | --- | --- |
| `bash artifact/run_small.sh --plots-only` | the paper's figures (Fig. 3-9, 16, 17), Tables 1-2 and the headline number of each finding, recomputed from the released measurements | ~1 min | setup steps 1-2 |
| `bash artifact/run_small.sh` | subset: the key cases of every throughput, latency, space and mitigation claim (Fig. 3-9, 16, 17), at full scale, on your machine | ~4 h | setup steps 1-3, hardware below |
| `bash artifact/experiments/fig10_15_root_cause/run.sh` | Section 5 root-cause figures (Fig. 10-15) from an instrumented build; not part of `run_small.sh` | ~2-3 h | setup steps 1-3, hardware below |
| `bash artifact/run_small.sh --tier figures` | after the default subset: the reference cases (baseline, full build, bulkload 0) Fig. 5 and 6 draw and the Fig. 8 panels, on all four datasets | ~11 h | setup steps 1-3, hardware below |
| `bash artifact/run_all.sh` | full set: every grid of the paper and the Section 5 profiling | ~1 week | setup steps 1-3, hardware below |
| `bash artifact/experiments/<claim>/run.sh [--full]` | one claim, subset or full grid | §3 | setup steps 1-3, hardware below |

**Hardware.**
* Minimum: Linux x86-64, ≥ 17 logical CPUs on the NUMA node the runs bind to
  (16-thread cases use 16 of them, CPU 0 of the node is left free), ≥ 48 GB RAM
  on that node for the default, `figures` and `core` tiers (their largest case
  is ALEX at ~37 GB), ~9 GB disk.
* Recommended: ≥ 64 GB RAM, needed for `--tier extended` and the full set
  (both include SALI at ~55 GB), 2 NUMA nodes with `numactl`.

**What success looks like.**
* `run_small.sh --plots-only` prints
  `reference claims: 31/31 within tolerance (2 informational), 0 mismatch`
  and ends with `=== produced ===`.
* Each case prints `[n/N] <case> (~eta, ~eta left)`, then
  `ok -> <result file> (+k records)`.
* `artifact/results/status.json` marks each case `ok`, apart from the
  crashes and timeouts listed below.

**Benign messages.**
* `binding with numa node: 0`, and compiler warnings from the index
  submodules during `build.sh`.
* `taskset 1-1 -> 2: CPUs of numa node 0, where memory is bound`: on machines
  that number CPUs alternately across sockets (node 0 = CPUs 0, 2, 4, ...),
  `run.py` moves the pinning onto the bound node so memory stays local.
* `RuntimeWarning: invalid value encountered in scalar divide` from
  `analysis_preload.ipynb` in results mode: a CDF of Fig. 16 whose cases you
  have not run (e.g. after `--datasets fb-1`, which has no ALEX uint64 case);
  that curve's label reads `nan`.
* `cell N: failed-case table skipped, *_fail_case.log not released` in
  `artifact/figures/paper/MANIFEST.md`: the paper's failed-case logs were not
  released, so those two notebook cells are skipped. The figures are
  unaffected, and your runs write the logs into `artifact/results/`.
* `cell N: not drawn yet, its cases are not in artifact/results` in
  `artifact/figures/results/MANIFEST.md`, for figures whose cases you have
  not run yet.
* In `--tier extended`, DyTIS, DILI and FINEdex cases recorded as `failed` or
  `timeout`. These crashes and timeouts are the paper's own result (Fig. 3/4
  captions, Table 1, Finding 6), not an artifact problem.
* In `--tier figures`, the four ALEX-OLC `<bulkload 0>` 16-thread latency
  cases (one per dataset) recorded as `timeout` after 30 minutes each: ALEX-OLC
  deadlocks when it starts from an empty index. This matches the paper's
  ALEX-OLC timeouts due to deadlocks (Section 4.2); the released data has no
  such row either, so Fig. 6 has no bulkload-0 line for ALEX-OLC.
* Covid/LIPP `<segmented, 2M, sorted>`, the slowest subset case, can reach
  run.py's 30-minute per-case limit on a slow machine. It is then recorded as
  `timeout`, with its finished phases kept.

**Disk.** The datasets are raw uint64 binaries, 1,600,000,008 bytes each, with
no decompression step: `covid`, `osm` and `fb` are downloaded, `linear` and
`fb-1` are generated. 8.0 GB in total, plus 45 MB for the build.

**Resuming.** Every step can be rerun:
* `artifact/data/fetch.sh` keeps existing dataset files.
* The subset runner skips every case recorded in
  `artifact/results/status.json`.
* `run_all.sh` marks each finished grid in `artifact/results/.full/`.

An interrupted run continues where it stopped; `--force` runs again.

**What depends on the hardware.** Absolute throughput and latency depend on
the hardware. Relative statements must hold on any machine, and those are
what the claims are:
- which index is faster;
- how far a worst case falls below its baseline;
- whether a parameter helps or hurts.

Space does not depend on the CPU. Suspect a real problem if:
- an ordering flips (e.g. LIPP's worst insert above B+tree's);
- a lookup with an outlier is not below the same lookup without it;
- a space value differs from the case's `reference` in `artifact/cases.json`
  by more than ~10%.

**Contact.**
* Yuanhui Luo (losk@ruc.edu.cn).
* Corresponding authors: Minhui Xie (xieminhui@ruc.edu.cn) and Yunpeng Chai
  (ypchai@ruc.edu.cn).
* Or open an issue at https://github.com/cds-ruc/RoBin.

## 1. Claims map

Throughput, latency and space change from machine to machine and from run to
run, so a reproduction is judged on the **trend** each figure shows, not on
its numbers: which index is above which, which configuration falls below its
baseline, which way a parameter moves the result. Each experiment directory's
`README.md` states the claim, what to look at in the figure and how to check
your runs.

| paper | experiment | trend the paper claims |
| --- | --- | --- |
| Fig. 3, Table 1 | [`fig03_tab1_single_thread`](experiments/fig03_tab1_single_thread/) | learned indexes fluctuate far more than B+tree and ART and are often slower than ART; except ALEX, their worst-case insert falls far below B+tree's |
| Fig. 4, Table 2 | [`fig04_tab2_multi_thread`](experiments/fig04_tab2_multi_thread/) | the same picture with 16 threads, larger gaps; ALEX-OLC and SALI do not scale in their worst insert case, B+tree-OLC and ART-OLC do |
| Fig. 5, 6 | [`fig05_06_tail_latency`](experiments/fig05_06_tail_latency/) | the worst insert tail latency of learned indexes is far above the traditional indexes', more so with 16 threads |
| Fig. 7 | [`fig07_space`](experiments/fig07_space/) | learned-index space varies widely; LIPP uses many times the data size; ALEX has an abnormal case on Face; SALI needs about twice LIPP's space |
| Fig. 8a-c | [`fig08_sensitivity`](experiments/fig08_sensitivity/) | one changed variable moves learned indexes a lot: ALEX reacts to the insert pattern, LIPP to bulkload size and sampling |
| Fig. 9 | [`fig09_out_of_bound`](experiments/fig09_out_of_bound/) | one outlier in the bulkload lowers ALEX's lookup in most cases (75%), and by half or more in about one in five (20%) |
| Fig. 10-15 | [`fig10_15_root_cause`](experiments/fig10_15_root_cause/) | overfitted root models, unbalanced trees, SMOs that do not fix the models, gap- and metadata-dominated space |
| Fig. 16 | [`fig16_data_augmentation`](experiments/fig16_data_augmentation/) | 1% augmented keys mostly help LIPP and can hurt ALEX |
| Fig. 17 | [`fig17_reconstruction`](experiments/fig17_reconstruction/) | with most of the data bulkloaded, LIPP beats ART and B+tree everywhere; ALEX needs more and still fails on hard datasets |

**Notation.** A case is written `<sampling, bulkload size, insert pattern>`,
e.g. `<uniform, 100M, shuffled>`, the paper's baseline:
- the index is built (bulkloaded) on 100M keys sampled uniformly from the
  200M-key dataset;
- the other 100M are then inserted in shuffled order;
- a lookup pass over all 200M keys follows.

`full` builds on the whole dataset and inserts nothing; `segmented` samples a
contiguous key range. The result files name these cases by
`test_suite`:

| `test_suite` | sampling | insert pattern |
| --- | --- | --- |
| 10 | full | none |
| 21 | uniform | sorted |
| 22 | uniform | shuffled |
| 41 | segmented | sorted |
| 42 | segmented | shuffled |

`preload_suite` 33 and 34 add 1% keys from the uint64 or the dataset key
space (Fig. 16).

## 2. Layout

```text
LICENSE                     MIT (the index submodules keep their own licenses)
reproduce.sh                every measurement grid of the paper (used by run_all.sh)
run.py, build.sh, src/      the benchmark
run_case_profiling.sh       the Section 5 profiling cases
artifact/
  README.md                 this guide
  requirements.txt          pinned Python packages
  run_small.sh              subset: key cases of every claim
  run_all.sh                full set: every grid + profiling
  data/
    fetch.sh, SHA256SUMS    datasets into datasets/, verified
    fetch_reference.sh      the released measurements into artifact/reference/
  experiments/<claim>/      one directory per paper result: README.md + run.sh
  plot/
    plot.py                 runs the paper's notebooks on reference/ or results/
    notebooks/              the paper's plotting notebooks (copies of result/, profiling_result/)
  lib/
    gen_cases.py            builds the case set (cases.json) from the released measurements
    run_cases.py            runs cases, one at a time, into results/
    analyze.py              recomputes the paper's quoted numbers and Tables 1-2 (SUMMARY.md)
    layout.py, common.sh    shared paths and shell steps
    vendor_submodules.sh    offline only: extracts the index sources when git submodules cannot be fetched
generated by the scripts (not in git):
  artifact/reference/       the released measurements
  artifact/results/         your measurements
  artifact/figures/         paper/, results/, profiling/ (each with MANIFEST.md)
  artifact/cases.json       the case set
  artifact/SUMMARY.md       recomputed numbers and Tables 1-2 (+ summary.json)
```

**Results** are written under the file names the paper's notebooks read:
one file per measurement grid, never one file per run.

| file in `artifact/results/` | records |
| --- | --- |
| `single_thread_thp.csv`, `multi_thread_thp.csv` | lookup and insert throughput, with space |
| `single_thread_tail_lat.csv`, `multi_thread_tail_lat.csv` | 99.9th-percentile latency |
| `memory_consumption.csv` | space, derived from the two throughput files |
| `single_thread_thp.alex_oob.csv` | ALEX with one outlier in the bulkload |
| `single_thread_thp.alex_lipp_preload_1_100_{uint64,dataset}.csv` | bulkload with 1% augmented keys |
| `single_thread_thp.alex_lipp_extended.csv` | bulkload sizes outside the main grid (Fig. 17; the subset writes the 60% and 90% fractions) |
| `*_fail_case.log` | run.py's `Failed command:` lines |
| `status.json`, `logs/` | per-case status and stdout |

The subset runner appends each case to its file and replaces earlier records
of the same case on `--force`. The full set writes whole files. Both use the
same names, so the full set supersedes the subset.

**`artifact/SUMMARY.md`** is written by every entry point. It recomputes from
the released measurements the headline number of each finding (31 rows) plus
Tables 1 and 2, and compares them with the printed values; per-dataset
breakdowns and the Section 5 statistics, which need the full profiling logs,
are not recomputed. This checks the scripts; it is not a measurement of your
machine. Once `artifact/results/` holds your runs, it adds a table that
compares them with the released measurements of the same cases.

## 3. Running

**Subset.** `run_small.sh` runs the cases one at a time, each pinned to one
CPU (16 CPUs for 16-thread cases) of the NUMA node its memory is bound to.
Concurrent cases slow each other down by different amounts, so there is no
parallel mode.

`artifact/lib/gen_cases.py` derives the cases from the released
measurements. The `core` and `extended` tiers take, for every (dataset,
index):
- the baseline `<uniform, 100M, shuffled>`;
- the full build `<full, 200M>`;
- the extremes each figure plots: the best and worst lookup case, the worst
  insert case, the worst 99.9th-latency case of lookup and of insert, and the
  largest space case.

To those it adds the one-variable sensitivity cases, the worst released
outlier pairs, the largest augmentation gain and loss, and for Section 6:
- Fig. 16: one case with both key spaces, ALEX `<uniform, 100M, shuffled>` on
  Linear, augmented with uint64 keys and with dataset keys;
- Fig. 17: on Linear (easy) and Face (hard), `<uniform, shuffled>`, LIPP at
  the 90% bulkload fraction and ALEX at 60% and 100%, the fractions the text
  names, plus the B+tree and ART cases its thresholds average over.

The default `claims` tier is a fixed slice of those (`claims_selection` in
`gen_cases.py`): per claim, a few (dataset, index) pairs, mostly all four
indexes on Linear plus the datasets the extreme cases come from, not every
pair. A configuration two experiments need is one case.

| experiment | default (`claims`) | `--tier figures` adds | `--full` runs (reproduce.sh) |
| --- | --- | --- | --- |
| `fig03_tab1_single_thread` | 28 runs, ~1 h | 10 runs, ~45 min | `test_ro_thp_single_thread`, 62 h |
| `fig04_tab2_multi_thread` | 14 runs, ~5 min | - | `test_ro_thp_multi_thread`, 13 h |
| `fig05_06_tail_latency` | 10 runs, ~50 min | 90 runs, ~6.5 h (2 h of it the four ALEX-OLC timeouts) | `test_ro_lat_{single,multi}_thread`, 37 h |
| `fig07_space` | 5 runs, ~10 min | - | both throughput grids, 75 h |
| `fig08_sensitivity` | 18 runs, ~20 min | 71 runs, ~5 h | `test_ro_thp_single_thread`, 62 h |
| `fig09_out_of_bound` | 9 runs, ~25 min | 2 runs, ~5 min | + `test_ro_thp_single_thread_extended_oob`, 4 h |
| `fig10_15_root_cause` | not in `run_small.sh`: run its `run.sh`, ~2-3 h | - | same |
| `fig16_data_augmentation` | 9 runs, ~25 min | - | + `test_ro_thp_single_thread_data_augmentation`, 25 h |
| `fig17_reconstruction` | 14 runs, ~25 min | - | + `test_ro_thp_single_thread_extended_bulksize`, 9 h |

The subset times are wall time, loading included. The `--full` times are
operations only, estimated from the paper's throughputs; loading the dataset
and bulkloading come on top. Experiments share grids: e.g.
`fig09_out_of_bound --full` reuses the single-threaded grid if
`fig03_tab1_single_thread --full` already ran it.

**`run_small.sh` options.**
* `--tier`:
  * `claims` (default): the cases above;
  * `figures`: the remaining cases Fig. 5, 6 and 8 plot on all datasets (run it after the default);
  * `core`: every extreme of B+tree, ART, ALEX, LIPP and their OLC variants;
  * `extended`: only the DyTIS, DILI, FINEdex and SALI extremes.
* `--experiments "..."` and `--datasets "..."` restrict the selection.
* `--force` reruns finished cases.
* `--plots-only` runs nothing.

**Smoke test.** A reduced dataset exercises the build and the scripts only.
It is not evidence for any finding, because bulkload sizes and tree shapes
change with the scale.

```bash
python3 run.py --index=lipp --dataset=linear --concurrency=1 \
  --sampling_method=segmented --bulkload_size=1000000 --insert_pattern=sorted \
  --dataset_size=2000000 --table_size=2000000 --taskset=1-1 --output_path=/tmp/smoke.csv
```

`/tmp/smoke.csv` holds one `insert` and one `lookup` record. Add
`--out_of_bound_inject=True` or `--preload_suite=33` to exercise the Fig. 9 and
Fig. 16 paths.

## 4. Figures

There is one plotting implementation: the paper's notebooks in
`artifact/plot/notebooks/`. `artifact/plot/plot.py` stages their input files
in a scratch directory and runs their code cells there. The outputs carry the
file names of the paper's figures.

| mode | input | output |
| --- | --- | --- |
| `--mode paper` | `artifact/reference/` | `artifact/figures/paper/` |
| `--mode results` | `artifact/results/` only, no released row mixed in | `artifact/figures/results/`, each PDF stamped with the configurations and datasets it was drawn from |
| `--mode profiling` | `artifact/results/profiling_log/` | `artifact/figures/profiling/` |

Each output directory gets a `MANIFEST.md` listing what each notebook drew.
An experiment's `run.sh` writes `MANIFEST.<experiment>.md` instead, except
`fig10_15_root_cause/run.sh`, which writes `artifact/figures/profiling/MANIFEST.md`.

| paper | file | notebook |
| --- | --- | --- |
| Fig. 3, 8a-c | `single_thread_overall_thp.pdf`, `sensitivity_impact_of_{bulkload_size,insert_pattern,sampling_method}.pdf` | `results/single_thread.ipynb` |
| Fig. 4 | `multi_thread_overall_thp.pdf` | `results/multi_thread.ipynb` |
| Fig. 5, 6 | `single_thread_lat_overall.pdf`, `multi_thread_lat_overall.pdf` | `results/{single,multi}_thread_tail_lat.ipynb` |
| Fig. 7 | `memory_consumption_overall.pdf` | `results/memory_consumption.ipynb` |
| Fig. 9 | `alex_read_throughput_change_ratio_under_oob.pdf` | `results/analysis_alex_oob.ipynb` |
| Fig. 10-15 | `lipp_overfit_*`, `alex_overfit_*`, `*_treeheight_cdf`, `alex_cmp_cdf`, `alex_depth_smo`, `memory_breakdown`, `alex_memory_{1,2}` | `profiling/analysis_{overfit,depth,smo,memory}.ipynb` |
| Fig. 16 | `alex_lipp_read_throughput_change_ratio_under_preload.pdf` | `results/analysis_preload.ipynb` |
| Fig. 17 | `alex_lipp_read_throughput_better_than_trad.pdf` | `results/analysis_bulkload_variance.ipynb` |
| Tables 1, 2 | `artifact/SUMMARY.md` | `artifact/lib/analyze.py` |

In results mode, a figure is drawn from whatever cases your results hold:
every measured point is plotted, and a panel appears for each dataset with
measured cases (the stamp on each PDF and `MANIFEST.md` list them).
- After the default subset: Fig. 3, 4, 7, 9, 16 and 17 on the subset's cases,
  Fig. 8b and 8c on Linear, Fig. 5 and 6 on Covid only (Fig. 17 marks the
  bulkload fractions you measured and leaves the others out);
- after `--tier figures`: Fig. 5, 6 and 8a-c on all four datasets.

## 5. Root-cause profiling (Fig. 10-15)

`bash artifact/experiments/fig10_15_root_cause/run.sh` does four things:
1. builds the instrumented binary (`build.sh profiling`: `-DPROFILING`,
   `-DROOT_PROFILING`);
2. runs the cases of `run_case_profiling.sh` into
   `artifact/results/profiling_log/`;
3. rebuilds the release binary (`build.sh` always recreates `build/`);
4. draws the figures.

The instrumentation changes throughput, so no throughput case runs on that
binary. The paper's profiling logs were not released, so these figures are
always regenerated.

## 6. Reviewer checklist

* [ ] `bash artifact/run_small.sh --plots-only` exits 0, and
  `artifact/SUMMARY.md` reports 31/31 numbers within tolerance, Table 1 21/21
  and Table 2 36/36.
* [ ] `artifact/figures/paper/` holds 11 PDFs (Fig. 3-9, 16, 17).
* [ ] `bash artifact/run_small.sh` finishes, and `artifact/results/status.json`
  lists every case.
* [ ] The trend of every experiment's `README.md` holds in
  `artifact/figures/results/` and in your result files.
* [ ] `fig10_15_root_cause/run.sh` produces the 11 root-cause PDFs.
