#include "../indexInterface.h"
#include "./src/src/DyTIS.h"
#include "./src/src/DyTIS_impl.h"
#include <csignal>

template <class KEY_TYPE, class PAYLOAD_TYPE>
class dytisInterface final : public indexInterface<KEY_TYPE, PAYLOAD_TYPE> {
public:
  dytisInterface() { std::signal(SIGTERM, handleSignal); }
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

  long long memory_consumption() { return 0; }

  void print_stats(std::string s) { return; }

public:
  void static handleSignal(int signal) {
    if (signal == SIGTERM) {
      printf("dytis timeout, dytis_insert_succ: %ld\n", dytis_insert_succ);
      exit(1);
    }
  }
  inline static uint64_t dytis_insert_succ = 0;

private:
  DyTIS index;
};

template <class KEY_TYPE, class PAYLOAD_TYPE>
void dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::bulk_load(
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value, size_t num, Param *param) {
  dytis_insert_succ = 0;
  for (auto i = 0; i < num; i++) {
    index.Insert(key_value[i].first, key_value[i].second);
    dytis_insert_succ++;
  }
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::get(KEY_TYPE key,
                                                 PAYLOAD_TYPE &val,
                                                 Param *param) {
  PAYLOAD_TYPE *res = index.Find(key);
  if (res != nullptr) {
    val = *res;
    return true;
  }
  return false;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::put(KEY_TYPE key,
                                                 PAYLOAD_TYPE value,
                                                 Param *param) {
  index.Insert(key, value);
  dytis_insert_succ++;
  return true;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::update(KEY_TYPE key,
                                                    PAYLOAD_TYPE value,
                                                    Param *param) {
  return index.Update(key, value);
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::remove(KEY_TYPE key,
                                                    Param *param) {
  return index.Delete(key);
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
size_t dytisInterface<KEY_TYPE, PAYLOAD_TYPE>::scan(
    KEY_TYPE key_low_bound, size_t key_num,
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *result, Param *param) {

  auto iter = index.Scan(key_low_bound, key_num);
  return key_num;
}