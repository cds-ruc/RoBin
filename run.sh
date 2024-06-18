#!/bin/bash

batch=1

# for dataset in "fb" "genome" "planet" "covid"  "osm" # "linear"
# do
#     for test_suite in 99 9999 1 2 3 4 6 7 8
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
#                 --index=btreeolc,artolc,alex,lipp
#             done
#         done
#     done
# done


# # Baseline workload 999
# for dataset in "covid"  "osm" "fb" "genome" "planet"
# do
#     for test_suite in 999
#     do
#         for init_table_ratio in 1
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
#                 --index=btree,artunsync,alex,lipp
#             done
#         done
#     done
# done


# Dive into the fb/alex
for dataset in "fb"
do
    for test_suite in 99 2 4
    do
        for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
        do
            for ((i=1; i<=batch; i++))
            do
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
                --dump_bulkload=true \
                --index=alex
            done
        done
    done
done