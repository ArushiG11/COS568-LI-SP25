#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <chrono>

#include "base.h"
#include "dynamic_pgm_index.h"
#include "lipp.h"
#include "searches/branching_binary_search.h"

// Hybrid index with double buffering and asynchronous flushing

template<class KeyType>
class HybridDoubleBuffer : public Base<KeyType> {
public:
    HybridDoubleBuffer(const std::vector<int>& params)
        : dp_active_(params), dp_flushing_(params), lipp_index_(params), stop_thread_(false), flushing_needed_(false), insert_count_(0) {
        // Start background flush thread
        background_thread_ = std::thread([this]() { FlushMonitor(); });
    }

    ~HybridDoubleBuffer() {
        stop_thread_ = true;
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
    }

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        return lipp_index_.Build(data, num_threads);  // bulk load into LIPP
    }

    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        dp_active_.Insert(data, thread_id);
        ++insert_count_;

        if (insert_count_ >= flush_threshold_ && !flushing_needed_) {
            std::lock_guard<std::mutex> lock(swap_mutex_);
            std::swap(dp_active_, dp_flushing_);
            insert_count_ = 0;
            flushing_needed_ = true;
        }
    }

    size_t EqualityLookup(const KeyType& key, uint32_t thread_id) const {
        size_t result = dp_active_.EqualityLookup(key, thread_id);
        if (result != util::NOT_FOUND && result != util::OVERFLOW) return result;

        result = dp_flushing_.EqualityLookup(key, thread_id);
        if (result != util::NOT_FOUND && result != util::OVERFLOW) return result;

        return lipp_index_.EqualityLookup(key, thread_id);
    }

    uint64_t RangeQuery(const KeyType& lower, const KeyType& upper, uint32_t thread_id) const {
        return dp_active_.RangeQuery(lower, upper, thread_id) +
               dp_flushing_.RangeQuery(lower, upper, thread_id) +
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

private:
    // Indexes
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 256> dp_active_;
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 256> dp_flushing_;
    Lipp<KeyType> lipp_index_;

    // Double buffering state
    std::atomic<bool> flushing_needed_;
    std::atomic<bool> stop_thread_;
    std::thread background_thread_;
    std::mutex swap_mutex_;

    // Flush threshold
    size_t insert_count_;
    const size_t flush_threshold_ = 500000;

    void FlushMonitor() {
        while (!stop_thread_) {
            if (flushing_needed_) {
                FlushToLIPP();
                flushing_needed_ = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void FlushToLIPP() {
        auto buffer = dp_flushing_.Export();
        std::sort(buffer.begin(), buffer.end(), [](const KeyValue<KeyType>& a, const KeyValue<KeyType>& b) {
            return a.key < b.key;
        });
        for (const auto& kv : buffer) {
            lipp_index_.Insert(kv, 0); // Optionally optimize for bulk
        }
        dp_flushing_.Clear();
    }
};