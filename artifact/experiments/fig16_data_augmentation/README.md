# Fig. 16 (Section 6.1): bulkload with 1% augmented keys

**Claim.** Adding 1% augmented keys to the bulkload mostly helps LIPP and can hurt ALEX; for ALEX, keys from the dataset's key space are better than keys from the whole uint64 space.

**What to look at.** LIPP's CDFs mostly right of 0; ALEX's partly left of 0, the dataset curve right of the uint64 one.

**How to check your runs.** Augmentation raises LIPP's lookup in both pairs and lowers ALEX's on Face.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig16_data_augmentation/run.sh` | claims tier, ~25 min |
| full grid | `bash artifact/experiments/fig16_data_augmentation/run.sh --full` | test_ro_thp_single_thread + test_ro_thp_single_thread_data_augmentation, ~87 h |
| released data only | `bash artifact/experiments/fig16_data_augmentation/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.alex_lipp_preload_1_100_uint64.csv` (preload suite 33), `single_thread_thp.alex_lipp_preload_1_100_dataset.csv` (suite 34), compared with `single_thread_thp.csv` in `artifact/results/`.
Figures: `alex_lipp_read_throughput_change_ratio_under_preload.pdf` in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
