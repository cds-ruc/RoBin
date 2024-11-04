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
  } else if (index_type == "dili") {
    index = new diliInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "btreeolc") {
    index = new BTreeOLCInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "artolc") {
    index = new ARTOLCInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "xindex") {
    index = new xindexInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "finedex") {
    index = new finedexInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "alexolc") {
    index = new alexolInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "sali") {
    index = new SALIInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "lippolc") {
    index = new LIPPOLInterface<KEY_TYPE, PAYLOAD_TYPE>;
  } else if (index_type == "masstree") {
    index = new MasstreeInterface<KEY_TYPE, PAYLOAD_TYPE>;
  }

  else {
    std::cout << "Could not find a matching index called " << index_type
              << ".\n";
    exit(0);
  }

  return index;
}