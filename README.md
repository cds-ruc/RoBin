# RoBin

Robin is a **Ro**bustness **B**enchmark for range **in**dexes (especially for updatable learned indexes).

## Notice

1. We only modify the Face dataset to support experiments on ALEX. Because the Face dataset contains the numeric_limit<uint64_t>, ALEX uses this as a sential for easing implementation. Therefore, we shifted all fb keys by minus one as **Face-1** dataset for testing ALEX only.
2. We modify the LIPP's hyperparameter (TODO) to ensure it can successfully run all the test cases (otherwise it will crash due to its assertion at runtime).
3. We modify the bulkload process of STX B+tree to ensure its node half filled (load factor = 0.5) after bulkloaading, which aligns its insertions and splits to show its performance robustness.
4. Other parameters of all indexes are the same as their original implementations.
5. Index Repo
6. Profiling Commit 

## Reference

1. We build this benchmark based on a well-designed benchmark [GRE](<https://github.com/gre4index/GRE>).
