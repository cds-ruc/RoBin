#!/bin/bash

# Define the possible values for each parameter
index_options=("btreeolc" "artolc" "masstree" "alexolc" "lippolc" "xindex" "finedex" "dytis" "sali")
dataset_options=("linear" "covid" "fb-1" "osm")
sampling_method_options=("segmented" "uniform")
bulkload_size_options=("1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000")
insert_pattern_options=("sorted" "shuffled")
concurrency_options=(1 2 4 8 16)

# Iterate over all combinations of parameters
for index in "${index_options[@]}"; do
  for dataset in "${dataset_options[@]}"; do
    for sampling_method in "${sampling_method_options[@]}"; do
      for bulkload_size in "${bulkload_size_options[@]}"; do
        for insert_pattern in "${insert_pattern_options[@]}"; do
          for concurrency in "${concurrency_options[@]}"; do
            
            # Construct the base command
            cmd="python3 run.py --index $index --dataset $dataset --concurrency $concurrency"
            
            # Add optional parameters
            if [ -n "$sampling_method" ]; then
              cmd+=" --sampling_method $sampling_method"
            fi
            if [ -n "$bulkload_size" ]; then
              cmd+=" --bulkload_size $bulkload_size"
            fi
            if [ -n "$insert_pattern" ]; then
              cmd+=" --insert_pattern $insert_pattern"
            fi
            
            # Determine taskset CPUs based on concurrency, starting from CPU 2
            if (( concurrency > 0 )); then
              # Generate a comma-separated list from 2 up to (2 + concurrency - 1)
              end_cpu=$((2 + concurrency - 1))
              taskset=$(seq -s, 2 $end_cpu)
              cmd+=" --taskset $taskset"
            fi
            
            # Run the command 3 times
            for epoch in {1..3}; do
              echo "Running command (epoch $epoch): $cmd"
              eval "$cmd > run.log"
            done
          done
        done
      done
    done
  done
done
