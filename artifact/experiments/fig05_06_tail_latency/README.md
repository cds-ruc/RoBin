# Fig. 5 and Fig. 6 (Finding 7): 99.9th-percentile tail latency, single- and 16-threaded

**Claim.** Learned indexes have low tail latency in the baseline case but large tail-latency fluctuation overall; their worst insert tail is far above that of the traditional indexes, more so with 16 threads.

**What to look at.** The learned indexes' insert bands reach far above B+tree's and ART's; at 16 threads the gap widens.

**How to check your runs.** The slowest insert p99.9 of ALEX and LIPP (ALEX-OLC at 16 threads) is several times that of ART and B+tree on the same dataset.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig05_06_tail_latency/run.sh` | claims tier ~50 min; `--tier figures` adds the cases the figures draw, ~2.2 h |
| full grid | `bash artifact/experiments/fig05_06_tail_latency/run.sh --full` | test_ro_lat_single_thread + test_ro_lat_multi_thread, ~37 h |
| released data only | `bash artifact/experiments/fig05_06_tail_latency/run.sh --plots-only` | ~1 min |

Results: `single_thread_tail_lat.csv`, `multi_thread_tail_lat.csv` in `artifact/results/`.
Figures: `single_thread_lat_overall.pdf`, `multi_thread_lat_overall.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
