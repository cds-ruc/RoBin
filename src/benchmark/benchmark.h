#include "../competitor/competitor.h"
#include "../competitor/indexInterface.h"
#include "../tscns.h"
#include "flags.h"
#include "omp.h"
#include "pgm_metric.h"
#include "tbb/parallel_sort.h"
#include "utils.h"
#include <algorithm>
#include <boost/random.hpp>
#include <boost/random/normal_distribution.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <jemalloc/jemalloc.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_set>
#include <vector>

template <typename KEY_TYPE, typename PAYLOAD_TYPE> class Benchmark {
  typedef indexInterface<KEY_TYPE, PAYLOAD_TYPE> index_t;

  enum Operation { READ = 0, INSERT, DELETE, SCAN, UPDATE };

  // parameters
  double read_ratio = 1;
  double insert_ratio = 0;
  double delete_ratio = 0;
  double update_ratio = 0;
  double scan_ratio = 0;
  size_t scan_num = 100;
  size_t operations_num;
  long long table_size = -1;
  size_t init_table_size;
  int sample_round;
  double init_table_ratio;
  double del_table_ratio;
  size_t thread_num = 1;
  std::vector<std::string> all_index_type;
  std::vector<std::string> all_thread_num;
  std::string index_type;
  std::string keys_file_path;
  std::string backup_keys_file_path;
  std::string keys_file_type;
  std::string sample_distribution;
  bool latency_sample = false;
  double latency_sample_ratio = 0.01;
  int error_bound;
  std::string output_path;
  size_t random_seed;
  bool memory_record;
  bool dataset_statistic;
  bool data_shift = false;
  int test_suite = 0;
  bool dump_bulkload = false;
  double sigma_ratio = 0.5;
  double zipfian_constant = 0.99;

  std::vector<KEY_TYPE> init_keys;
  KEY_TYPE *keys;
  KEY_TYPE *backup_keys;
  std::pair<KEY_TYPE, PAYLOAD_TYPE> *init_key_values;
  std::vector<std::pair<Operation, KEY_TYPE>> operations;
  std::mt19937 gen;

  struct Stat {
    std::vector<double> latency;
    uint64_t throughput = 0;
    size_t fitness_of_dataset = 0;
    long long memory_consumption = 0;
    uint64_t success_insert = 0;
    uint64_t success_read = 0;
    uint64_t success_update = 0;
    uint64_t success_remove = 0;
    uint64_t scan_not_enough = 0;

    void clear() {
      latency.clear();
      throughput = 0;
      fitness_of_dataset = 0;
      memory_consumption = 0;
      success_insert = 0;
      success_read = 0;
      success_update = 0;
      success_remove = 0;
      scan_not_enough = 0;
    }
  } stat;

  struct alignas(CACHELINE_SIZE) ThreadParam {
    std::vector<std::pair<uint64_t, uint64_t>> latency;
    uint64_t success_insert = 0;
    uint64_t success_read = 0;
    uint64_t success_update = 0;
    uint64_t success_remove = 0;
    uint64_t scan_not_enough = 0;
  };
  typedef ThreadParam param_t;

  // custom test cases.
  // controlled under test_suite flag
  // when test_suite is true
  // it will run two tests. first is insert benchmark, second is read benchmark
  /*
  | workload | dataset | bulkload | insert | lookup |  |
  | --- | --- | --- | --- | --- | --- |
  | 0 | shuffled | first sequence | random point | random point |  |
  | 1 | sorted | random sequence | random point | random point |  |
  | 2 | sorted | first sequence + last one | random point | random point |  |
  | 3 | sorted | random sequence + smallest(leftmost) & biggest(rightmost) one |
  random point | random point |  | | 4 | sorted |  |  |  |  | | 5 | sorted |
  generated pseudo-data via KernelDensity Estimation |  |  |  | | 6 | sorted |
  normal distribution sampling |  |  |  |
  */
public:
  size_t backup_operations_num;
  std::vector<std::pair<Operation, KEY_TYPE>> backup_operations;
  int64_t bulkload_duration;
  void run_custom_suite() {
    assert(test_suite);
    keys = load_keys_inner(keys_file_path);
    if (test_suite == 10) {
      INVARIANT(!backup_keys_file_path.empty());
      backup_keys = load_keys_inner(backup_keys_file_path);
    }
    generate_dataset_inner();
    for (auto s : all_index_type) {
      for (auto t : all_thread_num) {
        thread_num = stoi(t);
        index_type = s;
        index_t *index;
        // bulkload
        prepare(index, keys);
        // insert
        read_ratio = 0.0;
        insert_ratio = 1.0;
        run(index);
        // 清空一些元信息，转移operations，开始测read
        std::swap(operations, backup_operations);
        std::swap(operations_num, backup_operations_num);
        read_ratio = 1.0;
        insert_ratio = 0.0;
        run(index);
        // swap back, recover
        std::swap(operations, backup_operations);
        std::swap(operations_num, backup_operations_num);
        if (index != nullptr)
          delete index;
      }
    }
  }

  KEY_TYPE *load_keys_inner(std::string keys_file_path_inner) {
    // Read keys from file
    COUT_THIS("Reading data from file.");
    KEY_TYPE *keys_inner;
    if (table_size > 0) // FIXME: Duplicated allocation with load_binary_data or load_text_data can cause memory leak. Note that this is GRE's fault, not JSC's.
      keys_inner = new KEY_TYPE[table_size];

    if (keys_file_type == "binary") {
      table_size =
          load_binary_data(keys_inner, table_size, keys_file_path_inner);
      if (table_size <= 0) {
        COUT_THIS(
            "Could not open key file, please check the path of key file.");
        exit(0);
      }
    } else if (keys_file_type == "text") {
      table_size = load_text_data(keys_inner, table_size, keys_file_path_inner);
      if (table_size <= 0) {
        COUT_THIS(
            "Could not open key file, please check the path of key file.");
        exit(0);
      }
    } else {
      COUT_THIS("Could not open key file, please check the path of key file.");
      exit(0);
    }

    tbb::parallel_sort(keys_inner, keys_inner + table_size);
    auto last = std::unique(keys_inner, keys_inner + table_size);
    table_size = last - keys_inner;
    COUT_VAR(table_size);
    return keys_inner;
  }
  // step1
  // 数据已经在keys里了，有序且去过重了，这一步的目的是生成bulkload的数据
  // init_key_values, init_keys
  // step2
  // 这里要生成两拨数据，insert rest的ops是在operations里面
  // operations, operations_num
  // 剩下read的ops放到backup_operations里面
  // backup_operations, backup_operations_num
  void generate_dataset_inner() {
    switch (test_suite) {
    case 1: {
      generate_dataset_case1();
      break;
    };
    case 2: {
      generate_dataset_case2();
      break;
    };
    case 3: {
      generate_dataset_case3();
      break;
    };
    case 4: {
      __builtin_unreachable();
      // generate_dataset_case4();
      break;
    };
    case 5: {
      __builtin_unreachable();
      // generate_dataset_case5();
      break;
    };
    case 6: {
      generate_dataset_case6();
      break;
    };
    case 7: {
      generate_dataset_case7();
      break;
    };
    case 8: {
      generate_dataset_case8();
      break;
    };
    case 9: {
      generate_dataset_case9();
      break;
    };
    case 10: {
      generate_dataset_case10();
      break;
    }
    case 11: {
      generate_dataset_case11();
      break;
    }
    case 12: {
      generate_dataset_case12();
      break;
    }
    case 13: {
      generate_dataset_case13();
      break;
    }
    case 14: {
      generate_dataset_case14();
      break;
    }
    case 15: {
      generate_dataset_case15();
      break;
    }
    case 16: {
      generate_dataset_case16();
      break;
    }
    case 20: {
      generate_dataset_case20();
      break;
    }
    case 21: {
      generate_dataset_case21();
      break;
    }
    case 22: {
      generate_dataset_case22();
      break;
    }
    case 23: {
      generate_dataset_case23();
      break;
    }
    case 24: {
      generate_dataset_case24();
      break;
    }
    case 25: {
      generate_dataset_case25();
      break;
    }
    case 99: {
      generate_dataset_case99();
      break;
    };
    case 999: {
      generate_dataset_case999();
      break;
    };
    case 9999: {
      generate_dataset_case9999();
      break;
    };
    case 99999: {
      generate_dataset_case99999();
      break;
    };
    case 999999: {
      generate_dataset_case999999();
    };
    default:
      assert(false);
      break;
    }
  }

  void generate_dataset_case1() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 随机选取一个位置，然后取init_table_size个key
    size_t start_pos = gen() % (table_size - init_table_size + 1);
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < start_pos; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    for (size_t i = start_pos + init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case2() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size-1个key
    // 最后一个元素固定添加到init_keys里面
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size - 1; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    init_keys[init_table_size - 1] = keys[table_size - 1];
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size - 1; i < table_size - 1; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case3() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 随机选取一个位置，然后取init_table_size-2个key
    // 需要注意，选取的序列，不包含一头一尾，这两个数会单独添加
    size_t start_pos = gen() % (table_size - (init_table_size - 1)) +
                       1; //[1, table_size - init_table_size + 1]
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size - 2; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    init_keys[init_table_size - 2] = keys[0];
    init_keys[init_table_size - 1] = keys[table_size - 1];
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 1; i < start_pos; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    for (size_t i = start_pos + init_table_size - 2; i < table_size - 1; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // ramdom substring + sampling
  void generate_dataset_case4() {
    COUT_N_EXIT("deprecated");
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    double seq_ratio = 0.8;
    size_t seq_init_size = init_table_size * seq_ratio;
    double sampling_ratio = 1.0 - seq_ratio;
    size_t sampling_size = init_table_size * sampling_ratio;
    std::normal_distribution<long double> normal_dis(
        (table_size - seq_init_size) / 2, sampling_size * sigma_ratio);
    std::unordered_set<uint64_t> s;
    size_t start_pos = gen() % (table_size - size_t(seq_init_size));
    for (size_t i = start_pos; i < start_pos + seq_init_size; ++i) {
      s.insert(i + 1);
    }
    // [0,start_pos), [start_pos, start_pos+seq_init_size), [start_pos +
    // seq_init_size, table_size)
    while (s.size() < init_table_size) {
      uint64_t x = round(normal_dis(gen));
      if (x > 0 && x < table_size - seq_init_size) {
        if (x > start_pos) {
          x += seq_init_size;
        }
        s.insert(x);
      }
    }
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x - 1];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());
    COUT_VAR(s.size());
    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i + 1) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert
    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");
    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case5() { COUT_N_EXIT("deprecated"); }

  // normal sampling with random insert
  void generate_dataset_case6() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    std::normal_distribution<long double> normal_dis(
        table_size / 2, init_table_size * sigma_ratio);
    std::unordered_set<uint64_t> s;
    while (s.size() < init_table_size) {
      uint64_t x = round(normal_dis(gen));
      if (x > 0 && x <= table_size) {
        s.insert(x);
      }
    }
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x - 1];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());
    COUT_VAR(s.size());
    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i + 1) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert
    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");
    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // sorted append
  void generate_dataset_case7() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 随机选取一个位置，然后取init_table_size个key
    size_t start_pos = gen() % (table_size - init_table_size + 1);
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < start_pos; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    for (size_t i = start_pos + init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // sorted append
  void generate_dataset_case8() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size-1个key
    // 最后一个元素固定添加到init_keys里面
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size - 1; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    init_keys[init_table_size - 1] = keys[table_size - 1];
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size - 1; i < table_size - 1; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // normal sampling with sorted insert
  void generate_dataset_case9() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    std::normal_distribution<long double> normal_dis(
        table_size / 2, init_table_size * sigma_ratio);
    std::unordered_set<uint64_t> s;
    while (s.size() < init_table_size) {
      uint64_t x = round(normal_dis(gen));
      if (x > 0 && x <= table_size) {
        s.insert(x);
      }
    }
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x - 1];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());
    COUT_VAR(s.size());
    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i + 1) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted insert
    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");
    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // for two dataset. one is for bulkload another for insert
  // shifting workload
  void generate_dataset_case10() {
    INVARIANT(keys != nullptr);
    INVARIANT(backup_keys != nullptr);
    INVARIANT(init_table_ratio == 0.5);
    KEY_TYPE *tmp_keys = new KEY_TYPE[table_size];
    std::unordered_set<KEY_TYPE> s;
    std::shuffle(keys, keys + table_size, gen);
    std::shuffle(backup_keys, backup_keys + table_size, gen); // FIXME: need to assert the table size of two datasets are the same
                                                              // since they use the same variable "table_size" when loading
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = keys[i];
      s.insert(keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    memcpy(tmp_keys, keys, init_table_size * sizeof(KEY_TYPE));
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    int tmp_keys_pos = init_table_size;
    for (size_t i = 0; i < table_size; ++i) {
      if (!s.count(backup_keys[i])) {
        operations.push_back(
            std::pair<Operation, KEY_TYPE>(INSERT, backup_keys[i]));
        tmp_keys[tmp_keys_pos++] = backup_keys[i];
        s.insert(backup_keys[i]);     // FIXME: 应该不用再insert到set里面，backup keys已经在load的时候unique了
        if (operations.size() == operations_num) {
          break;
        }
      }
    }
    if (operations.size() != operations_num || tmp_keys_pos != table_size) {
      COUT_N_EXIT(
          "operations.size() != operations_num || tmp_keys_pos != table_size");
    }
    COUT_THIS("pass shifting check");
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    if (sample_distribution == "uniform") {
      sample_ptr =
          get_search_keys(&tmp_keys[0], table_size, backup_operations_num,
                          &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&tmp_keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] tmp_keys;
    delete[] sample_ptr;
  }

  // sampling at fixed interval
  void generate_dataset_case11() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    size_t gap_size =
        1 /
        init_table_ratio; // for every gap_size keys, we select the first one
    COUT_VAR(gap_size);
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < table_size; i += gap_size) {
      init_keys[start_pos++] = keys[i];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (i % gap_size != 0) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // sampling at fixed interval
  void generate_dataset_case12() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    size_t gap_size =
        1 /
        init_table_ratio; // for every gap_size keys, we select the first one
    COUT_VAR(gap_size);
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < table_size; i += gap_size) {
      init_keys[start_pos++] = keys[i];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (i % gap_size != 0) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted insert  FIXME: 这里可以不用再sort一遍

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // sampling at fixed interval
  // this is a tiered version
  void generate_dataset_case13() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    size_t gap_size =
        1 /
        init_table_ratio; // for every gap_size keys, we select the first one
    COUT_VAR(gap_size);
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < table_size; i += gap_size) {
      init_keys[start_pos++] = keys[i];
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 1; i < gap_size; i++) {
      for (size_t j = i; j < table_size; j += gap_size) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[j]));
      }
    }
    // no need sort here
    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case14() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case15() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted append

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  void generate_dataset_case16() {
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 随机选取一个位置，然后取init_table_size-2个key
    // 需要注意，选取的序列，不包含一头一尾，这两个数会单独添加
    size_t start_pos = gen() % (table_size - (init_table_size - 1)) +
                       1; //[1, table_size - init_table_size + 1]
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size - 2; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    init_keys[init_table_size - 2] = keys[0];
    init_keys[init_table_size - 1] = keys[table_size - 1];
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 1; i < start_pos; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    for (size_t i = start_pos + init_table_size - 2; i < table_size - 1; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted append

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // sorted / zipf sampling / random point
  void generate_dataset_case20() {
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // sorted / zipf sampling / sorted append
  void generate_dataset_case21() {
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted append

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // shuffled / zipf sampling / random point
  void generate_dataset_case22() {
    std::shuffle(keys, keys + table_size, gen);
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // shuffled / zipf sampling / sorted append
  void generate_dataset_case23() {
    std::shuffle(keys, keys + table_size, gen);
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
      }
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted append

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // middle crossover / zipf sampling / random point
  void generate_dataset_case24() {
    // 这里要变成middle crossover的结构
    KEY_TYPE *mc_keys = new KEY_TYPE[table_size];
    size_t mi_pos = (table_size - 1) / 2;
    for (size_t i = 0; i < table_size; i++) {
      mc_keys[i] = keys[mi_pos];
      if (i % 2 == 0) {
        mi_pos += (i + 1);
      } else {
        mi_pos -= (i + 1);
      }
    }
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = mc_keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, mc_keys[i]));
      }
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");
    delete [] mc_keys;

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }
  // middle crossover / zipf sampling / sorted append
  void generate_dataset_case25() {
    // 这里要变成middle crossover的结构
    KEY_TYPE *mc_keys = new KEY_TYPE[table_size];
    size_t mi_pos = (table_size - 1) / 2;
    for (size_t i = 0; i < table_size; i++) {
      mc_keys[i] = keys[mi_pos];
      if (i % 2 == 0) {
        mi_pos += (i + 1);
      } else {
        mi_pos -= (i + 1);
      }
    }
    // step 1 init_keys, init_key_values
    init_keys.resize(table_size); // 临时给大
    // 按照zipf采样，采一批数据到init_keys里面去，要搞个set标记一下哪些被采过了
    std::unordered_set<uint64_t> s; // 这里面存放的是key的pos
    // 按照zipf 采若干轮到s里,，zipf的key就是从0到table_size-1
    ZipfianGenerator zipf_gen(table_size, zipfian_constant, random_seed);
    COUT_VAR(zipf_gen.get_state().theta);
    for (int i = 0; i < sample_round; i++) {
      s.insert(zipf_gen.next());
    }
    init_table_size = s.size();
    init_table_ratio = double(init_table_size) / table_size;
    size_t i_pos = 0;
    for (auto x : s) {
      init_keys[i_pos++] = mc_keys[x];
    }
    init_keys.resize(init_table_size);
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      if (s.find(i) == s.end()) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, mc_keys[i]));
      }
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // random insert

    if (operations.size() != operations_num) {
      COUT_N_EXIT("operations.size() != operations_num")
    }
    COUT_THIS("pass sampling check");
    delete [] mc_keys;


    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // random bulkload, random insert, random read
  void generate_dataset_case99() {
    std::shuffle(keys, keys + table_size, gen);
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // random bulkload, sorted insert, random read
  void generate_dataset_case999() {
    std::shuffle(keys, keys + table_size, gen);
    // step 1 init_keys, init_key_values
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    tbb::parallel_sort(operations.begin(), operations.end()); // sorted insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // the best performance ro test
  // bulkload all dataset
  void generate_dataset_case9999() {
    // step 1 init_keys, init_key_values
    INVARIANT(init_table_ratio == 1.0);
    init_table_size = init_table_ratio * table_size;
    init_keys.resize(init_table_size);
    // 从最左端开始，然后取init_table_size个key
    size_t start_pos = 0;
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = start_pos; i < start_pos + init_table_size; ++i) {
      init_keys[i - start_pos] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());
    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size - init_table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = init_table_size; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    // step 3 backup_operations, backup_operations_num
    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // bulk 0, random append all, random point
  void generate_dataset_case99999() {
    // step 1 init_keys, init_key_values
    INVARIANT(init_table_ratio == 0);
    init_table_size = 0;
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }
    std::shuffle(operations.begin(), operations.end(), gen); // random insert

    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

  // bulk 0, sorted append all, random point
  void generate_dataset_case999999() {
    // step 1 init_keys, init_key_values
    INVARIANT(init_table_ratio == 0);
    init_table_size = 0;
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    // step 2 operations, operations_num
    operations_num = table_size; // insert rest of all
    operations.reserve(operations_num);
    for (size_t i = 0; i < table_size; ++i) {
      operations.push_back(std::pair<Operation, KEY_TYPE>(INSERT, keys[i]));
    }

    backup_operations_num = table_size;
    backup_operations.reserve(backup_operations_num);
    KEY_TYPE *sample_ptr = nullptr;
    std::shuffle(keys, keys + table_size, gen);
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&keys[0], table_size, backup_operations_num,
                                   &random_seed); // random read
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&keys[0], table_size,
                                        backup_operations_num, &random_seed);
    }
    for (size_t i = 0; i < backup_operations_num; ++i) {
      backup_operations.push_back(
          std::pair<Operation, KEY_TYPE>(READ, sample_ptr[i]));
    }
    delete[] sample_ptr;
  }

