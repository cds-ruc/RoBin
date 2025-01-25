#!/bin/bash
batch=1

function AlexCaseTest {
    # overfit analysis - fb worst
    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        for test_suite in 22
        do
            for init_table_ratio in 0.5
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
                    --index=alex
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
    # unbalanced structure analysis - osm best vs. worst
    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        # best
        for test_suite in 42
        do
            for init_table_ratio in 0.05
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
                    --index=alex
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
        # worst
        for test_suite in 41
        do
            for init_table_ratio in 0.1
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
                    --index=alex
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
    # smo analysis - covid
    datasets=("covid")
    for dataset in ${datasets[@]}
    do
        for test_suite in 21 22
        do
            for init_table_ratio in 0.5
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
                    --index=alex
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
}

function LippCaseTest {
    # overfit analysis - covid worst
    datasets=("covid")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.01
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
                    --index=lipp
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
    # unbalanced structure analysis - osm best vs. worst
    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        # best
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
                --index=lipp
            done
            TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
            if [ ! -d "$TARGET_DIR" ]; then
            mkdir -p "$TARGET_DIR"
            fi
            mv *.log "$TARGET_DIR"
        done
        # worst
        for test_suite in 41
        do
            for init_table_ratio in 0.005
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
                    --index=lipp
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
}

function MemoryBreakdown {
    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        # normal case
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
                --memory=true \
                --index=alex,lipp,alexolc,sali
            done
            TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
            if [ ! -d "$TARGET_DIR" ]; then
            mkdir -p "$TARGET_DIR"
            fi
            mv *.log "$TARGET_DIR"
        done
        # abnormal case - alex
        for test_suite in 42
        do
            for init_table_ratio in 0.5
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
                    --memory=true \
                    --index=alex
                done
                TARGET_DIR="./log/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
}

AlexCaseTest
LippCaseTest
MemoryBreakdown
