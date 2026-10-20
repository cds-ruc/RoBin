# Fig. 4 and Table 2 (Findings 5-6): 16-threaded lookup and worst-case insert

**Claim.** With 16 threads the lookup picture is the same as single-threaded, with larger gaps. B+tree-OLC and ART-OLC scale in their worst insert case, while ALEX-OLC and SALI do not, on some datasets dropping to or below the single-threaded worst cases of Table 1.

**What to look at.** The same ordering as Fig. 3; B+tree-OLC / ART-OLC worst-insert cells far above the single-threaded ones on every dataset, ALEX-OLC (Linear, Face) and SALI cells at the single-threaded level.

**How to check your runs.** On Linear, ALEX-OLC's slowest 16-thread insert is below ALEX's slowest single-threaded insert (run `fig03_tab1_single_thread` too) and far below B+tree-OLC's.

| | command | time |
| --- | --- | --- |
| subset | `bash artifact/experiments/fig04_tab2_multi_thread/run.sh` | claims tier, ~5 min |
| full grid | `bash artifact/experiments/fig04_tab2_multi_thread/run.sh --full` | test_ro_thp_multi_thread, ~13 h |
| released data only | `bash artifact/experiments/fig04_tab2_multi_thread/run.sh --plots-only` | ~1 min |

The 16-threaded cases run on 16 physical cores of one NUMA node, with memory bound to
the same node (the scripts set this up; see "CPU and NUMA binding" in
[`artifact/README.md`](../../README.md)). Cross-socket effects are excluded,
so the fluctuation comes from the index and the workload.

Results: `multi_thread_thp.csv`, `multi_thread_fail_case.log` in `artifact/results/`.
Figures: `multi_thread_overall_thp.pdf` in `artifact/figures/paper/` (released
data) and `artifact/figures/results/` (your runs); Table 2 in `artifact/SUMMARY.md`.

Table 2's SALI and FINEdex cells have a "your result" only after
`--tier extended` (their extremes; FINEdex mostly crashes, as in the paper) or
`--full`; the default subset leaves them `-`.
