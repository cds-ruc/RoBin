#!/bin/bash

numanode=1
batch=3

all_datasets=("covid" "osm" "fb" "genome" "planet" "linear")


function BaselineTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 10
        do
            init_table_ratio = 1.0
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

function UniformSamplingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 21 22
        do
            for init_table_ratio in 0.0 0.005 0.01 0.025 0.05 0.1 0.25 0.5
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

function IntervalSampingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 35 # 31 32 33 34 35
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

function SubstringSamplingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 41 42 51 52 61 62 71 72
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

function ZipfSamplingTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 81 82 91 92 101 102
        do
            for sample_round in 100000000 # this is a fixed number 100M
            do
                for zipfian_constant in 0.99 # 0.8 0.4 0.2 0.01
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
                        --init_table_ratio=0 \
                        --sample_round=$sample_round \
                        --zipfian_constant=$zipfian_constant \
                        --thread_num=1 \
                        --index=btree,art,alex,lipp
                    done
                done
            done
        done
    done
}

# function NormalSamplingTest {
#     for dataset in ${all_datasets[@]}
#     do
#         for test_suite in 111 112
#         do
#             for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
#             do
#                 for ((i=1; i<=batch; i++))
#                 do
#                     numactl --cpunodebind=$numanode --membind=$numanode \
#                     nice -n -10 ./build/microbench \
#                     --keys_file=datasets/$dataset \
#                     --keys_file_type=binary \
#                     --read=0.0 --insert=0.0 \
#                     --update=0.0 --scan=0.0  --delete=0.0 \
#                     --test_suite=$test_suite \
#                     --operations_num=0 \
#                     --table_size=-1 \
#                     --init_table_ratio=$init_table_ratio \
#                     --thread_num=1 \
#                     --sigma_ratio=0.2 \
#                     --index=btree,art,alex,lipp
#                 done
#             done
#         done
#     done
# }

# function ShiftTest {
#     all_datasets=("covid" "libio" "osm" "fb")
#     for bulk_dataset in ${all_datasets[@]}
#     do
#         for insert_dataset in ${all_datasets[@]}
#         do
#             if [ $bulk_dataset == $insert_dataset ]; then
#                 continue
#             fi
#             test_suite=10
#             init_table_ratio=0.5
#             for ((i=1; i<=batch; i++))
#             do
#                 numactl --cpunodebind=$numanode --membind=$numanode \
#                 nice -n -10 ./build/microbench \
#                 --keys_file=datasets/$bulk_dataset \
#                 --backup_keys_file=datasets/$insert_dataset \
#                 --keys_file_type=binary \
#                 --read=0.0 --insert=0.0 \
#                 --update=0.0 --scan=0.0  --delete=0.0 \
#                 --test_suite=$test_suite \
#                 --operations_num=0 \
#                 --table_size=-1 \
#                 --init_table_ratio=$init_table_ratio \
#                 --thread_num=1 \
#                 --index=alex,lipp
#             done
#         done
#     done
# }

# select test from input
case $1 in
    BaselineTest)
        BaselineTest
        ;;
    UniformSamplingTest)
        UniformSamplingTest
        ;;
    IntervalSampingTest)
        IntervalSampingTest
        ;;
    SubstringSamplingTest)  
        SubstringSamplingTest
        ;;
    ZipfSamplingTest)
        ZipfSamplingTest
        ;;

    *)
        echo "Usage: $0 {BaselineTest|UniformSamplingTest|IntervalSampingTest|SubstringSamplingTest|ZipfSamplingTest}"
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