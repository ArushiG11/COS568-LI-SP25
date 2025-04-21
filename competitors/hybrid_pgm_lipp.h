#pragma once

#include "base.h"
#include "dynamic_pgm.h"
#include "lipp.h"

template<class KeyType>
class HybridPGMLIPP : public Base<KeyType> {
public:
    HybridPGMLIPP(const std::vector<int>& params) 
        : dp_index_(params), lipp_index_(params) {}

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        // Build phase uses LIPP for initial data
        return lipp_index_.Build(data, num_threads);
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
        if (result == util::OVERFLOW || result == util::NOT_FOUND) {
            return lipp_index_.EqualityLookup(key, thread_id);
        }
        return result;
    }

    uint64_t RangeQuery(const KeyType& lower, const KeyType& upper, uint32_t thread_id) const {
        // Optional: support from both indexes, combining results
        uint64_t total = 0;
        total += dp_index_.RangeQuery(lower, upper, thread_id);
        total += lipp_index_.RangeQuery(lower, upper, thread_id);
        return total;
    }

    std::string name() const override {
        return "HybridPGMLIPP";
    }

    std::vector<std::string> variants() const override {
        return {};
    }

    size_t size() const override {
        return dp_index_.size() + lipp_index_.size();
    }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) override {
        return unique && !multithread;
    }

private:
    mutable DynamicPGM<KeyType, BinarySearch, 16> dp_index_; // Replace BinarySearch/16 if needed
    Lipp<KeyType> lipp_index_;
    size_t insert_count_ = 0;
    const size_t flush_threshold_ = 500000;

    void FlushToLIPP() {
        std::vector<KeyValue<KeyType>> buffer = dp_index_.Export();
        for (const auto& kv : buffer) {
            lipp_index_.Insert(kv, 0);
        }
        dp_index_.Clear();
    }
};
