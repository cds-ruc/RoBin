#!/bin/bash

# Define the possible values for each parameter
index_options=("btreeolc" "artolc" "alexolc" "finedex" "dytis" "sali")
dataset_options=("linear" "covid" "fb-1" "osm")
sampling_method_options=("full" "uniform" "segmented")
bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
insert_pattern_options=("sorted" "shuffled")
concurrency_options=(1)
numanode=0
epoch_limit=1
# export numanode=$numanode

function test_shift_multi_thread(){
  concurrency_options=(16)
  sampling_method_options=("uniform" "segmented")
  bulkload_size_options=("100000000")
  # Iterate over all combinations of parameters
  for index in "${index_options[@]}"; do
    for dataset in "${dataset_options[@]}"; do
      for dataset2 in "${dataset_options[@]}"; do
        for sampling_method in "${sampling_method_options[@]}"; do
          for bulkload_size in "${bulkload_size_options[@]}"; do
            for insert_pattern in "${insert_pattern_options[@]}"; do
              for concurrency in "${concurrency_options[@]}"; do
                if [ $dataset == $dataset2 ]; then
                  continue
                fi
                # Construct the base command
                cmd="python3 run.py --index=$index --dataset=$dataset --dataset2=$dataset2 --concurrency=$concurrency"
                
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
                  if (( numanode == 0 )); then
                    end_cpu=$((2 + concurrency - 1))
                    cmd+=" --taskset=2-$end_cpu"
                  else
                    end_cpu=$((20 + concurrency - 1))
                    cmd+=" --taskset=20-$end_cpu"
                  fi
                fi
                
                # Run the command loop $epoch_limit times
                for epoch in $(seq 1 $epoch_limit); do
                  echo "Running command (epoch $epoch): $cmd"
                  eval "$cmd >> run.log"
                done
              done
            done
          done
        done
      done
    done
  done
}

function test_rw_multi_thread() {
  concurrency_options=(1 2 4 8 16)
  bulkload_size_options=("0" "100000000" "200000000")
  # Iterate over all combinations of parameters
  for index in "${index_options[@]}"; do
    for dataset in "${dataset_options[@]}"; do
      for sampling_method in "${sampling_method_options[@]}"; do
        for bulkload_size in "${bulkload_size_options[@]}"; do
          for insert_pattern in "${insert_pattern_options[@]}"; do
            for concurrency in "${concurrency_options[@]}"; do
              if [ "$bulkload_size" = "200000000" ] && [ "$sampling_method" != "full" ]; then
                # Skip bulkload size 200M with sampling method other than full
                continue
              fi
              
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
                if (( numanode == 0 )); then
                  end_cpu=$((2 + concurrency - 1))
                  cmd+=" --taskset=2-$end_cpu"
                else
                  end_cpu=$((20 + concurrency - 1))
                  cmd+=" --taskset=20-$end_cpu"
                fi
              fi
              
              cmd+=" --mixed_rw=1"
              
              # Run the command 3 times
              for epoch in $(seq 1 $epoch_limit); do
                echo "Running command (epoch $epoch): $cmd"
                eval "$cmd >> run.log"
              done
            done
          done
        done
      done
    done
  done
}

function test_ro_multi_thread() {
  concurrency_options=(1 2 4 8 16)
  bulkload_size_options=("0" "100000000" "200000000")
  # Iterate over all combinations of parameters
  for index in "${index_options[@]}"; do
    for dataset in "${dataset_options[@]}"; do
      for sampling_method in "${sampling_method_options[@]}"; do
        for bulkload_size in "${bulkload_size_options[@]}"; do
          for insert_pattern in "${insert_pattern_options[@]}"; do
            for concurrency in "${concurrency_options[@]}"; do
              if [ "$bulkload_size" = "200000000" ] && [ "$sampling_method" != "full" ]; then
                # Skip bulkload size 200M with sampling method other than full
                continue
              fi
              
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
                if (( numanode == 0 )); then
                  end_cpu=$((2 + concurrency - 1))
                  cmd+=" --taskset=2-$end_cpu"
                else
                  end_cpu=$((20 + concurrency - 1))
                  cmd+=" --taskset=20-$end_cpu"
                fi
              fi
              
              # Run the command 3 times
              for epoch in $(seq 1 $epoch_limit); do
                echo "Running command (epoch $epoch): $cmd"
                eval "$cmd >> run.log"
              done
            done
          done
        done
      done
    done
  done
}

test_shift_multi_thread;