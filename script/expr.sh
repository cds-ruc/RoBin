#!/bin/bash

# Define the possible values for each parameter
index_options=("btreeolc" "artolc" "masstree" "alexolc" "lippolc" "xindex" "finedex" "dytis" "sali")
dataset_options=("linear" "covid" "fb-1" "osm")
sampling_method_options=("full" "uniform" "segmented")
bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
insert_pattern_options=("sorted" "shuffled")
concurrency_options=(1 2 4 8 16)

# Iterate over all combinations of parameters
for index in "${index_options[@]}"; do
  for dataset in "${dataset_options[@]}"; do
    for sampling_method in "${sampling_method_options[@]}"; do
      for bulkload_size in "${bulkload_size_options[@]}"; do
        for insert_pattern in "${insert_pattern_options[@]}"; do
          for concurrency in "${concurrency_options[@]}"; do
            if [ "$sampling_method" = "full" ] && { [ "$bulkload_size" != "200000000" ] || [ "$insert_pattern" != "sorted" ]; }; then
              # Skip full sampling with bulkload sizes other than 200M and insert patterns must be sorted
              continue
            fi

            if [ "$bulkload_size" = "0" ] && [ "$sampling_method" != "uniform" ]; then
              # Skip bulkload size 0 with sampling_method_options other than uniform
              continue
            fi

            # Construct the base command
            cmd="python3 run.py --index=$index --dataset=$dataset --concurrency=$concurrency"
            
            # Add optional parameters
            if [ -n "$sampling_method" ]; then
              cmd+=" --sampling_method=$sampling_method"
            fi
            if [ -n "$bulkload_size" ]; then
              cmd+=" --bulkload_size=$bulkload_size"
            fi
            if [ -n "$insert_pattern" ]; then
              cmd+=" --insert_pattern=$insert_pattern"
            fi
            
            # Determine taskset CPUs based on concurrency, starting from CPU 2
            if (( concurrency > 0 )); then
              # Generate a comma-separated list from 2 up to (2 + concurrency - 1)
              end_cpu=$((2 + concurrency - 1))
              cmd+=" --taskset=2-$end_cpu"
            fi
            
            # Run the command 3 times
            for epoch in {1..3}; do
              echo "Running command (epoch $epoch): $cmd"
              eval "$cmd >> run.log"
            done
          done
        done
      done
    done
  done
done