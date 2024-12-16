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
#include <oneapi/tbb/parallel_sort.h>

template <class KEY_TYPE, class PAYLOAD_TYPE>
class partitionedIndexInterface final
    : public indexInterface<KEY_TYPE, PAYLOAD_TYPE> {
public:
  void prepare(std::string index_type, std::string partition_method,
               size_t &partition_num) {
    if (partition_method == "range") {
      partition_num_ = partition_num;
      use_model_ = false;
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
      } else {
        std::cout << "Could not find a matching index called " << index_type
                  << ".\n";
        exit(0);
      }
    } else if (partition_method == "model") {
      // TODO:
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
    }
  }

  bool get(KEY_TYPE key, PAYLOAD_TYPE &val, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->get(key, val, param);
    }
    return false;
  }

  bool put(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->put(key, value, param);
    }
  }

  bool update(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->update(key, value, param);
    }
  }

  bool remove(KEY_TYPE key, Param *param = nullptr) {
    if (!use_model_) {
      size_t idx =
          heuristic_upper_bound(partition_key_, partition_num_ - 1, key);
      return index_[idx]->remove(key, param);
    }
  }

  size_t scan(KEY_TYPE key_low_bound, size_t key_num,
              std::pair<KEY_TYPE, PAYLOAD_TYPE> *result,
              Param *param = nullptr) {
    return 0;
  }

  void init(Param *param = nullptr) {
    // call before bulk_load
    if (!use_model_) {
      KEY_TYPE *copied_keys = new KEY_TYPE[200000000];
      #pragma omp parallel for
      for (size_t i = 0; i < 200000000; i++) {
        copied_keys[i] = ((KEY_TYPE *)param->keys)[i];
      }
      tbb::parallel_sort(copied_keys, copied_keys + 200000000);
      assert(param->keys != nullptr);
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
      delete []copied_keys;
    } else {
      // TODO: load model file
    }
  }

  long long memory_consumption() { return 0; }

#ifdef PROFILING
  void print_stats(std::string s);
#endif

private:
  size_t partition_num_;
  bool use_model_;
  KEY_TYPE *partition_key_;
  indexInterface<KEY_TYPE, PAYLOAD_TYPE> **index_;
};