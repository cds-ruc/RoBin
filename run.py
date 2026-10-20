import os
import signal
import sys
import shlex
import subprocess
import argparse


def parse_cpus(spec):
    """CPU list syntax of taskset/sysfs ("1-3,8") -> [1, 2, 3, 8]."""
    cpus = []
    for part in spec.split(","):
        low, _, high = part.strip().partition("-")
        cpus += range(int(low), int(high or low) + 1)
    return cpus


def cpus_on_node(requested, node):
    """Keep a --taskset request on the NUMA node numactl binds memory to.

    --taskset=1-N means "N CPUs, starting from CPU 1" (CPU 0 is left to the OS).
    On machines that number CPUs round-robin across sockets (node 0 = 0,2,4,...),
    CPU 1 sits on node 1 while numactl --membind keeps memory on node 0, so every
    access would be remote.  Such a request is mapped to the same number of
    CPUs of the bound node, again skipping its first CPU.
    """
    path = f"/sys/devices/system/node/node{node}/cpulist"
    if not os.path.exists(path):
        return requested
    wanted = parse_cpus(requested)
    local = parse_cpus(open(path).read().strip())
    if set(wanted) <= set(local):
        return requested
    pool = local[1:] if len(local) > len(wanted) else local
    if len(pool) < len(wanted):
        print(f"warning: node {node} has {len(local)} CPUs, fewer than --taskset={requested} asks for")
        return requested
    mapped = ",".join(str(cpu) for cpu in pool[: len(wanted)])
    print(f"taskset {requested} -> {mapped}: CPUs of numa node {node}, where memory is bound")
    return mapped


