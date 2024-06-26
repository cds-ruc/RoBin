#!/bin/bash

numanode=1
batch=3

all_datasets=("osm" "fb" "genome" "planet")

function BaselineTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 9999 99999 999999
        do
            if [ $test_suite -eq 9999 ]; then
                init_table_ratio=1.0
            else
                init_table_ratio=0.0
            fi
            for ((i=1; i<=batch; i++))
            do
                numactl --cpunodebind=$numanode --membind=$numanode \
                nice -n -10 ./build/microbench \
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
                --index=btree,art,alex,lipp
            done
        done
    done
}

function SortedBulkTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 99 999 1 2 3
        do
            for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    numactl --cpunodebind=$numanode --membind=$numanode \
                    nice -n -10 ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree,art,alex,lipp
                done
            done
        done
    done
}

function NormalSamplingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 6 9
        do
            for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    numactl --cpunodebind=$numanode --membind=$numanode \
                    nice -n -10 ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --sigma_ratio=0.2 \
                    --index=btree,art,alex,lipp
                done
            done
        done
    done
}

function AppendTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 7 8
        do
            for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    numactl --cpunodebind=$numanode --membind=$numanode \
                    nice -n -10 ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree,art,alex,lipp
                done
            done
        done
    done
}

function IntervalTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 11 12 13
        do
            for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
            # equvilant to gap_size in 200 100 40 20 10 4 2
            do
                for ((i=1; i<=batch; i++))
                do
                    numactl --cpunodebind=$numanode --membind=$numanode \
                    nice -n -10 ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree,art,alex,lipp
                done
            done
        done
    done
}

function ZipfSamplingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 20 21 22 23 24 25
        do
            # for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
            for sampling_round in 1 2 3 4 5
            do
                for ((i=1; i<=batch; i++))
                do
                    numactl --cpunodebind=$numanode --membind=$numanode \
                    nice -n -10 ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree,art,alex,lipp
                done
            done
        done
    done
}

function ShiftTest {
    all_datasets=("covid" "libio" "osm" "fb")
    for bulk_dataset in ${all_datasets[@]}
    do
        for insert_dataset in ${all_datasets[@]}
        do
            if [ $bulk_dataset == $insert_dataset ]; then
                continue
            fi
            test_suite=10
            init_table_ratio=0.5
            for ((i=1; i<=batch; i++))
            do
                numactl --cpunodebind=$numanode --membind=$numanode \
                nice -n -10 ./build/microbench \
                --keys_file=datasets/$bulk_dataset \
                --backup_keys_file=datasets/$insert_dataset \
                --keys_file_type=binary \
                --read=0.0 --insert=0.0 \
                --update=0.0 --scan=0.0  --delete=0.0 \
                --test_suite=$test_suite \
                --operations_num=0 \
                --table_size=-1 \
                --init_table_ratio=$init_table_ratio \
                --thread_num=1 \
                --index=alex,lipp
            done
        done
    done
}

# select test from input
case $1 in
    BaselineTest)
        BaselineTest
        ;;
    SortedBulkTest)
        SortedBulkTest
        ;;
    NormalSamplingTest)
        NormalSamplingTest
        ;;
    AppendTest)
        AppendTest
        ;;
    IntervalTest)
        IntervalTest
        ;;
    ZipfSamplingTest)   
        ZipfSamplingTest
        ;;
    ShiftTest)
        ShiftTest
        ;;
    *)
        echo "Usage: $0 {BaselineTest|SortedBulkTest|NormalSamplingTest|AppendTest|IntervalTest|ZipfSamplingTest|ShiftTest}"
        exit 1
esac


# # Dive into the fb/alex
# for dataset in "fb"
# do
#     for test_suite in 99 2 4
#     do
#         for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
#         do
#             for ((i=1; i<=batch; i++))
#             do
#                 nice -n -10 ./build/microbench \
#                 --keys_file=datasets/$dataset \
#                 --keys_file_type=binary \
#                 --read=0.0 --insert=0.0 \
#                 --update=0.0 --scan=0.0  --delete=0.0 \
#                 --test_suite=$test_suite \
#                 --operations_num=0 \
#                 --table_size=-1 \
#                 --init_table_ratio=$init_table_ratio \
#                 --thread_num=1 \
#                 --memory=true \
#                 --dump_bulkload=true \
#                 --index=alex
#             done
#         done
#     done
# done