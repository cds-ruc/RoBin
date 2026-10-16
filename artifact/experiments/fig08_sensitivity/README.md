# Fig. 8a-c (Findings 10-11): sensitivity to one workload variable

**Claim.** Changing one variable of the baseline moves learned indexes a lot, and each index reacts to different variables: ALEX to the insert pattern, LIPP to bulkload size and sampling.

**What to look at.** (a) the learned rows change strongly with bulkload size; (b) on Linear and Covid, ALEX's sorted bar well below its shuffled bar, LIPP's bars close; (c) LIPP's segmented bar well below uniform, ALEX's close.

**How to check your runs.** ALEX is much faster with shuffled than with sorted insert; LIPP is much slower with segmented than with uniform sampling.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig08_sensitivity/run.sh` | claims tier ~20 min; `--tier figures` adds the bulkload sweep on all four datasets, ~3.5 h |
| full grid | `bash artifact/experiments/fig08_sensitivity/run.sh --full` | test_ro_thp_single_thread, ~62 h |
| released data only | `bash artifact/experiments/fig08_sensitivity/run.sh --plots-only` | ~1 min |

Results: `single_thread_thp.csv` in `artifact/results/`.
Figures: `sensitivity_impact_of_bulkload_size.pdf` (8a), `sensitivity_impact_of_insert_pattern.pdf` (8b), `sensitivity_impact_of_sampling_method.pdf` (8c) in `artifact/figures/paper/` (released data) and
`artifact/figures/results/` (your runs).
