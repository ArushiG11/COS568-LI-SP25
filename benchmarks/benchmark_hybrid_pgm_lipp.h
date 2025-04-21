// benchmarks/benchmark_hybrid_pgm_lipp.h

#pragma once

#include "benchmark_util.h"
#include "hybrid_pgm_lipp.h"

template <typename KeyType, typename PayloadType>
class HybridPGMLIPPBenchmark : public BaseIndexBenchmark<KeyType, PayloadType> {
 public:
  using IndexType = HybridPGMLIPP<KeyType, PayloadType>;

  explicit HybridPGMLIPPBenchmark(const BenchmarkConfig& config)
      : BaseIndexBenchmark<KeyType, PayloadType>(config) {}

  std::string GetName() const override {
    return "HybridPGMLIPP";
  }

  std::unique_ptr<BaseIndex<KeyType, PayloadType>> CreateIndex() const override {
    return std::make_unique<IndexType>();
  }
};
