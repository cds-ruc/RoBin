#include "./benchmark.h"
#include <csignal>
void handleSignal(int signal) {
    if (signal == SIGTERM) {
        printf("dytis timeout, dytis_insert_succ: %ld\n",dytis_insert_succ);
        exit(1);
    }
}

int main(int argc, char **argv) {
    std::signal(SIGTERM, handleSignal);
    Benchmark <uint64_t, uint64_t> bench;
    bench.parse_args(argc, argv);
    bench.run_benchmark();
}

