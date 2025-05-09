#include "benchmarks/benchmark_hybrid_pgm_lipp.h"
#include "competitors/hybrid_pgm_lipp.h"

void benchmark_64_hybrid_pgm_lipp(tli::Benchmark<uint64_t>& benchmark) {
    benchmark.template Run<HybridPGMLIPP<uint64_t>>();
}
