# Fig. 17 (Section 6.2): timely re-construction: bulkload fraction vs traditional indexes

**Claim.** Once most of the data is bulkloaded (few insertions), LIPP beats ART and B+tree in every case; ALEX needs a larger bulkload share and still fails on hard datasets.

**What to look at.** LIPP curves reach 100% at high bulkload fractions; some ALEX curves never do.

**How to check your runs.** The subset measures `<uniform, shuffled>` on Linear (easy) and Face (hard): LIPP at the 90% bulkload fraction, ALEX at 60% and 100%, and the B+tree and ART cases the thresholds average over; the figure marks those fractions and leaves the others out. LIPP is above both thresholds on both datasets; ALEX is above both on Linear, and below both on Face even at 100%. With `--full`: the whole curve.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig17_reconstruction/run.sh` | claims tier, 14 runs, ~25 min |
| full grid | `bash artifact/experiments/fig17_reconstruction/run.sh --full` | test_ro_thp_single_thread + test_ro_thp_single_thread_extended_bulksize, ~71 h |
| released data only | `bash artifact/experiments/fig17_reconstruction/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.csv`, `single_thread_thp.alex_lipp_extended.csv` in `artifact/results/`.
Figures: `alex_lipp_read_throughput_better_than_trad.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
