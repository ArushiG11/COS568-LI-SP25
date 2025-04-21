#include "benchmarks/benchmark_hybrid_pgm_lipp.h"

#include "benchmark.h"
#include "benchmarks/common.h"
#include "competitors/hybrid_pgm_lipp.h"

int main(int argc, char** argv) {
    // Initialize Benchmark with arguments
    tli::Benchmark<uint64_t> benchmark;
    benchmark::InitBenchmark(benchmark, argc, argv);

    // Run the hybrid benchmark
    benchmark_64_hybrid_pgm_lipp(benchmark);
    return 0;
}
