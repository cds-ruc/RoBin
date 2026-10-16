# Fig. 7 (Findings 8-9): index space

**Claim.** Learned-index space varies widely across cases while traditional indexes vary much less; LIPP uses many times the data size; ALEX is usually the smallest but has one abnormal case on Face; SALI needs about twice LIPP's space.

**What to look at.** A tall LIPP band far above the data size; one isolated high ALEX point on Face; SALI above LIPP.

**How to check your runs.** LIPP's cases and ALEX's Face case are a multiple of the data size, B+tree and ART much less; SALI uses clearly more than LIPP on the same workload. Space does not depend on the CPU, so it should be close to the paper's.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig07_space/run.sh` | claims tier, ~10 min |
| full grid | `bash artifact/experiments/fig07_space/run.sh --full` | test_ro_thp_single_thread + test_ro_thp_multi_thread, ~75 h |
| released data only | `bash artifact/experiments/fig07_space/run.sh --plots-only` | ~1 min |

Results: `memory_consumption.csv` (derived from the two throughput files: every run records its space) in `artifact/results/`.
Figures: `memory_consumption_overall.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
