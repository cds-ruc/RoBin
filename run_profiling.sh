#!/bin/bash

batch=1

function BtreeBestTest {
    datasets=("covid" "osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 32
        do
            for init_table_ratio in 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1" "linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 31
        do
            for init_table_ratio in 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function BtreeWorstTest {
    datasets=("covid" "linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1" "osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.05
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=btree
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function ArtBestTest {
    datasets=("covid")
    for dataset in ${datasets[@]}
    do
        for test_suite in 22
        do
            for init_table_ratio in 0.01
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.005
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 42
        do
            for init_table_ratio in 0.005
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 21
        do
            for init_table_ratio in 0.01
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function ArtWorstTest {
    datasets=("covid")
    for dataset in ${datasets[@]}
    do
        for test_suite in 31
        do
            for init_table_ratio in 0.25
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        for test_suite in 32
        do
            for init_table_ratio in 0.05
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 31
        do
            for init_table_ratio in 0.05
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 21
        do
            for init_table_ratio in 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
                    --keys_file_type=binary \
                    --read=0.0 --insert=0.0 \
                    --update=0.0 --scan=0.0  --delete=0.0 \
                    --test_suite=$test_suite \
                    --operations_num=0 \
                    --table_size=-1 \
                    --init_table_ratio=$init_table_ratio \
                    --thread_num=1 \
                    --index=art
                done
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function LippBestTest {
    datasets=("covid" "fb-1" "osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 10
        do
            init_table_ratio=1.0
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
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
                --index=lipp
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 21
        do
            for init_table_ratio in 0.5
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function LippWorstTest {
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
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1" "linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 21
        do
            for init_table_ratio in 0.0
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.005
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function AlexBestTest {
    datasets=("covid")
    for dataset in ${datasets[@]}
    do
        for test_suite in 10
        do
            init_table_ratio=1.0
            for ((i=1; i<=batch; i++))
            do
                ./build/microbench \
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
                --index=alex
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        for test_suite in 22
        do
            for init_table_ratio in 0.01
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 32
        do
            for init_table_ratio in 0.005
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 42
        do
            for init_table_ratio in 0.05
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function AlexWorstTest {
    datasets=("covid" "linear")
    for dataset in ${datasets[@]}
    do
        for test_suite in 31
        do
            for init_table_ratio in 0.25
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("fb-1")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.05
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done

    datasets=("osm")
    for dataset in ${datasets[@]}
    do
        for test_suite in 41
        do
            for init_table_ratio in 0.1
            do
                for ((i=1; i<=batch; i++))
                do
                    ./build/microbench \
                    --keys_file=datasets/$dataset \
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
            done
        done
        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi
        mv *.log "$TARGET_DIR"
    done
}

function AlexCaseTest {
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
                    --keys_file=datasets/$dataset \
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
                TARGET_DIR="./log/alex/$dataset/$test_suite/$init_table_ratio"
                if [ ! -d "$TARGET_DIR" ]; then
                mkdir -p "$TARGET_DIR"
                fi
                mv *.log "$TARGET_DIR"
            done
        done
    done
}

BtreeBestTest
BtreeWorstTest
ArtBestTest
ArtWorstTest
LippBestTest
LippWorstTest
AlexBestTest
AlexWorstTest

AlexCaseTest
