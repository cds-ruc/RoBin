# RoBin

Robin is a **Ro**bustness **B**enchmark for range **in**dexes (especially for updatable learned indexes).

## Notice

1. We only modify the Face dataset to support experiments on ALEX. Because the Face dataset contains the numeric_limit<uint64_t>, ALEX uses this as a sential for easing implementation. Therefore, we shifted all fb keys by minus one as **Face-1** dataset for testing ALEX only.
2. We modify the LIPP's hyperparameter (TODO) to ensure it can successfully run all the test cases (otherwise it will crash due to its assertion at runtime).
3. We modify the bulkload process of STX B+tree to ensure its node half filled (load factor = 0.5) after bulkloaading, which aligns its insertions and splits to show its performance robustness.
4. Other parameters of all indexes are the same as their original implementations.
5. Index Repo
6. Profiling Commit 

## Reproduce Step
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

Download the dataset from remote and construct **linear** and **fb-1**
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

### Reproduce

Benchmark all the competitors via RoBin with the following command

**It may cost some time to finish**
```shell
export numanode=xxx # if you have multiple numa nodes, you must specify the numa node in case of the memory allocation from different numa nodes
bash reproduce.sh
```

Using the jupyter notebook to plot the result
```shell
cd script
## open and run the jupyter notebook to reprocude the figure in our paper
```

### Run and Play
We also provide a script to run the RoBin with custom parameters. You can run the following command to see the help information:

```shell
python3 run.py --help
```


## Reference

1. We build this benchmark based on a well-designed benchmark [GRE](<https://github.com/gre4index/GRE>).
