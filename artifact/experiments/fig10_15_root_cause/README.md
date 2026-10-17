# Fig. 10-15 (Section 5): root-cause analysis

| figure | claim | what to look at |
| --- | --- | --- |
| Fig. 10-11 | The root models overfit the bulkload: LIPP's root (Covid `<segmented, 2M, sorted>`) and ALEX's root (Face `<uniform, 100M, shuffled>`, skewed by its outliers near `UINT64_MAX`) put most keys into very few slots, and insertions do not correct them. | key mass concentrated in a few root slots, before and after insertions |
| Fig. 12 | In the best cases, skewed data sits in a few deep sub-trees and most data stays shallow; in the worst cases (OSM) data spreads over many levels. | the worst-case tree-height distribution spreads over more levels than the best case's |
| Fig. 13 | With the same tree height, ALEX under sorted insert is slower than under shuffled insert, needs many more comparisons, and performs far more SMOs without fixing its models (Covid). | sorted vs shuffled: similar height, more comparisons, far more SMOs |
| Fig. 14-15 | LIPP's space is dominated by empty gaps and SALI adds latch metadata on top; ALEX's Face blow-up is node metadata of a huge number of empty leaves at the low end of the key space. | gap segment dominates the LIPP bar; empty ALEX nodes concentrated at the smallest keys |

| | command | time |
| --- | --- | --- |
| run | `bash artifact/experiments/fig10_15_root_cause/run.sh` | ~2-3 h |
| figures from existing logs | `bash artifact/experiments/fig10_15_root_cause/run.sh --plots-only` | ~1 min |

The step builds the instrumented binary (`build.sh profiling`: `-DPROFILING` and
`-DROOT_PROFILING`), runs the ten cases of `run_case_profiling.sh` into
`artifact/results/profiling_log/`, and rebuilds the release binary. The paper's
own profiling logs were not released, so these figures are always regenerated.

Figures: `lipp_overfit_{bulkload,insert}.pdf`, `alex_overfit_{bulkload,insert}.pdf`,
`{alex,lipp}_treeheight_cdf.pdf`, `alex_cmp_cdf.pdf`, `alex_depth_smo.pdf`,
`memory_breakdown.pdf`, `alex_memory_{1,2}.pdf` in `artifact/figures/profiling/`.
