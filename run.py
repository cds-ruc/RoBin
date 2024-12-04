import os
import sys
import subprocess
import argparse


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
    parser.add_argument(
        "--dataset2",
        required=False,
        choices=["linear", "covid", "fb", "fb-1", "osm"],
        help="dataset2 name",
    )
    parser.add_argument("--concurrency", required=True, default=1, help="concurrency")
    parser.add_argument(
        "--sampling_method",
        required=True,
        choices=["uniform", "segmented", "zipfian", "full"],
        help="sampling method",
    )
    parser.add_argument("--bulkload_size", required=False, default=0, help="bulkload size")
    parser.add_argument("--sampling_round", required=False, help="sampling round")
    parser.add_argument(
        "--insert_pattern",
        required=True,
        choices=["sorted", "shuffled"],
        help="insert pattern",
    )
    parser.add_argument(
        "--mixed_rw", required=False, default=False, help="mix read-write ops"
    )
    parser.add_argument(
        "--hardness_statistic", required=False, default=False, help="hardness statistic"
    )
    parser.add_argument("--taskset", required=False, help="taskset")

    args = parser.parse_args()

    numactl_arg = ""
    numanode = os.getenv("numanode")

    if os.path.isfile("/usr/bin/numactl"):
        if not numanode:
            print("Please specify the numa node to bind, e.g., export numanode=0")
            sys.exit(1)
        print(f"binding with numa node: {numanode}")
        numactl_arg = f"numactl --cpunodebind={numanode} --membind={numanode}"

    # 计算 init_table_ratio
    init_table_ratio = float(args.bulkload_size) / 200000000

    test_suite = 0

    if args.sampling_method == "uniform":
        if args.insert_pattern == "sorted":
            test_suite = 21
            if args.dataset2 is not None:
                test_suite = 8221
        elif args.insert_pattern == "shuffled":
            test_suite = 22
            if args.dataset2 is not None:
                test_suite = 8222
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
        if args.mixed_rw:
            test_suite *= 10
    elif args.sampling_method == "segmented":
        if args.insert_pattern == "sorted":
            test_suite = 41
        elif args.insert_pattern == "shuffled":
            test_suite = 42
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
        if args.mixed_rw:
            test_suite *= 10
    elif args.sampling_method == "zipfian":
        if args.insert_pattern == "sorted":
            test_suite = 81
        elif args.insert_pattern == "shuffled":
            test_suite = 82
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
    elif args.sampling_method == "full":
        if args.bulkload_size != "200000000" or args.insert_pattern != "sorted":
            print(
                f"Full sampling only supports bulkload size 200000000 and insert pattern sorted"
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
        args.taskset = "taskset -c " + args.taskset
    
    keys_path2_arg=""
    if args.dataset2 is not None:
        keys_path2_arg = f"--backup_keys_file=datasets/{args.dataset2}"

    hardness_statistic_arg = ""
    if args.hardness_statistic:
        hardness_statistic_arg = " --dataset_statistic=true"

    command = f"{numactl_arg} {args.taskset} ./build/microbench --keys_file=datasets/{args.dataset} {keys_path2_arg}  --keys_file_type=binary {hardness_statistic_arg} --read=0.0 --insert=0.0 --update=0.0 --scan=0.0 --delete=0.0 --test_suite={test_suite} --operations_num=0 --table_size=-1 --init_table_ratio={init_table_ratio} --del_table_ratio=0.0 --sampling_round={args.sampling_round} --thread_num={args.concurrency} --index={args.index}"
    print(f"Running command: {command}")
    try:
        proc = subprocess.Popen(command.split())
        ret = proc.wait(timeout=1800)
        if ret != 0:
            print(f"Subprocess failed with error code {ret}. Failed command: {command}")
            proc.terminate()
    except subprocess.TimeoutExpired:
        print(f"Subprocess timeout, kill it. Failed command: {command}")
        proc.kill()  # 强制终止子进程
    except subprocess.CalledProcessError as e:
        print(
            f"Subprocess failed with error code {e.returncode}: {e}. Failed command: {command}"
        )
        proc.terminate()  # 终止子进程
    finally:
        if proc.poll() is None:  # 检查是否仍在运行
            proc.kill()


if __name__ == "__main__":
    main()
