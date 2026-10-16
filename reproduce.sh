#!/bin/bash
# Full reproduction of every measurement grid of the paper.
#
#   bash reproduce.sh                              # every grid, in order
#   bash reproduce.sh test_ro_thp_single_thread    # selected grids only
#
# RESULT_DIR (default: result/) receives the CSVs under the names the plotting
# notebooks read; numanode (default 0) selects the NUMA node to bind to.
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"
numanode=${numanode:-0}
epoch_limit=1
RESULT_DIR=${RESULT_DIR:-result}

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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/single_thread_thp.csv"
  grep "Failed command:" run.log > "$RESULT_DIR/single_thread_fail_case.log" || true
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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/multi_thread_thp.csv"
  grep "Failed command:" run.log > "$RESULT_DIR/multi_thread_fail_case.log" || true
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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/single_thread_tail_lat.csv"
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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/multi_thread_tail_lat.csv"
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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/single_thread_thp.alex_lipp_extended.csv"
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
  mkdir -p "$RESULT_DIR"
  mv out.csv "$RESULT_DIR/single_thread_thp.alex_oob.csv"
  rm run.log
}

# data augmentation single thread test (paper Section 6.1, Figure 16)
#   preload_suite 33 -> 1% keys from the uint64 key space   -> single_thread_thp.alex_lipp_preload_1_100_uint64.csv
#   preload_suite 34 -> 1% keys from the dataset key space  -> single_thread_thp.alex_lipp_preload_1_100_dataset.csv
#   (23/24 are the same with 10% and are not plotted in the paper)
# The artificial keys are bulkloaded with the real ones and deleted before the
# insert/lookup phases; the figure compares lookups against single_thread_thp.csv.
function test_ro_thp_single_thread_data_augmentation() {
  index_options=("alex" "lipp")
  dataset_options=("linear" "covid" "fb-1" "osm")
  sampling_method_options=("full" "uniform" "segmented")
  bulkload_size_options=("0" "1000000" "2000000" "5000000" "10000000" "20000000" "50000000" "100000000" "200000000")
  insert_pattern_options=("sorted" "shuffled")
  concurrency=1
  declare -A preload_outputs=(
    [33]="single_thread_thp.alex_lipp_preload_1_100_uint64.csv"
    [34]="single_thread_thp.alex_lipp_preload_1_100_dataset.csv"
  )

  for preload_suite in 33 34; do
    for index in "${index_options[@]}"; do
      for dataset in "${dataset_options[@]}"; do
        for sampling_method in "${sampling_method_options[@]}"; do
          for bulkload_size in "${bulkload_size_options[@]}"; do
            for insert_pattern in "${insert_pattern_options[@]}"; do
              if [ "$bulkload_size" = "200000000" ] && [ "$sampling_method" != "full" ]; then
                continue
              fi
              if [ "$sampling_method" = "full" ] && { [ "$bulkload_size" != "200000000" ] || [ "$insert_pattern" != "sorted" ]; }; then
                continue
              fi
              if [ "$bulkload_size" = "0" ] && [ "$sampling_method" != "uniform" ]; then
                continue
              fi

              cmd="python3 run.py --index=$index --dataset=$dataset --concurrency=$concurrency --preload_suite=$preload_suite"
              cmd+=" --sampling_method=$sampling_method --bulkload_size=$bulkload_size --insert_pattern=$insert_pattern"
              cmd+=" --taskset=1-1"

              for epoch in $(seq 1 $epoch_limit); do
                  echo "Running command (epoch $epoch): $cmd"
                  eval "$cmd >> run.log"
              done
            done
          done
        done
      done
    done
    mkdir -p "$RESULT_DIR"
    mv out.csv "$RESULT_DIR/${preload_outputs[$preload_suite]}"
  done
  rm run.log
}

# Set the number of NUMA nodes
export numanode=$numanode

ALL_TESTS=(
  test_ro_thp_single_thread
  test_ro_thp_multi_thread
  test_ro_lat_single_thread
  test_ro_lat_multi_thread
  test_ro_thp_single_thread_extended_bulksize
  test_ro_thp_single_thread_extended_oob
  test_ro_thp_single_thread_data_augmentation
)

# Run the tests: the ones named on the command line, or all of them
tests=("$@")
[ ${#tests[@]} -eq 0 ] && tests=("${ALL_TESTS[@]}")
for test in "${tests[@]}"; do
  if ! declare -F "$test" > /dev/null; then
    echo "unknown test: $test (one of: ${ALL_TESTS[*]})" >&2
    exit 1
  fi
  # run.py appends to out.csv; never let a previous, interrupted run leak in
  rm -f out.csv run.log
  echo "=== $test -> $RESULT_DIR ($(date '+%F %T'))"
  "$test"
done

echo "All tests completed successfully! Please check $RESULT_DIR for the output files."