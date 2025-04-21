#pragma once

#include "base.h"
#include "dynamic_pgm_index.h"
#include "lipp.h"
#include "searches/branching_binary_search.h" // for BranchingBinarySearch

template<class KeyType>
class HybridPGMLIPP : public Base<KeyType> {
public:
    HybridPGMLIPP(const std::vector<int>& params) 
        : dp_index_(params), lipp_index_(params) {}

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        return lipp_index_.Build(data, num_threads);  // bulk load into LIPP
    }

    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        dp_index_.Insert(data, thread_id);
        ++insert_count_;
        if (insert_count_ >= flush_threshold_) {
            FlushToLIPP();
            insert_count_ = 0;
        }
    }

    size_t EqualityLookup(const KeyType& key, uint32_t thread_id) const {
        size_t result = dp_index_.EqualityLookup(key, thread_id);
        if (result == util::NOT_FOUND || result == util::OVERFLOW) {
            return lipp_index_.EqualityLookup(key, thread_id);
        }
        return result;
    }

    uint64_t RangeQuery(const KeyType& lower, const KeyType& upper, uint32_t thread_id) const {
        return dp_index_.RangeQuery(lower, upper, thread_id) + 
               lipp_index_.RangeQuery(lower, upper, thread_id);
    }

    std::string name() const  {
        return "HybridPGMLIPP";
    }

    std::vector<std::string> variants() const  {
        return {};
    }

    size_t size() const  {
        return dp_index_.size() + lipp_index_.size();
    }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename)  {
        return unique && !multithread;
    }

private:
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 256> dp_index_;
    Lipp<KeyType> lipp_index_;
    size_t insert_count_ = 0;
    const size_t flush_threshold_ = 500000;

    void FlushToLIPP() {
        auto buffer = dp_index_.Export();
        for (const auto& kv : buffer) {
            lipp_index_.Insert(kv, 0); 
        }
        dp_index_.Clear();
    }
};
