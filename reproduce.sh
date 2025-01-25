#!/bin/bash
set -e
numanode=0
epoch_limit=1

# single thread test -> single_thread_thp.csv
function test_ro_thp_single_thread() {
  index_options=("btree" "art" "alex" "lipp" "dytis" "dili")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(1)
  
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
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/single_thread_thp.csv
  cat run.log | grep "Failed command:" > result/single_thread_fail_case.log
  rm run.log
}

# multi thread test -> multi_thread_thp.csv
function test_ro_thp_multi_thread() {
  index_options=("btreeolc" "artolc" "alexolc" "sali" "finedex")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(16)
  
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

              # Determine taskset CPUs based on concurrency
              if (( concurrency > 0 )); then
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/multi_thread_thp.csv
  cat run.log | grep "Failed command:" > result/multi_thread_fail_case.log
  rm run.log
}

# single thread latency test -> single_thread_tail_lat.csv
function test_ro_lat_single_thread() {
  index_options=("btree" "art" "alex" "lipp" "dytis" "dili")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(1)
  
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
              cmd="python3 run.py --index=$index --dataset=$dataset --concurrency=$concurrency --tail_lat=True"

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
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/single_thread_tail_lat.csv
  rm run.log
}

# multi thread latency test -> multi_thread_tail_lat.csv
function test_ro_lat_multi_thread() {
  index_options=("btreeolc" "artolc" "alexolc" "sali")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(16)
  
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
              cmd="python3 run.py --index=$index --dataset=$dataset --concurrency=$concurrency --tail_lat=True"

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
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/multi_thread_tail_lat.csv
  rm run.log
}

# extended bulkload_size single thread test -> single_thread_thp.alex_lipp_extended.csv
function test_ro_thp_single_thread_extended_bulksize() {
  index_options=("alex" "lipp")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("40000000" "60000000" "80000000" "120000000" "140000000" "160000000" "180000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(1)
  
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
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/single_thread_thp.alex_lipp_extended.csv
  rm run.log
}

# extended oob single thread test -> single_thread_thp.alex_oob.csv
function test_ro_thp_single_thread_extended_oob() {
  index_options=("alex")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency_options=(1)
  
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
              cmd="python3 run.py --index=$index --dataset=$dataset --concurrency=$concurrency --out_of_bound_inject=True"

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
                end_cpu=$((1 + concurrency - 1))
                cmd+=" --taskset=1-$end_cpu"
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
  mkdir -p result
  mv out.csv result/single_thread_thp.alex_oob.csv
  rm run.log
}

# Set the number of NUMA nodes
export numanode=$numanode

# Run the tests
test_ro_thp_single_thread
test_ro_thp_multi_thread
test_ro_lat_single_thread
test_ro_lat_multi_thread
test_ro_thp_single_thread_extended_bulksize
test_ro_thp_single_thread_extended_oob

echo "All tests completed successfully! Please check the result folder for the output files."