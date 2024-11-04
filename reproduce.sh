#!/bin/bash

# batch is the number of runs for each test
batch=5

numactl_arg=""
if [ -f /usr/bin/numactl ]; then
    # if detected numa but not specified bind, throw an error
    if [ -z "$numanode" ]; then
        echo "Please specify the numa node to bind, e.g., export numanode=0"
        exit 1
    fi
    echo "binding with numa node: $numanode"
    numactl_arg="numactl --cpunodebind=$numanode --membind=$numanode"
fi

datasets=("linear" "covid" "fb-1" "osm")

indexs="btree,art,alex,lipp,dytis,dili"

for dataset in ${datasets[@]}
do
    for test_suite in 10
    do
        init_table_ratio=1.0
        for ((i=1; i<=batch; i++))
        do
            timeout 30m \
            $numactl_arg ./build/microbench \
            --keys_file=datasets/$dataset \
            --keys_file_type=binary \
            --read=0.0 --insert=0.0 \
            --update=0.0 --scan=0.0  --delete=0.0 \
            --test_suite=$test_suite \
            --operations_num=0 \
            --table_size=-1 \
            --init_table_ratio=$init_table_ratio \
            --thread_num=1 \
            --memory=true \
            --index=$indexs
        done
    done

    for test_suite in 21 22
    do
        for init_table_ratio in 0.0 0.005 0.01 0.025 0.05 0.1 0.25 0.5
        do
            for ((i=1; i<=batch; i++))
            do
                timeout 30m \
                $numactl_arg ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=0.0 --insert=0.0 \
                --update=0.0 --scan=0.0  --delete=0.0 \
                --test_suite=$test_suite \
                --operations_num=0 \
                --table_size=-1 \
                --init_table_ratio=$init_table_ratio \
                --thread_num=1 \
                --index=$indexs
            done
        done
    done

    for test_suite in 41 42
    do
        for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
        do
            for ((i=1; i<=batch; i++))
            do
                timeout 30m \
                $numactl_arg ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=0.0 --insert=0.0 \
                --update=0.0 --scan=0.0  --delete=0.0 \
                --test_suite=$test_suite \
                --operations_num=0 \
                --table_size=-1 \
                --init_table_ratio=$init_table_ratio \
                --thread_num=1 \
                --index=$indexs
            done
        done
    done
done