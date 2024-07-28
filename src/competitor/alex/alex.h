#include "../indexInterface.h"
#include "./src/src/core/alex.h"

template <class KEY_TYPE, class PAYLOAD_TYPE>
class alexInterface final : public indexInterface<KEY_TYPE, PAYLOAD_TYPE> {
public:
  void init(Param *param = nullptr) {}

  void bulk_load(std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value, size_t num,
                 Param *param = nullptr);

  bool get(KEY_TYPE key, PAYLOAD_TYPE &val, Param *param = nullptr);

  bool put(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr);

  bool update(KEY_TYPE key, PAYLOAD_TYPE value, Param *param = nullptr);

  bool remove(KEY_TYPE key, Param *param = nullptr);

  size_t scan(KEY_TYPE key_low_bound, size_t key_num,
              std::pair<KEY_TYPE, PAYLOAD_TYPE> *result,
              Param *param = nullptr);

  long long memory_consumption() {
    return index.model_size() + index.data_size();
  }

  void print_stats(std::string s) {
    if (s == "bulkload") {
      index.print_depth_stats(s);
      index.print_level_model_stats(s);
      index.print_hist_model_stats(s);
      index.print_smo_stats(s);
    }
    if (s == "insert") {
      index.print_depth_stats(s);
      index.print_level_model_stats(s);
      index.print_hist_model_stats(s);
      index.print_smo_stats(s);
    }
    if (s == "read") {
      print_key_cmp_distribution(s);
      print_key_cmp_stats(s);
    }
    return ;
  }

private:
  alex::Alex<KEY_TYPE, PAYLOAD_TYPE, alex::AlexCompare,
             std::allocator<std::pair<KEY_TYPE, PAYLOAD_TYPE>>, false>
      index;
  std::vector<long long> key_cmp_distribution;
  void print_key_cmp_distribution(std::string s) {
    std::ofstream out("alex_" + s + "_key_cmp_distribution.log");
    if (!out.is_open()) {
      std::cerr << "Failed to open file." << std::endl;
      return;
    }
    out << "cmp,count" << std::endl;
    for (size_t i = 0; i < key_cmp_distribution.size(); i++) {
      out << i << "," << key_cmp_distribution[i] << std::endl;
    }
  }
  std::vector<std::pair<KEY_TYPE, long long>> key_cmp_stats;
  void print_key_cmp_stats(std::string s) {
    std::ofstream out("alex_" + s + "_key_cmp_stats.log");
    if (!out.is_open()) {
      std::cerr << "Failed to open file." << std::endl;
      return;
    }
    out << "key,cmp" << std::endl;
    for (size_t i = 0; i < key_cmp_stats.size(); i++) {
      out << key_cmp_stats[i].first << "," << key_cmp_stats[i].second << std::endl;
    }
  }
};

template <class KEY_TYPE, class PAYLOAD_TYPE>
void alexInterface<KEY_TYPE, PAYLOAD_TYPE>::bulk_load(
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value, size_t num, Param *param) {
  index.bulk_load(key_value, (int)num);
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool alexInterface<KEY_TYPE, PAYLOAD_TYPE>::get(KEY_TYPE key, PAYLOAD_TYPE &val,
                                                Param *param) {
#ifdef PROFILING
  long long key_num_exp_search_iterations = 0;
  PAYLOAD_TYPE *res = index.get_payload(key, key_num_exp_search_iterations);
  if (key_cmp_distribution.size() <= key_num_exp_search_iterations) {
    key_cmp_distribution.resize(key_num_exp_search_iterations + 1, 0);
  }
  key_cmp_distribution[key_num_exp_search_iterations]++;
  key_cmp_stats.push_back(std::make_pair(key, key_num_exp_search_iterations));
#else
  PAYLOAD_TYPE *res = index.get_payload(key);
#endif
  if (res != nullptr) {
    val = *res;
    return true;
  }
  return false;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool alexInterface<KEY_TYPE, PAYLOAD_TYPE>::put(KEY_TYPE key,
                                                PAYLOAD_TYPE value,
                                                Param *param) {
  return index.insert(key, value).second;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool alexInterface<KEY_TYPE, PAYLOAD_TYPE>::update(KEY_TYPE key,
                                                   PAYLOAD_TYPE value,
                                                   Param *param) {
  // return index.update(key, value);
  return false;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool alexInterface<KEY_TYPE, PAYLOAD_TYPE>::remove(KEY_TYPE key, Param *param) {
  auto num_erase = index.erase(key);
  return num_erase > 0;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
size_t alexInterface<KEY_TYPE, PAYLOAD_TYPE>::scan(
    KEY_TYPE key_low_bound, size_t key_num,
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *result, Param *param) {
  auto iter = index.lower_bound(key_low_bound);
  int scan_size = 0;
  for (scan_size = 0; scan_size < key_num && !iter.is_end(); scan_size++) {
    result[scan_size] = {(*iter).first, (*iter).second};
    iter++;
  }
  return scan_size;
}