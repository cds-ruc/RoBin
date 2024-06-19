#include "../indexInterface.h"
#include "./src/src/butree/buInterval.cpp"
#include "./src/src/butree/interval.cpp"
#include "./src/src/butree/interval_utils.cpp"
#include "./src/src/butree/linearRegressor.cpp"
#include "./src/src/dili/DILI.cpp"
#include "./src/src/dili/DILI.h"
#include "./src/src/dili/diliNode.cpp"
#include "./src/src/global/global.cpp"
#include "./src/src/global/linearReg.cpp"
#include "./src/src/utils/data_utils.cpp"
#include "./src/src/utils/data_utils.h"
#include "./src/src/utils/file_utils.cpp"
#include "./src/src/utils/file_utils.h"
#include "./src/src/utils/linux_sys_utils.cpp"

#include <filesystem>
#include <utility>
#include <vector>

template <class KEY_TYPE, class PAYLOAD_TYPE>

class diliInterface final: public indexInterface<KEY_TYPE, PAYLOAD_TYPE> {
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

  long long memory_consumption() { return 0; }
private:
  DILI dili;
};

template <class KEY_TYPE, class PAYLOAD_TYPE>
void diliInterface<KEY_TYPE, PAYLOAD_TYPE>::bulk_load(
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *key_value, size_t num, Param *param) {
  std::vector<pair<keyType, recordPtr>> bulk_data(num);

  string mirror_dir = "src/competitor/dili/src/build/data/buTree";
  int status = file_utils::path_status(mirror_dir);
  assert(status != 2);
  if (status == 0) {
    file_utils::detect_and_create_dir(mirror_dir);
  } else { // == 1, is a directory
    // delete data/buTree/ and create it again
    std::filesystem::remove_all(mirror_dir);
    file_utils::detect_and_create_dir(mirror_dir);
  }
  dili.set_mirror_dir(mirror_dir);
  bool check_reinterpret = true;
  for (long i = 0; i < num; i++) {
    bulk_data.emplace_back(
        make_pair((long)(key_value[i].first), (long)(key_value[i].second)));
    if(check_reinterpret && (long)(key_value[i].first)!=key_value[i].first){
      std::cout<<"Warning: key type is not long, reinterpret it as long"<<std::endl;
      std::cout<<key_value[i].first<<" -> "<<(long)(key_value[i].first)<<std::endl;
      // check_reinterpret = false;
    }
  }
  dili.bulk_load(bulk_data);
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool diliInterface<KEY_TYPE, PAYLOAD_TYPE>::get(KEY_TYPE key, PAYLOAD_TYPE &val,
                                                Param *param) {
  PAYLOAD_TYPE res = (long)(dili.search((long)key));
  if (res != -1) { // dili.search return -1 when error
    val = res;
    return true;
  }
  return false;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool diliInterface<KEY_TYPE, PAYLOAD_TYPE>::put(KEY_TYPE key,
                                                PAYLOAD_TYPE value,
                                                Param *param) {
  return dili.insert((long)(key), (long)(value));
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool diliInterface<KEY_TYPE, PAYLOAD_TYPE>::update(KEY_TYPE key,
                                                   PAYLOAD_TYPE value,
                                                   Param *param) {
  return false;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
bool diliInterface<KEY_TYPE, PAYLOAD_TYPE>::remove(KEY_TYPE key, Param *param) {
  auto num_erase = dili.delete_key((long)(key));
  return num_erase > 0;
}

template <class KEY_TYPE, class PAYLOAD_TYPE>
size_t diliInterface<KEY_TYPE, PAYLOAD_TYPE>::scan(
    KEY_TYPE key_low_bound, size_t key_num,
    std::pair<KEY_TYPE, PAYLOAD_TYPE> *result, Param *param) {
  return 0;
}