public:
  Benchmark() {}

  KEY_TYPE *load_keys() {
    // Read keys from file
    COUT_THIS("Reading data from file.");

    if (table_size > 0)
      keys = new KEY_TYPE[table_size];

    if (keys_file_type == "binary") {
      table_size = load_binary_data(keys, table_size, keys_file_path);
      if (table_size <= 0) {
        COUT_THIS(
            "Could not open key file, please check the path of key file.");
        exit(0);
      }
    } else if (keys_file_type == "text") {
      table_size = load_text_data(keys, table_size, keys_file_path);
      if (table_size <= 0) {
        COUT_THIS(
            "Could not open key file, please check the path of key file.");
        exit(0);
      }
    } else {
      COUT_THIS("Could not open key file, please check the path of key file.");
      exit(0);
    }

    if (!data_shift) {
      tbb::parallel_sort(keys, keys + table_size);
      auto last = std::unique(keys, keys + table_size);
      table_size = last - keys;
      std::shuffle(keys, keys + table_size, gen);
    }

    init_table_size = init_table_ratio * table_size;
    std::cout << "Table size is " << table_size << ", Init table size is "
              << init_table_size << std::endl;

    for (auto j = 0; j < 10; j++) {
      std::cout << keys[j] << " ";
    }
    std::cout << std::endl;

    // prepare data
    COUT_THIS("prepare init keys.");
    init_keys.resize(init_table_size);
#pragma omp parallel for num_threads(thread_num)
    for (size_t i = 0; i < init_table_size; ++i) {
      init_keys[i] = (keys[i]);
    }
    tbb::parallel_sort(init_keys.begin(), init_keys.end());

    init_key_values = new std::pair<KEY_TYPE, PAYLOAD_TYPE>[init_keys.size()];
#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < init_keys.size(); i++) {
      init_key_values[i].first = init_keys[i];
      init_key_values[i].second = 123456789;
    }
    COUT_VAR(table_size);
    COUT_VAR(init_keys.size());

    return keys;
  }

  inline void prepare(index_t *&index, const KEY_TYPE *keys) {
    index = get_index<KEY_TYPE, PAYLOAD_TYPE>(index_type);

    // initilize Index (sort keys first)
    Param param = Param(thread_num, 0);
    index->init(&param);

    if (dump_bulkload) {
      // dump the bulkload, with name as
      // {key}_{test_suite}_{init_table_size}_bulk.dump
      std::string dataset_name =
          keys_file_path.substr(keys_file_path.find_last_of("/") + 1);
      std::string dump_file_name =
          dataset_name + "_" + std::to_string(test_suite) + "_" +
          std::to_string(init_table_size) + "_bulk.dump";
      COUT_THIS("dump bulkload to " << dump_file_name);
      if (std::filesystem::exists(dump_file_name)) {
        COUT_THIS("dump file exists, skip dump");
        goto bulkload;
      }
      std::ofstream file(dump_file_name, std::ios::binary | std::ios::out);
      file.write(reinterpret_cast<const char *>(init_keys.data()),
                 std::streamsize(init_keys.size() * sizeof(KEY_TYPE)));
      file.close();
      return;
    }

  bulkload:
    // deal with the background thread case
    thread_num = param.worker_num;

    COUT_THIS("bulk loading");
    TSCNS ts;
    ts.init();
    auto start_time = ts.rdtsc();
    index->bulk_load(init_key_values, init_keys.size(), &param);
    auto end_time = ts.rdtsc();
    bulkload_duration = ts.tsc2ns(end_time) - ts.tsc2ns(start_time);
  }

  /*
   * keys_file_path:      the path where keys file at
   * keys_file_type:      binary or text
   * read_ratio:          the ratio of read operation
   * insert_ratio         the ratio of insert operation
   * delete_ratio         the ratio of delete operation
   * update_ratio         the ratio of update operation
   * scan_ratio           the ratio of scan operation
   * scan_num             the number of keys that every scan operation need to
   * scan operations_num      the number of operations(read, insert, delete,
   * update, scan) table_size           the total number of keys in key file
   * init_table_size      the number of keys that will be used in bulk loading
   * thread_num           the number of worker thread
   * index_type           the type of index(xindex, hot, alex...). Detail could
   * be refered to src/competitor sample_distribution  the distribution of
   * latency_sample_ratio the ratio of latency sampling
   * error_bound          the error bound of PGM metric
   * output_path          the path to store result
   */
  inline void parse_args(int argc, char **argv) {
    auto flags = parse_flags(argc, argv);
    keys_file_path = get_required(flags, "keys_file"); // required
    backup_keys_file_path =
        get_with_default(flags, "backup_keys_file", ""); // required
    keys_file_type = get_with_default(flags, "keys_file_type", "binary");
    read_ratio = stod(get_required(flags, "read"));              // required
    insert_ratio = stod(get_with_default(flags, "insert", "0")); // required
    delete_ratio = stod(get_with_default(flags, "delete", "0"));
    update_ratio = stod(get_with_default(flags, "update", "0"));
    scan_ratio = stod(get_with_default(flags, "scan", "0"));
    scan_num = stoi(get_with_default(flags, "scan_num", "100"));
    operations_num = stoi(
        get_with_default(flags, "operations_num", "1000000000")); // required
    table_size = stoi(get_with_default(flags, "table_size", "-1"));
    init_table_ratio = stod(get_with_default(flags, "init_table_ratio", "0.5"));
    del_table_ratio = stod(get_with_default(flags, "del_table_ratio", "0.5"));
    init_table_size = 0;
    sample_round = stoi(get_with_default(flags, "sample_round", "50000000"));
    all_thread_num = get_comma_separated(flags, "thread_num"); // required
    all_index_type = get_comma_separated(flags, "index");      // required
    sample_distribution =
        get_with_default(flags, "sample_distribution", "uniform");
    latency_sample = get_boolean_flag(flags, "latency_sample");
    latency_sample_ratio =
        stod(get_with_default(flags, "latency_sample_ratio", "0.01"));
    error_bound = stoi(get_with_default(flags, "error_bound", "64"));
    output_path = get_with_default(flags, "output_path", "./out.csv");
    random_seed = stoul(get_with_default(flags, "seed", "1866"));
    gen.seed(random_seed);
    memory_record = get_boolean_flag(flags, "memory");
    dataset_statistic = get_boolean_flag(flags, "dataset_statistic");
    data_shift = get_boolean_flag(flags, "data_shift");
    test_suite = stoi(get_with_default(flags, "test_suite", "0"));
    dump_bulkload = get_boolean_flag(flags, "dump_bulkload");
    sigma_ratio = stod(get_with_default(flags, "sigma_ratio", "0.5"));
    zipfian_constant = stod(get_with_default(flags, "zipfian_constant", "0.99"));
    COUT_THIS("[micro] Read:Insert:Update:Scan:Delete= "
              << read_ratio << ":" << insert_ratio << ":" << update_ratio << ":"
              << scan_ratio << ":" << delete_ratio);
    double ratio_sum =
        read_ratio + insert_ratio + delete_ratio + update_ratio + scan_ratio;
    double insert_delete = insert_ratio + delete_ratio;
    INVARIANT(insert_delete == insert_ratio || insert_delete == delete_ratio);
    if (!test_suite) {
      INVARIANT(ratio_sum > 0.9999 &&
                ratio_sum < 1.0001); // avoid precision lost
    }
    INVARIANT(sample_distribution == "zipf" ||
              sample_distribution == "uniform");
    INVARIANT(all_thread_num.size() > 0);
  }

  void generate_operations(KEY_TYPE *keys) {
    // prepare operations
    operations.reserve(operations_num);
    COUT_THIS("sample keys.");
    KEY_TYPE *sample_ptr = nullptr;
    if (sample_distribution == "uniform") {
      sample_ptr = get_search_keys(&init_keys[0], init_table_size,
                                   operations_num, &random_seed);
    } else if (sample_distribution == "zipf") {
      sample_ptr = get_search_keys_zipf(&init_keys[0], init_table_size,
                                        operations_num, &random_seed);
    }

    // generate operations(read, insert, update, scan)
    COUT_THIS("generate operations.");
    std::uniform_real_distribution<> ratio_dis(0, 1);
    size_t sample_counter = 0, insert_counter = init_table_size;
    size_t delete_counter = table_size * (1 - del_table_ratio);

    if (data_shift) {
      size_t rest_key_num = table_size - init_table_size;
      if (rest_key_num > 0) {
        std::sort(keys + init_table_size, keys + table_size);
        std::random_shuffle(keys + init_table_size, keys + table_size);
      }
    }

    size_t temp_counter = 0;
    for (size_t i = 0; i < operations_num; ++i) {
      auto prob = ratio_dis(gen);
      if (prob < read_ratio) {
        // if (temp_counter >= table_size) {
        //     operations_num = i;
        //     break;
        // }
        // operations.push_back(std::pair<Operation, KEY_TYPE>(READ,
        // keys[temp_counter++]));
        operations.push_back(
            std::pair<Operation, KEY_TYPE>(READ, sample_ptr[sample_counter++]));
      } else if (prob < read_ratio + insert_ratio) {
        if (insert_counter >= table_size) {
          operations_num = i;
          break;
        }
        operations.push_back(
            std::pair<Operation, KEY_TYPE>(INSERT, keys[insert_counter++]));
      } else if (prob < read_ratio + insert_ratio + update_ratio) {
        operations.push_back(std::pair<Operation, KEY_TYPE>(
            UPDATE, sample_ptr[sample_counter++]));
      } else if (prob < read_ratio + insert_ratio + update_ratio + scan_ratio) {
        operations.push_back(
            std::pair<Operation, KEY_TYPE>(SCAN, sample_ptr[sample_counter++]));
      } else {
        if (delete_counter >= table_size) {
          operations_num = i;
          break;
        }
        operations.push_back(
            std::pair<Operation, KEY_TYPE>(DELETE, keys[delete_counter++]));
        // operations.push_back(std::pair<Operation, KEY_TYPE>(DELETE,
        // sample_ptr[sample_counter++]));
      }
    }

    COUT_VAR(operations.size());

    delete[] sample_ptr;
  }

  void run(index_t *index) {
    std::thread *thread_array = new std::thread[thread_num];
    param_t params[thread_num];
    TSCNS tn;
    tn.init();
    printf("Begin running\n");
    auto start_time = tn.rdtsc();
    auto end_time = tn.rdtsc();
    //    System::profile("perf.data", [&]() {
#pragma omp parallel num_threads(thread_num)
    {
      // thread specifier
      auto thread_id = omp_get_thread_num();
      auto paramI = Param(thread_num, thread_id);
      // Latency Sample Variable
      int latency_sample_interval =
          operations_num / (operations_num * latency_sample_ratio);
      auto latency_sample_start_time = tn.rdtsc();
      auto latency_sample_end_time = tn.rdtsc();
      param_t &thread_param = params[thread_id];
      thread_param.latency.reserve(operations_num / latency_sample_interval);
      // Operation Parameter
      PAYLOAD_TYPE val;
      std::pair<KEY_TYPE, PAYLOAD_TYPE> *scan_result =
          new std::pair<KEY_TYPE, PAYLOAD_TYPE>[scan_num];
      // waiting all thread ready
#pragma omp barrier
#pragma omp master
      start_time = tn.rdtsc();
// running benchmark
#pragma omp for schedule(dynamic, 10000)
      for (auto i = 0; i < operations_num; i++) {
        auto op = operations[i].first;
        auto key = operations[i].second;

        if (latency_sample && i % latency_sample_interval == 0)
          latency_sample_start_time = tn.rdtsc();

        if (op == READ) { // get
          auto ret = index->get(key, val, &paramI);
          // if(!ret) {
          //     printf("read not found, Key %lu\n",key);
          //     continue;
          // }
          // if(val != 123456789) {
          //     printf("read failed, Key %lu, val %llu\n",key, val);
          //     exit(1);
          // }
          thread_param.success_read += ret;
        } else if (op == INSERT) { // insert
          auto ret = index->put(key, 123456789, &paramI);
          thread_param.success_insert += ret;
        } else if (op == UPDATE) { // update
          auto ret = index->update(key, 234567891, &paramI);
          thread_param.success_update += ret;
        } else if (op == SCAN) { // scan
          auto scan_len = index->scan(key, scan_num, scan_result, &paramI);
          if (scan_len != scan_num) {
            thread_param.scan_not_enough++;
          }
        } else if (op == DELETE) { // delete
          auto ret = index->remove(key, &paramI);
          thread_param.success_remove += ret;
        }

        if (latency_sample && i % latency_sample_interval == 0) {
          latency_sample_end_time = tn.rdtsc();
          thread_param.latency.push_back(std::make_pair(
              latency_sample_start_time, latency_sample_end_time));
        }
      } // omp for loop
#pragma omp master
      end_time = tn.rdtsc();
    } // all thread join here

    //    });
    auto diff = tn.tsc2ns(end_time) - tn.tsc2ns(start_time);
    printf("Finish running\n");

    // gather thread local variable
    for (auto &p : params) {
      if (latency_sample) {
        for (auto e : p.latency) {
          auto temp =
              (tn.tsc2ns(e.first) - tn.tsc2ns(e.second)) / (double)1000000000;
          stat.latency.push_back(tn.tsc2ns(e.second) - tn.tsc2ns(e.first));
        }
      }
      stat.success_read += p.success_read;
      stat.success_insert += p.success_insert;
      stat.success_update += p.success_update;
      stat.success_remove += p.success_remove;
      stat.scan_not_enough += p.scan_not_enough;
    }
    // calculate throughput
    stat.throughput =
        static_cast<uint64_t>(operations_num / (diff / (double)1000000000));

    // calculate dataset metric
    if (dataset_statistic) {
      std::sort(keys, keys + table_size);
      stat.fitness_of_dataset =
          pgmMetric::PGM_metric(keys, table_size, error_bound);
    }

    // record memory consumption
    if (memory_record)
      stat.memory_consumption = index->memory_consumption();

    print_stat();

    delete[] thread_array;
  }

  void print_stat(bool header = false, bool clear_flag = true) {
    if (test_suite == 9999 && insert_ratio == 1.0 && stat.throughput == 0) {
      // lookup baseline. calc the bulkload throughput
      stat.throughput = table_size * 1000000000 / bulkload_duration;
    }

    double avg_latency = 0;
    // average latency
    for (auto t : stat.latency) {
      avg_latency += t;
    }
    avg_latency /= stat.latency.size();

    // latency variance
    double latency_variance = 0;
    if (latency_sample) {
      for (auto t : stat.latency) {
        latency_variance += (t - avg_latency) * (t - avg_latency);
      }
      latency_variance /= stat.latency.size();
      std::sort(stat.latency.begin(), stat.latency.end());
    }

    printf("Throughput = %lu\n", stat.throughput);
    printf("Memory: %lld\n", stat.memory_consumption);
    printf("success_read: %lu\n", stat.success_read);
    printf("success_insert: %lu\n", stat.success_insert);
    printf("success_update: %lu\n", stat.success_update);
    printf("success_remove: %lu\n", stat.success_remove);
    printf("scan_not_enough: %lu\n", stat.scan_not_enough);

    // time id
    std::time_t t = std::time(nullptr);
    char time_str[100];

    if (!file_exists(output_path)) {
      std::ofstream ofile;
      ofile.open(output_path, std::ios::app);
      ofile << "id"
            << ",";
      ofile << "read_ratio"
            << ","
            << "insert_ratio"
            << ","
            << "update_ratio"
            << ","
            << "scan_ratio"
            << ","
            << "delete_ratio"
            << ",";
      ofile << "key_path"
            << ",";
      ofile << "backup_key_path"
            << ",";
      ofile << "index_type"
            << ",";
      ofile << "throughput"
            << ",";
      ofile << "init_table_size"
            << ",";
      ofile << "memory_consumption"
            << ",";
      ofile << "thread_num"
            << ",";
      ofile << "min"
            << ",";
      ofile << "50 percentile"
            << ",";
      ofile << "90 percentile"
            << ",";
      ofile << "99 percentile"
            << ",";
      ofile << "99.9 percentile"
            << ",";
      ofile << "99.99 percentile"
            << ",";
      ofile << "max"
            << ",";
      ofile << "avg"
            << ",";
      ofile << "seed"
            << ",";
      ofile << "scan_num"
            << ",";
      ofile << "latency_variance"
            << ",";
      ofile << "latency_sample"
            << ",";
      ofile << "data_shift"
            << ",";
      ofile << "pgm"
            << ",";
      ofile << "error_bound"
               ",";
      ofile << "table_size"
               ",";
      ofile << "test_suite" << std::endl;
    }

    std::ofstream ofile;
    ofile.open(output_path, std::ios::app);
    if (std::strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S",
                      std::localtime(&t))) {
      ofile << time_str << ',';
    }
    ofile << read_ratio << "," << insert_ratio << "," << update_ratio << ","
          << scan_ratio << "," << delete_ratio << ",";

    ofile << keys_file_path << ",";
    ofile << backup_keys_file_path << ",";
    ofile << index_type << ",";
    ofile << stat.throughput << ",";
    ofile << init_table_size << ",";
    ofile << stat.memory_consumption << ",";
    ofile << thread_num << ",";
    if (latency_sample) {
      ofile << stat.latency[0] << ",";
      ofile << stat.latency[0.5 * stat.latency.size()] << ",";
      ofile << stat.latency[0.9 * stat.latency.size()] << ",";
      ofile << stat.latency[0.99 * stat.latency.size()] << ",";
      ofile << stat.latency[0.999 * stat.latency.size()] << ",";
      ofile << stat.latency[0.9999 * stat.latency.size()] << ",";
      ofile << stat.latency[stat.latency.size() - 1] << ",";
      ofile << avg_latency << ",";
    } else {
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
      ofile << 0 << ",";
    }
    ofile << random_seed << ",";
    ofile << scan_num << ",";
    ofile << latency_variance << ",";
    ofile << latency_sample << ",";
    ofile << data_shift << ",";
    ofile << stat.fitness_of_dataset << ",";
    ofile << error_bound << ",";
    ofile << table_size << ",";
    ofile << test_suite << std::endl;
    ofile.close();

    if (clear_flag)
      stat.clear();
  }

  void run_benchmark() {
    if (test_suite) {
      run_custom_suite();
      return;
    }
    load_keys();
    generate_operations(keys);
    for (auto s : all_index_type) {
      for (auto t : all_thread_num) {
        thread_num = stoi(t);
        index_type = s;
        index_t *index;
        prepare(index, keys);
        run(index);
        if (index != nullptr)
          delete index;
      }
    }
  }
};
