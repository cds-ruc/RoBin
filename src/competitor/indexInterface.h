#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

struct Param { // for xindex
  size_t worker_num;
  uint32_t thread_id;
  const void *keys;
  std::string dataset_name;
  int test_suite;
  double init_table_ratio;
  std::string index_name;
  Param(size_t worker_num, uint32_t thread_id)
      : worker_num(worker_num), thread_id(thread_id) {}
  Param(size_t worker_num, uint32_t thread_id, const void *keys)
      : worker_num(worker_num), thread_id(thread_id), keys(keys) {}
  Param(size_t worker_num, uint32_t thread_id, const void *keys,
        std::string dataset_name, std::string index_name)
      : worker_num(worker_num), thread_id(thread_id), keys(keys),
        dataset_name(dataset_name), index_name(index_name) {}
};

struct BaseCompare {
  template <class T1, class T2>
  bool operator()(const T1 &x, const T2 &y) const {
    static_assert(std::is_arithmetic<T1>::value &&
                      std::is_arithmetic<T2>::value,
                  "Comparison types must be numeric.");
    return x < y;
  }
};

template <class KEY_TYPE, class PAYLOAD_TYPE, class KeyComparator = BaseCompare>
class indexInterface {
public:
  virtual ~indexInterface() = default;
  virtual void bulk_load(std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value,
                         size_t num, Param *param = nullptr) = 0;

  virtual bool get(KEY_TYPE key, PAYLOAD_TYPE &val, Param *param = nullptr) = 0;

  virtual bool put(KEY_TYPE key, PAYLOAD_TYPE value,
                   Param *param = nullptr) = 0;

  virtual bool update(KEY_TYPE key, PAYLOAD_TYPE value,
                      Param *param = nullptr) = 0;

  virtual bool remove(KEY_TYPE key, Param *param = nullptr) = 0;

  virtual size_t scan(KEY_TYPE key_low_bound, size_t key_num,
                      std::pair<KEY_TYPE, PAYLOAD_TYPE> *result,
                      Param *param = nullptr) = 0;

  virtual void init(Param *param = nullptr) = 0;

  virtual long long memory_consumption() = 0; // bytes
#ifdef PROFILING
  virtual void print_stats(std::string s) = 0;
#endif
};