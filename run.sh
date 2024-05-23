#!/bin/bash

# custom_var_suite 0 baseline 插入，什么也不做
# custom_var_suite 1 随机起点的append only bulk load
# custom_var_suite 2 数据集起点+最后一个key的append only bulk load
# custom_var_suite 3 在0基础上的lookup only
# custom_var_suite 4 在1基础上的lookup only
# custom_var_suite 5 在2基础上的lookup only
# custom_var_suite 6 density gen


# 定义数据集
for dataset in "covid" "osm" "fb" "linear" "prime"
do
    for test_suite in 0 1 2 3 4 5 6
        ./build/microbench \
        --keys_file=datasets/$dataset \
        --keys_file_type=binary \
        --read=0.0 --insert=0.0 \
        --update=0.0 --scan=0.0  --delete=0.0 \
        --test_suite=1 \
        --operations_num=0 \
        --table_size=-1 \
        --init_table_ratio=0.5 \
        --thread_num=1 \
        --index=btreeolc,artolc,alex,lipp
    done
done


# 定义执行次数
batch=3
# 定义table_size
dataset_size=200000000
# 定义并发
thread_num=1
# 定义读负载大小
read_num=200000000
# 定义索引
for index in "alex" "lipp" "btreeolc" "artolc"
do
    # 定义数据集
    for dataset in "osm" "covid" "fb" "linear" "prime"
    do
        # 定义bulk_size
        for bulk_size in 1000000 2000000 5000000 10000000 20000000 50000000 100000000
        do
            # suite 0
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=0.0 \
                --insert=1.0 \
                --operations_num=$((dataset_size - bulk_size)) \
                --table_size=-1 \
                --init_table_ratio=$(echo "scale=5; $bulk_size / $dataset_size" | bc) \
                --thread_num=$thread_num \
                --index=$index
            done

            # suite 1
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=1.0 \
                --operations_num=0 \
                --table_size=-1 \
                --custom_var_suite=1 \
                --custom_var_bulk_size=$bulk_size \
                --thread_num=$thread_num \
                --index=$index
            done

            # suite 2
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=1.0 \
                --operations_num=0 \
                --table_size=-1 \
                --custom_var_suite=2 \
                --custom_var_bulk_size=$((bulk_size - 1)) \
                --thread_num=$thread_num \
                --index=$index
            done

            # suite 3
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=1.0 \
                --insert=0.0 \
                --operations_num=$read_num \
                --table_size=-1 \
                --custom_var_suite=3 \
                --init_table_ratio=$(echo "scale=5; $bulk_size / $dataset_size" | bc) \
                --thread_num=$thread_num \
                --index=$index
            done

            # suite 4
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=1.0 \
                --operations_num=$read_num \
                --table_size=-1 \
                --custom_var_suite=4 \
                --custom_var_bulk_size=$bulk_size \
                --thread_num=$thread_num \
                --index=$index
            done

            # suite 5
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=datasets/$dataset \
                --keys_file_type=binary \
                --read=1.0 \
                --operations_num=$read_num \
                --table_size=-1 \
                --custom_var_suite=5 \
                --custom_var_bulk_size=$((bulk_size - 1)) \
                --thread_num=$thread_num \
                --index=$index
            done
        done
    done
done
