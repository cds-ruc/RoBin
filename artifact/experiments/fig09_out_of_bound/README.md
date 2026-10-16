# Fig. 9 (Finding 12): ALEX with one outlier in the bulkload

**Claim.** A single outlier in the bulkload lowers ALEX's lookup throughput in most cases, often by half or more.

**What to look at.** Most of the CDF lies left of 0, with a sizeable tail beyond -50%.

**How to check your runs.** Lookup with the outlier is below lookup without it in every pair.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig09_out_of_bound/run.sh` | claims tier, ~25 min |
| full grid | `bash artifact/experiments/fig09_out_of_bound/run.sh --full` | test_ro_thp_single_thread + test_ro_thp_single_thread_extended_oob, ~66 h |
| released data only | `bash artifact/experiments/fig09_out_of_bound/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.alex_oob.csv` (with the outlier) and `single_thread_thp.csv` (the same cases without it) in `artifact/results/`.
Figures: `alex_read_throughput_change_ratio_under_oob.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).

The outlier is `UINT64_MAX`, added to the bulkloaded keys (`--out_of_bound_inject=True`).
