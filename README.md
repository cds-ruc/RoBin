
# RoBin<img src="https://github.com/user-attachments/assets/88d35c0c-6286-418d-8ce4-83328711a4ef" width="64" height="64" />

Robin is a **Ro**bustness **B**enchmark for range **in**dexes (especially for updatable learned indexes).

> [Robin](https://en.wikipedia.org/wiki/European_robin) is also an insect-eating bird that offers great benefits to agriculture.

[![RoBin Build](https://github.com/cds-ruc/RoBin/actions/workflows/cmake-build.yml/badge.svg)](https://github.com/cds-ruc/RoBin/actions/workflows/cmake-build.yml)

## Notice

1. Face dataset contains the numeric_limit<uint64_t>, Some indexes may use this as a sential for easing implementation. Therefore, we shifted all fb keys by minus **one** as **Face-1** dataset.
2. We modify the LIPP/SALI's hyperparameter [MAX_DEPTH](https://github.com/cds-ruc/IndexRepo/blob/b237911cb31fc0a94c1b1911b0fbcadb8fd0870f/src/core/lipp.h#L1088) to ensure it can successfully run all the test cases (otherwise it will crash due to its assertion at runtime).
3. We modify the bulkload process of STX B+tree to ensure its node half filled (load factor = 0.5) after bulkloaading, which aligns its insertions and splits to show its performance robustness.
4. Other parameters of all indexes are the same as their original implementations.
5. All of our tested index implementations can be found in [this repo](https://github.com/cds-ruc/IndexRepo). Each branch is corresponding to one index.
6. We add profiling stats for art, btree, alex and lipp about the distribution of depth, comparison count of leaf node search, the model of root node and so on, with minor invasion.

## Reproduce Step

If you want to go faster, you can just run the following script to install the dependencies download the dataset and build the project:

```shell
bash prepare.sh
```

### Prepare
RoBin depends on the tbb, jemalloc and boost library. You can install them by the following command:

```shell
sudo apt update
sudo apt install -y libtbb-dev libjemalloc-dev libboost-dev
```

If the repository is not cloned with the `--recursive` option, you can run the following command to clone the submodule:

```shell
git submodule update --init --recursive
```

Download the dataset from remote and construct **linear** and **fb-1**:
```shell
cd datasets
bash download.sh
python3 gen_linear_fb-1.py
```

### Build
```shell
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -j
```
or just run the following script:
```shell
bash build.sh
```

### Reproduce

Benchmark all the competitors via RoBin with the following command:

**It may cost some time to finish.**
```shell
bash reproduce.sh
```

The results will be stored in the `results` directory.

Using the jupyter notebook to plot the results:
```shell
cd results
# open and run the following jupyter notebook to reproduce the figure in our paper
# such as single_thread.ipynb and etc.
```

### Profiling (tyh TODO)

Build with the flag "PROFILING":
```shell
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DPROFILING=ON
make -j
```

**Note that our code modifications for profiling have no impact on index performance when building without this flag for benchmark test.**

Run profiling script:
```shell
bash run_profiling.sh
```

Using the jupyter notebook to plot the profiling results:
```shell
cd script
## open and run the following jupyter notebooks to reproduce the figure in our paper
## analysis_cmp.ipynb
## analysis_depth.ipynb
## analysis_overfit.ipynb
```


### Run and Play
We also provide a script to run the RoBin with custom parameters. You can run the following command to see the help information:

```shell
python3 run.py --help
```


## Reference

1. We build this benchmark based on a well-designed benchmark [GRE](<https://github.com/gre4index/GRE>). The related paper is:
> Wongkham, Chaichon, et al. "Are updatable learned indexes ready?." Proceedings of the VLDB Endowment 15.11 (2022): 3004-3017.
