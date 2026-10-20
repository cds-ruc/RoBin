# Fig. 5 and Fig. 6 (Finding 7): 99.9th-percentile tail latency, single- and 16-threaded

**Claim.** Learned indexes have low tail latency in the baseline case but large tail-latency fluctuation overall; their worst insert tail is far above that of the traditional indexes, more so with 16 threads.

**What to look at.** The learned indexes' insert bands reach far above B+tree's and ART's; at 16 threads the gap widens.

**How to check your runs.** The subset measures each index's slowest insert case on Covid: the slowest insert p99.9 of ALEX and LIPP (ALEX-OLC at 16 threads) is several times that of ART and B+tree. `--tier figures` adds only the baseline, full-build and bulkload-0 cases on all four datasets; these are not each dataset's slowest cases, so on Linear their insert tails need not be above B+tree's.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig05_06_tail_latency/run.sh` | claims tier ~50 min; `--tier figures` adds the reference cases the figures draw, ~6.5 h (four ALEX-OLC bulkload-0 cases time out after 30 min each, as in the paper) |
| full grid | `bash artifact/experiments/fig05_06_tail_latency/run.sh --full` | test_ro_lat_single_thread + test_ro_lat_multi_thread, ~37 h |
| released data only | `bash artifact/experiments/fig05_06_tail_latency/run.sh --plots-only` | ~1 min |

The 16-threaded cases (Fig. 6) run on 16 physical cores of one NUMA node, with memory
bound to the same node (the scripts set this up; see "CPU and NUMA binding" in
[`artifact/README.md`](../../README.md)), so cross-socket latency stays out of
the tails.

Results: `single_thread_tail_lat.csv`, `multi_thread_tail_lat.csv` in `artifact/results/`.
Figures: `single_thread_lat_overall.pdf`, `multi_thread_lat_overall.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
