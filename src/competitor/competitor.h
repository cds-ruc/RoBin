#include "./alex/alex.h"
#include "./art/art.h"
#include "./dili/dili.h"
#include "./dytis/dytis.h"
#include "./indexInterface.h"
#include "./lipp/lipp.h"
#include "./btree/btree.h"
#include "./pgm/pgm.h"
// #include "./bptree/bptree.h"
#include <iostream>

template <class KEY_TYPE, class PAYLOAD_TYPE>
indexInterface<KEY_TYPE, PAYLOAD_TYPE> *get_index(std::string index_type) {
  indexInterface<KEY_TYPE, PAYLOAD_TYPE> *index;
  if (index_type == "alex") {
    index = new alexInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "pgm") {
    index = new pgmInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "btree") {
    index = new BTreeInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "art") {
    index = new ARTInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "lipp") {
    index = new LIPPInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "dytis") {
    index = new dytisInterface<KEY_TYPE, PAYLOAD_TYPE>;
  }
  // else if (index_type == "bptree") {
  //   index = new BPTreeInterface<KEY_TYPE, PAYLOAD_TYPE>;
  // }
  else if (index_type == "dili") {
    index = new diliInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else {
    std::cout << "Could not find a matching index called " << index_type
              << ".\n";
    exit(0);
  }

  return index;
}