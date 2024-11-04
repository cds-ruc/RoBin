import os
import sys
import subprocess
import argparse

def main():
    parser = argparse.ArgumentParser(description="Run RoBin")
    parser.add_argument('--index', required=True,choices=['btree', 'art', 'alex','lipp','dytis','dili','pgm','btreeolc','artolc','masstree','alexolc','lippolc','xindex','finedex','sali'], help='index type')
    parser.add_argument('--dataset', required=True,choices=['linear','covid','fb','fb-1','osm'], help='dataset name')
    parser.add_argument('--concurrency', required=True, default=1, help='concurrency')
    parser.add_argument('--sampling_method', required=True, choices=['uniform', 'segmented', 'full'], help='sampling method')
    parser.add_argument('--bulkload_size', required=True, help='bulkload size')
    parser.add_argument('--insert_pattern', required=True, choices=['sorted', 'shuffled'], help='insert pattern')
    parser.add_argument('--taskset', help='taskset')

    args = parser.parse_args()

    numactl_arg = ""
    numanode = os.getenv('numanode')

    if os.path.isfile('/usr/bin/numactl'):
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
        elif args.insert_pattern == "shuffled":
            test_suite = 22
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
    elif args.sampling_method == "segmented":
        if args.insert_pattern == "sorted":
            test_suite = 41
        elif args.insert_pattern == "shuffled":
            test_suite = 42
        else:
            print(f"Unknown insert pattern: {args.insert_pattern}")
            sys.exit(1)
    elif args.sampling_method == "full":
        if args.bulkload_size != "200000000" or args.insert_pattern != "sorted":
            print(f"Full sampling only supports bulkload size 200000000 and insert pattern sorted")
            sys.exit(1)
        else:
            test_suite = 10
    else:
        print(f"Unknown sampling method: {args.sampling_method}")
        sys.exit(1)

    if args.taskset is None:
        args.taskset = ""
    else:
        args.taskset = "taskset -c "+args.taskset

    command = f'{numactl_arg} {args.taskset} ./build/microbench --keys_file=datasets/{args.dataset} --keys_file_type=binary --dataset_statistic=true --read=0.0 --insert=0.0 --update=0.0 --scan=0.0 --delete=0.0 --test_suite={test_suite} --operations_num=0 --table_size=-1 --init_table_ratio={init_table_ratio} --del_table_ratio=0.0 --thread_num={args.concurrency} --index={args.index}'
    print(f"Running command: {command}")
    try:
        subprocess.run(command, shell=True, timeout=1800)
    except subprocess.TimeoutExpired:
        print("Timeout case, fail cmd is: ", command)
if __name__ == "__main__":
    main()