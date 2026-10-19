# Fig. 3 and Table 1 (Findings 1-4): single-threaded lookup and worst-case insert

**Claim.** Learned indexes fluctuate far more across test cases than B+tree and ART; in a large share of cases they are slower than ART, and some are slower than B+tree; building on the entire dataset is usually not their best case. Except ALEX, learned indexes can have a worst-case insert throughput far below B+tree's, or crash / time out.

**What to look at.** The learned indexes' bands are much taller than the traditional ones'; many learned points lie below ART's baseline; the full-build line is not at the top of the learned bands. In Table 1, LIPP's cells are below B+tree's on every dataset, most DyTIS and DILI cells too.

**How to check your runs.** The subset re-measures all four indexes on Linear: there, the range of ALEX and LIPP across cases is much wider than that of B+tree and ART, at least one learned case is below ART's baseline, and LIPP's slowest insert is below B+tree's slowest. On the other datasets it re-measures only some indexes (Covid: ALEX, LIPP; Face: B+tree, ART, ALEX; OSM: LIPP's full build), so their panels show only those; `--full` measures every dataset and index.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig03_tab1_single_thread/run.sh` | claims tier, ~1 h |
| full grid | `bash artifact/experiments/fig03_tab1_single_thread/run.sh --full` | test_ro_thp_single_thread, ~62 h |
| released data only | `bash artifact/experiments/fig03_tab1_single_thread/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.csv`, `single_thread_fail_case.log` in `artifact/results/`.
Figures: `single_thread_overall_thp.pdf` in `artifact/figures/paper/` (released
data) and `artifact/figures/results/` (your runs); Table 1 in `artifact/SUMMARY.md`.

Table 1's DyTIS cells for Linear and Face come from runs killed at run.py's
30-minute limit (keys inserted before the limit / elapsed time), so they are not
in the CSVs and `SUMMARY.md` marks them `timeout`.
