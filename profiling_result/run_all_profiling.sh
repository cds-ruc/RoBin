#!/bin/bash

datasets=("linear" "covid" "fb-1" "osm")
indexs="alex,lipp,alexolc,sali"
batch=1

for dataset in ${datasets[@]}
do
    for test_suite in 10
    do
        init_table_ratio=1.0
        for ((i=1; i<=batch; i++))
        do
            ./build/microbench \
            --keys_file=/home/dataset/gre_dataset/$dataset \
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
        TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    for test_suite in 21 22
    do
        for init_table_ratio in 0.0 0.005 0.01 0.025 0.05 0.1 0.25 0.5
        do
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=/home/dataset/gre_dataset/$dataset \
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
            TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
            if [ ! -d "$TARGET_DIR" ]; then
            mkdir -p "$TARGET_DIR"
            fi
            mv *.log "$TARGET_DIR"
        done
    done

    for test_suite in 41 42
    do
        for init_table_ratio in 0.005 0.01 0.025 0.05 0.1 0.25 0.5
        do
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
                --keys_file=/home/dataset/gre_dataset/$dataset \
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
            TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
            if [ ! -d "$TARGET_DIR" ]; then
            mkdir -p "$TARGET_DIR"
            fi
            mv *.log "$TARGET_DIR"
        done
    done
done