def main():
    parser = argparse.ArgumentParser(description="Run RoBin")
    parser.add_argument(
        "--index",
        required=True,
        choices=[
            "btree",
            "art",
            "alex",
            "lipp",
            "dytis",
            "dili",
            "pgm",
            "btreeolc",
            "artolc",
            "masstree",
            "alexolc",
            "lippolc",
            "xindex",
            "finedex",
            "sali",
        ],
        help="index type",
    )
    parser.add_argument(
        "--dataset",
        required=True,
        choices=["linear", "covid", "fb", "fb-1", "osm"],
        help="dataset name",
    )
    # parser.add_argument(
    #     "--dataset2",
    #     required=False,
    #     choices=["linear", "covid", "fb", "fb-1", "osm"],
    #     help="dataset2 name",
    # )
    parser.add_argument("--concurrency", required=True, default=1, help="concurrency")
    # parser.add_argument("--partition_num", required=False, default=1, help="partition num")
    # parser.add_argument("--partition_method", required=False, default="", help="partition method",choices=["range", "model", "naive"])
    parser.add_argument(
        "--sampling_method",
        required=True,
        # choices=["uniform", "segmented", "zipfian", "full"],
        choices=["uniform", "segmented", "full"],
        help="sampling method",
    )
    parser.add_argument("--bulkload_size", required=False, default=0, help="bulkload size")
    # parser.add_argument("--sampling_round", required=False, default=0, help="sampling round")
    parser.add_argument(
        "--insert_pattern",
        required=True,
        choices=["sorted", "shuffled"],
        help="insert pattern",
    )
    # parser.add_argument(
    #     "--mixed_rw", required=False, default=False, help="mix read-write ops"
    # )
    parser.add_argument(
        "--hardness_statistic", required=False, default=False, help="hardness statistic"
    )
    parser.add_argument("--taskset", required=False, help="taskset")
    parser.add_argument("--preload_suite", required=False, default=0, help="preload suite")
    parser.add_argument("--out_of_bound_inject", required=False, default=False, help="out_of_bound_inject suite")
    parser.add_argument("--tail_lat", required=False, default=False, help="tail latency")
    parser.add_argument(
        "--latency_sample_ratio",
        required=False,
        default=1,
        help="fraction of operations whose latency is recorded (1 = every operation)",
    )
    parser.add_argument(
        "--output_path",
        required=False,
        default="./out.csv",
        help="CSV file the benchmark appends its records to",
    )
    parser.add_argument(
        "--dataset_size",
        required=False,
        default=200000000,
        help="total number of keys of the dataset; used to derive init_table_ratio",
    )
    parser.add_argument(
        "--table_size",
        required=False,
        default=-1,
        help="cap on the number of loaded keys (-1 = load the whole dataset)",
    )
    
    args = parser.parse_args()

    numactl_arg = ""
    numanode = os.getenv("numanode")

    if os.path.isfile("/usr/bin/numactl"):
        if not numanode:
            print("Please specify the numa node to bind, e.g., export numanode=0")
            sys.exit(1)
        print(f"binding with numa node: {numanode}")
        numactl_arg = f"numactl --cpunodebind={numanode} --membind={numanode}"

    # 计算 init_table_ratio（相对数据集真实规模，支持 smoke 时缩小数据集）
    dataset_size = float(args.dataset_size)
    init_table_ratio = float(args.bulkload_size) / dataset_size

    test_suite = 0

    if args.sampling_method == "uniform":
        if args.insert_pattern == "sorted":
            test_suite = 21
            # if args.dataset2 is not None:
            #     test_suite = 8221
        elif args.insert_pattern == "shuffled":
            test_suite = 22
            # if args.dataset2 is not None:
            #     test_suite = 8222
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
        # if args.mixed_rw:
        #     test_suite *= 10
    elif args.sampling_method == "segmented":
        if args.insert_pattern == "sorted":
            test_suite = 41
        elif args.insert_pattern == "shuffled":
            test_suite = 42
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
        # if args.mixed_rw:
        #     test_suite *= 10
    # elif args.sampling_method == "zipfian":
    #     if args.insert_pattern == "sorted":
    #         test_suite = 81
    #     elif args.insert_pattern == "shuffled":
    #         test_suite = 82
    #     else:
    #         print(f"Unknown insert pattern: {args.insert_pattern}")
    #         sys.exit(1)
    elif args.sampling_method == "full":
        if int(args.bulkload_size) != int(dataset_size) or args.insert_pattern != "sorted":
            print(
                f"Full sampling only supports bulkload size equal to the dataset size ({int(dataset_size)}) and insert pattern sorted"
            )
            sys.exit(1)
        else:
            test_suite = 10
    else:
        print(f"Unknown sampling method: {args.sampling_method}")
        sys.exit(1)

    if args.taskset is None:
        args.taskset = ""
    else:
        if numactl_arg:
            args.taskset = cpus_on_node(args.taskset, numanode)
        args.taskset = "taskset -c " + args.taskset
    
    # keys_path2_arg=""
    # if args.dataset2 is not None:
    #     keys_path2_arg = f"--backup_keys_file=datasets/{args.dataset2}"

    hardness_statistic_arg = ""
    if args.hardness_statistic:
        hardness_statistic_arg = " --dataset_statistic=true"

    # partition_arg = ""
    # if args.partition_method == "range":
    #     partition_arg = f" --partition_num={args.partition_num} --partition_method=range"
    # elif args.partition_method == "model":
    #     partition_arg = f" --partition_num={args.partition_num} --partition_method=model"
    # elif args.partition_method == "naive":
    #     partition_arg = f" --partition_num={args.partition_num} --partition_method=naive"
    
    oob_arg = ""
    if args.out_of_bound_inject:
        oob_arg = " --out_of_bound_inject"
    
    latency_sampling_arg = ""
    if args.tail_lat:
        latency_sampling_arg = (
            f" --latency_sample --latency_sample_ratio={args.latency_sample_ratio}"
        )

    # command = f"{numactl_arg} {args.taskset} ./build/microbench --keys_file=datasets/{args.dataset} {keys_path2_arg}  --keys_file_type=binary {hardness_statistic_arg} --read=0.0 --insert=0.0 --update=0.0 --scan=0.0 --delete=0.0 --test_suite={test_suite} --operations_num=0 --table_size=-1 --init_table_ratio={init_table_ratio} --del_table_ratio=0.0 --sample_round={args.sampling_round} --thread_num={args.concurrency} {partition_arg} --index={args.index} --preload_suite={args.preload_suite} --memory {oob_arg} {latency_sampling_arg}"
    command = f"{numactl_arg} {args.taskset} ./build/microbench --keys_file=datasets/{args.dataset} --keys_file_type=binary {hardness_statistic_arg} --read=0.0 --insert=0.0 --update=0.0 --scan=0.0 --delete=0.0 --test_suite={test_suite} --operations_num=0 --table_size={args.table_size} --init_table_ratio={init_table_ratio} --del_table_ratio=0.0 --thread_num={args.concurrency} --index={args.index} --preload_suite={args.preload_suite} --memory {shlex.quote('--output_path=' + str(args.output_path))} {oob_arg} {latency_sampling_arg}"
    print(f"Running command: {command}")
    try:
        proc = subprocess.Popen(shlex.split(command))
        ret = proc.wait(timeout=1800)
        if ret != 0:
            proc.terminate()
            print(f"Subprocess failed with error code {ret}. Failed command: {command}")
    except subprocess.TimeoutExpired:
        proc.send_signal(signal.SIGTERM)
        proc.terminate()  # 强制终止子进程
        print(f"Subprocess timeout, kill it. Failed command: {command}")
    except subprocess.CalledProcessError as e:
        proc.terminate()  # 终止子进程
        print(
            f"Subprocess failed with error code {e.returncode}: {e}. Failed command: {command}"
        )
    finally:
        if proc.poll() is None:  # 检查是否仍在运行
            proc.kill()

if __name__ == "__main__":
    main()
