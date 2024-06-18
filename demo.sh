# basic usage test. do it after compile
./build/microbench \
--keys_file=datasets/planet \
--keys_file_type=binary \
--read=1.0 --insert=0.0 \
--update=0.0 --scan=0.0 \
--operations_num=200000000 \
--table_size=-1 \
--init_table_ratio=1.0 \
--thread_num=32 \
--index=btreeolc

# main test without latency measurement
./build/microbench \
--keys_file=datasets/osm \
--keys_file_type=binary \
--read=0.0 --insert=0.0 \
--update=0.0 --scan=0.0  --delete=0.0 \
--test_suite=99 \
--operations_num=0 \
--table_size=-1 \
--init_table_ratio=0.5 \
--thread_num=1 \
--index=dili

# main test with latency measurement (as noticed, it will slow down by up to 15% qps)
./build/microbench \
--keys_file=datasets/covid \
--keys_file_type=binary \
--read=0.0 --insert=0.0 \
--update=0.0 --scan=0.0  --delete=0.0 \
--test_suite=1 \
--operations_num=0 \
--table_size=-1 \
--init_table_ratio=0.5 \
--thread_num=1 \
--latency_sample=1 \
--index=alex