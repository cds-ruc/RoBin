#!/bin/bash

numanode=1
batch=1

# all_datasets=("covid")
# for dataset in ${all_datasets[@]}
# do
#     for test_suite in 99
#     do
#         for init_table_ratio in 0.5
#         do
#             for ((i=1; i<=batch; i++))
#             do
#                 numactl --cpunodebind=$numanode --membind=$numanode \
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
#                 --index=alex
#             done
#         done
#     done
# done

# all_datasets=("linear")
# for dataset in ${all_datasets[@]}
# do
#     for test_suite in 999
#     do
#         for init_table_ratio in 0.5
#         do
#             for ((i=1; i<=batch; i++))
#             do
#                 numactl --cpunodebind=$numanode --membind=$numanode \
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
#                 --index=alex
#             done
#         done
#     done
# done

# all_datasets=("fb")
# for dataset in ${all_datasets[@]}
# do
#     for test_suite in 8
#     do
#         for init_table_ratio in 0.01
#         do
#             for ((i=1; i<=batch; i++))
#             do
#                 numactl --cpunodebind=$numanode --membind=$numanode \
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
#                 --index=alex
#             done
#         done
#     done
# done