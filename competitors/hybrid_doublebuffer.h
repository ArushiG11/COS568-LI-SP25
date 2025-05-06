#pragma once

#include <thread>
#include <atomic>
#include <mutex>

#include "base.h"
#include "dynamic_pgm_index.h"
#include "lipp.h"
#include "searches/branching_binary_search.h"

template<class KeyType>
class HybridDoubleBuffer : public Base<KeyType> {
public:
    HybridDoubleBuffer(const std::vector<int>& params)
        : dp_active_(params), dp_flushing_(params), lipp_index_(params) {}

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        return lipp_index_.Build(data, num_threads);  // bulk load into LIPP
    }

    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        dp_active_.Insert(data, thread_id);
        ++insert_count_;

        if (insert_count_ >= flush_threshold_ && !flushing_in_progress_) {
            StartFlushThread();  // Trigger background flush
        }
    }

    size_t EqualityLookup(const KeyType& key, uint32_t thread_id) const {
        size_t result = dp_active_.EqualityLookup(key, thread_id);
        if (result == util::NOT_FOUND || result == util::OVERFLOW) {
            return lipp_index_.EqualityLookup(key, thread_id);
        }
        return result;
    }

    uint64_t RangeQuery(const KeyType& lower, const KeyType& upper, uint32_t thread_id) const {
        return dp_active_.RangeQuery(lower, upper, thread_id) + 
               lipp_index_.RangeQuery(lower, upper, thread_id);
    }

    std::string name() const {
        return "HybridDoubleBuffer";
    }

    std::vector<std::string> variants() const {
        return {};
    }

    size_t size() const {
        return dp_active_.size() + dp_flushing_.size() + lipp_index_.size();
    }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) {
        return unique && !multithread;
    }

    ~HybridDoubleBuffer() {
        if (flush_thread_.joinable()) {
            flush_thread_.join();  // Wait for background flush to finish
        }
    }

private:
    // Buffers
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 256> dp_active_;
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 256> dp_flushing_;
    Lipp<KeyType> lipp_index_;

    size_t insert_count_ = 0;
    const size_t flush_threshold_ = 500000;

    // Background flushing state
    std::atomic<bool> flushing_in_progress_{false};
    std::thread flush_thread_;
    std::mutex flush_mutex_;  // Optional if you expect concurrent lookups

    void StartFlushThread() {
        flushing_in_progress_ = true;

        // Swap active and flushing buffers
        std::swap(dp_active_, dp_flushing_);
        insert_count_ = 0;

        // Start background thread
        flush_thread_ = std::thread([this]() {
            FlushToLIPP();
            flushing_in_progress_ = false;
        });
    }

    void FlushToLIPP() {
        auto buffer = dp_flushing_.Export();
        for (const auto& kv : buffer) {
            lipp_index_.Insert(kv, 0);
        }
        dp_flushing_.Clear();
    }
};
