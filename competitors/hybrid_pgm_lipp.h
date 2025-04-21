#pragma once

#include "base.h"
#include "dynamic_pgm_index.h"
#include "lipp_index.h"

template<class KeyType>
class HybridPGMLIPP : public Base<KeyType> {
public:
    HybridPGMLIPP(const std::vector<int>& params)
        : dp_(params), lipp_(params), flush_threshold_(1000000), num_buffered_(0) {}

    // ----------------------------
    // BUILD: Load initial data into LIPP
    // ----------------------------
    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        return lipp_.Build(data, num_threads);
    }

    // ----------------------------
    // INSERT: Insert into DPGM and flush if needed
    // ----------------------------
    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        dp_.Insert(data, thread_id);
        ++num_buffered_;

        if (num_buffered_ >= flush_threshold_) {
            FlushToLIPP(thread_id);
        }
    }

    // ----------------------------
    // LOOKUP: Try DPGM, fallback to LIPP
    // ----------------------------
    size_t EqualityLookup(const KeyType& key, uint32_t thread_id) const {
        size_t result = dp_.EqualityLookup(key, thread_id);
        if (result == util::OVERFLOW || result == util::NOT_FOUND) {
            return lipp_.EqualityLookup(key, thread_id);
        }
        return result;
    }

    // ----------------------------
    // RANGE QUERY: Query both indexes
    // ----------------------------
    uint64_t RangeQuery(const KeyType& lower_key, const KeyType& upper_key, uint32_t thread_id) const {
        return dp_.RangeQuery(lower_key, upper_key, thread_id) +
               lipp_.RangeQuery(lower_key, upper_key, thread_id);
    }

    // ----------------------------
    // SIZE: Return combined index size
    // ----------------------------
    std::size_t size() const {
        return dp_.size() + lipp_.size();
    }

    // ----------------------------
    // NAME / VARIANT / APPLICABLE
    // ----------------------------
    std::string name() const {
        return "HybridPGMLIPP";
    }

    std::vector<std::string> variants() const {
        return std::vector<std::string>();
    }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) const {
        return unique && !multithread;
    }

private:
    // ----------------------------
    // FLUSH DPGM → LIPP
    // ----------------------------
    void FlushToLIPP(uint32_t thread_id) {
        std::vector<KeyValue<KeyType>> buffer = dp_.Export();  // Must implement this in DPGM
        std::sort(buffer.begin(), buffer.end(), [](const auto& a, const auto& b) {
            return a.key < b.key;
        });

        for (const auto& kv : buffer) {
            lipp_.Insert(kv, thread_id);
        }

        dp_.Clear();  // Must implement this in DPGM
        num_buffered_ = 0;
    }

    // Indexes
    DynamicPGM<KeyType, LinearSearch, 16> dp_;
    Lipp<KeyType> lipp_;

    size_t num_buffered_;
    size_t flush_threshold_;
};
