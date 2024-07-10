#!/bin/bash

numanode=1
batch=1

# all_datasets=("covid" "osm" "fb" "genome" "planet" "linear")
all_datasets=("covid" "osm" "fb" "genome" "planet")

function BaselineTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 99
        do
            for init_table_ratio in 0.5
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
                    --index=btree,alex,lipp
                done
            done
        done
        mv *.log ./log/99/$dataset
        mv *.csv ./log/99/$dataset
    done
}

function LippBtreeBestTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 9999
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
                --index=btree,lipp
            done
        done
        
        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"
    done
}

function LippBtreeWorstTest {
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 8
        do
            for init_table_ratio in 0.005
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
                    --index=btree,lipp
                done
            done
        done

        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"
    done
}

function AlexBestTest {
    all_datasets=("covid" "osm")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 9999
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
                --index=alex
            done
        done

        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done


    all_datasets=("fb")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 12
        do
            for init_table_ratio in 0.005
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
                    --index=alex
                done
            done
        done

        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done


    all_datasets=("genome" "planet")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 99999
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
                --index=alex
            done
        done

        TARGET_DIR="./log/best/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done
}


function AlexWorstTest {
    all_datasets=("covid")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 12
        do
            for init_table_ratio in 0.25
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
                    --index=alex
                done
            done
        done

        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done


    all_datasets=("fb" "genome" "planet")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 8
        do
            for init_table_ratio in 0.005
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
                    --index=alex
                done
            done
        done

        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done


    all_datasets=("osm")
    for dataset in ${all_datasets[@]}
    do
        for test_suite in 7
        do
            for init_table_ratio in 0.005
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
                    --index=alex
                done
            done
        done

        TARGET_DIR="./log/worst/$dataset"
        if [ ! -d "$TARGET_DIR" ]; then
        mkdir -p "$TARGET_DIR"
        fi

        mv *.log "$TARGET_DIR"
        mv *.csv "$TARGET_DIR"/alex.csv
    done
}


# select test from input
case $1 in
    BaselineTest)
        BaselineTest
        ;;
    LippBtreeBestTest)
        LippBtreeBestTest
        ;;
    LippBtreeWorstTest)
        LippBtreeWorstTest
        ;;
    AlexBestTest)
        AlexBestTest
        ;;
    AlexWorstTest)
        AlexWorstTest
        ;;
    *)
        echo "Usage: $0 {BaselineTest|LippBtreeBestTest|LippBtreeWorstTest|AlexBestTest|AlexWorstTest}"
        exit 1
esac
