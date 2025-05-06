#include "benchmarks/benchmark_doublebuffer.h"
#include "competitors/hybrid_pgm_lipp.h"

void benchmark_64_doublebuffer(tli::Benchmark<uint64_t>& benchmark) {
    benchmark.template Run<HybridDoubleBuffer<uint64_t>>();
}
