#include "../benchmark/binary_search.h"
#include "./alex/alex.h"
#include "./alexolc/alexolc.h"
#include "./art/art.h"
#include "./artolc/artolc.h"
#include "./btree/btree.h"
#include "./btreeolc/btreeolc.h"
#include "./dili/dili.h"
#include "./dytis/dytis.h"
#include "./findex/finedex.h"
#include "./indexInterface.h"
#include "./lipp/lipp.h"
#include "./pgm/pgm.h"
#include "./sali/sali.h"
#include "./xindex/xindex.h"
#include "./lippolc/lippolc.h"
#include "./masstree/masstree.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fstream>
#include "tbb/parallel_sort.h"

#define ROBIN_BOUND(a, x, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

template <class KEY_TYPE, class PAYLOAD_TYPE>
class partitionedIndexInterface final
    : public indexInterface<KEY_TYPE, PAYLOAD_TYPE> {
public:
  void prepare(std::string index_type, std::string partition_method,
               size_t &partition_num) {
    if (partition_method == "range") {
      use_model_ = false;
    } else if (partition_method == "model") {
      // TODO: load model file
      // partition_model/dataset/test_suite/init_table_ratio
      use_model_ = true;
    } else {
      std::cout << "Could not find a matching partition method called "
                << partition_method << ".\n";
      exit(0);
    }
    partition_num_ = partition_num;
    partition_key_ = new KEY_TYPE[partition_num - 1];
    index_ = new indexInterface<KEY_TYPE, PAYLOAD_TYPE> *[partition_num];
    if (index_type == "alex") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new alexInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else if (index_type == "btree") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new BTreeInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else if (index_type == "art") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new ARTInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else if (index_type == "lipp") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new LIPPInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else if (index_type == "dili") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new diliInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else if (index_type == "dytis") {
      for (size_t i = 0; i < partition_num; i++) {
        index_[i] = new dytisInterface<KEY_TYPE, PAYLOAD_TYPE>;
      }
    } else {
      std::cout << "Could not find a matching index called " << index_type
                << ".\n";
      exit(0);
    }
  }
  void bulk_load(std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value, size_t num,
                 Param *param = nullptr) {
    if (!use_model_) {
      // 均匀将数据分配到每个分区
      size_t start_idx = 0;
      for (size_t i = 0; i < partition_num_; i++) {
        if (i == partition_num_ - 1) {
          // all_keys to end
          index_[i]->bulk_load(key_value + start_idx, num - start_idx, param);
          // std::cout << "success bulk_load partition " << i
          //           << "start from: " << start_idx << " to: " << num
          //           << std::endl;
        } else {
          // all key_value < partition_key_[i]
          size_t end_idx =
              heuristic_upper_bound(key_value, num, {partition_key_[i], 0});
          index_[i]->bulk_load(key_value + start_idx, end_idx - start_idx,
                               param);
          // std::cout << "success bulk_load partition " << i
          //           << "start from: " << start_idx << " to: " << end_idx
          //           << std::endl;
          start_idx = end_idx;
        }
      }
    } else {
      for (size_t i = 0; i < num; i++) {
          size_t predicted_idx =
              ROBIN_BOUND(0, key_value[i].first * model_slope_ + model_intercept_,
                          partition_num_ - 1);
          // move forwards check the same part
          size_t j = i + 1;
          while (j < num &&
                (ROBIN_BOUND(0,
                            size_t(key_value[j].first * model_slope_ + model_intercept_),
                            partition_num_ - 1)) == predicted_idx) {
            j++;
          }
          // [i,j) is the same partition
          index_[predicted_idx]->bulk_load(key_value + i, j - i, param);
          // std::cout<<"success bulk_load partition "<<predicted_idx<<" start from: "<<i<<" to: "<<j<<std::endl;
          i = j - 1;
      }
    }
  }

  bool get(KEY_TYPE key, PAYLOAD_TYPE &val, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->get(key, val, param);
    } else {
      size_t predicted_idx = ROBIN_BOUND(
          0, key * model_slope_ + model_intercept_, partition_num_ - 1);
      return index_[predicted_idx]->get(key, val, param);
    }
    return false;
  }

  bool put(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->put(key, value, param);
    } else {
      size_t predicted_idx = ROBIN_BOUND(
          0, key * model_slope_ + model_intercept_, partition_num_ - 1);
      return index_[predicted_idx]->put(key, value, param);
    }
    return false;
  }

  bool update(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->update(key, value, param);
    } else {
      size_t predicted_idx = ROBIN_BOUND(
          0, key * model_slope_ + model_intercept_, partition_num_ - 1);
      return index_[predicted_idx]->update(key, value, param);
    }
    return false;
  }

  bool remove(KEY_TYPE key, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->remove(key, param);
    } else {
      size_t predicted_idx = ROBIN_BOUND(
          0, key * model_slope_ + model_intercept_, partition_num_ - 1);
      return index_[predicted_idx]->remove(key, param);
    }
    return false;
  }

  size_t scan(KEY_TYPE key_low_bound, size_t key_num,
              std::pair<KEY_TYPE, PAYLOAD_TYPE> *result,
              Param *param = nullptr) {
    return 0;
  }

  void init(Param *param = nullptr) {
    // call before bulk_load
    if (!use_model_) {
      assert(param->keys != nullptr);
      KEY_TYPE *copied_keys = new KEY_TYPE[200000000];
#pragma omp parallel for
      for (size_t i = 0; i < 200000000; i++) {
        copied_keys[i] = ((KEY_TYPE *)param->keys)[i];
      }
      tbb::parallel_sort(copied_keys, copied_keys + 200000000);
      int sub_index_size = 200000000 / partition_num_;
      // cout<<"partition_key_: "<<endl;
      for (size_t i = 0; i < partition_num_ - 1; i++) {
        partition_key_[i] = copied_keys[(i + 1) * sub_index_size];
        // cout<<partition_key_[i]<<", ";
      }
      // cout<<endl;
      for (size_t i = 0; i < partition_num_; i++) {
        index_[i]->init(param);
      }
      delete[] copied_keys;
    } else {
      // TODO: load model file
      std::string model_file;
      if (param->index_name == "alex") {
        std::string model_file = "result/partition_model/" +
                                 param->dataset_name + "/" +
                                 std::to_string(10) + "/" + std::to_string(1) +
                                 "/" + param->index_name + "_insert_root.log";
        std::cout<<"model_file: "<<model_file<<endl;
        // read this file, such as
        // num_inserts,model_slope,model_intercept,num_slots
        // 0,1.60894e-13,-216370,16384
        std::ifstream in(model_file);
        if (!in.is_open()) {
          std::cout << "Could not open model file " << model_file << ".\n";
          exit(0);
        }
        // skip the first line
        std::string line;
        std::getline(in, line);
        // read the second line
        std::getline(in, line);
        std::stringstream ss(line);
        // get the 2nd and 3rd value, , split by ','
        std::string temp;
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        model_slope_ = std::stod(temp);
        std::getline(ss, temp, ',');
        model_intercept_ = std::stod(temp);
        in.close();
      } else if (param->index_name == "lipp") {
        std::string model_file = "result/partition_model/" +
                                 param->dataset_name + "/" +
                                 std::to_string(22) + "/" + std::to_string(0) +
                                 "/" + param->index_name + "_insert_root.log";
        // read the same file, but the last line
        std::ifstream in(model_file);
        if (!in.is_open()) {
          std::cout << "Could not open model file " << model_file << ".\n";
          exit(0);
        }
        // read the last line
        std::string line;
        while (std::getline(in, line)) {
          ;
        }
        std::stringstream ss(line);
        // get the 2nd and 3rd value, , split by ','
        std::string temp;
        std::getline(ss, temp, ',');
        std::getline(ss, temp, ',');
        model_slope_ = std::stod(temp);
        std::getline(ss, temp, ',');
        model_intercept_ = std::stod(temp);
        in.close();
      }
      for (size_t i = 0; i < partition_num_; i++) {
        index_[i]->init(param);
      }
      std::cout << "model_slope_: " << model_slope_
                << ", model_intercept_: " << model_intercept_ << endl;
    }
  }

  long long memory_consumption() { return 0; }

#ifdef PROFILING
  void print_stats(std::string s) {}
#endif

private:
  size_t partition_num_;
  bool use_model_;
  KEY_TYPE *partition_key_;
  indexInterface<KEY_TYPE, PAYLOAD_TYPE> **index_;
  double model_slope_ = 0.0;
  double model_intercept_ = 0.0;
};