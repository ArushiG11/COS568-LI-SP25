// benchmarks/benchmark_hybrid_pgm_lipp.cc

#include "benchmarks/common.h"
#include "competitors/hybrid_pgm_lipp.h"

void benchmark_64_hybrid_pgm_lipp(tli::Benchmark<uint64_t>& benchmark, const std::string& filename) {
  benchmark.template Run<HybridPGMLIPP<uint64_t>>();
}

REGISTER_INDEX(benchmark_64_hybrid_pgm_lipp);
