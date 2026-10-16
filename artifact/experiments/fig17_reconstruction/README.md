# Fig. 17 (Section 6.2): timely re-construction: bulkload fraction vs traditional indexes

**Claim.** Once most of the data is bulkloaded (few insertions), LIPP beats ART and B+tree in every case; ALEX needs a larger bulkload share and still fails on hard datasets.

**What to look at.** LIPP curves reach 100% at high bulkload fractions; some ALEX curves never do.

**How to check your runs.** With `--full`: the same shape. Without it, the figure is checked on the released data (`--plots-only`).

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig17_reconstruction/run.sh` | none: the figure is computed over the whole bulkload grid |
| full grid | `bash artifact/experiments/fig17_reconstruction/run.sh --full` | test_ro_thp_single_thread + test_ro_thp_single_thread_extended_bulksize, ~71 h |
| released data only | `bash artifact/experiments/fig17_reconstruction/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.csv`, `single_thread_thp.alex_lipp_extended.csv` in `artifact/results/`.
Figures: `alex_lipp_read_throughput_better_than_trad.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
