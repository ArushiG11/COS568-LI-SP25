#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <algorithm>
#include <chrono>
#include <vector>

#include "base.h"
#include "dynamic_pgm_index.h"
#include "lipp.h"
#include "searches/branching_binary_search.h"

// Optimized Hybrid Double Buffer Index

template<class KeyType>
class HybridDoubleBuffer : public Base<KeyType> {
public:
    HybridDoubleBuffer(const std::vector<int>& params)
        : dp_active_(params), dp_flushing_(params), lipp_index_(params),
          stop_thread_(false), flushing_(false), insert_count_(0),
          flush_threshold_(100000), last_flush_duration_(0) {
        background_thread_ = std::thread([this]() { FlushMonitor(); });
    }

    ~HybridDoubleBuffer() {
        stop_thread_ = true;
        flush_condition_.notify_one();
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
    }

    uint64_t Build(const std::vector<KeyValue<KeyType>>& data, size_t num_threads) {
        return lipp_index_.Build(data, num_threads);
    }

    void Insert(const KeyValue<KeyType>& data, uint32_t thread_id) {
        dp_active_.Insert(data, thread_id);
        const size_t new_count = insert_count_.fetch_add(1, std::memory_order_relaxed) + 1;

        if (new_count >= flush_threshold_.load(std::memory_order_relaxed) && 
            !flushing_.exchange(true, std::memory_order_acq_rel)) {
            std::lock_guard<std::mutex> lock(swap_mutex_);
            std::swap(dp_active_, dp_flushing_);
            insert_count_.store(0, std::memory_order_relaxed);
            flush_condition_.notify_one();
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

    std::string name() const { return "HybridDoubleBuffer"; }
    std::vector<std::string> variants() const { return {}; }
    size_t size() const { return dp_active_.size() + dp_flushing_.size() + lipp_index_.size(); }

    bool applicable(bool unique, bool range_query, bool insert, bool multithread, const std::string& ops_filename) {
        return unique && !multithread;
    }

private:
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 64> dp_active_;
    mutable DynamicPGM<uint64_t, BranchingBinarySearch<1>, 64> dp_flushing_;
    Lipp<KeyType> lipp_index_;

    std::atomic<bool> flushing_;
    std::atomic<bool> stop_thread_;
    std::thread background_thread_;

    std::mutex swap_mutex_;
    std::condition_variable flush_condition_;
    std::mutex flush_cv_mutex_;

    std::atomic<size_t> insert_count_;
    std::atomic<size_t> flush_threshold_;
    std::atomic<uint64_t> last_flush_duration_;

    void FlushMonitor() {
        while (true) {
            std::unique_lock<std::mutex> lock(flush_cv_mutex_);
            flush_condition_.wait(lock, [this]() { return flushing_.load() || stop_thread_.load(); });

            if (stop_thread_) break;
            if (flushing_) {
                FlushToLIPP();
                flushing_.store(false, std::memory_order_release);
            }
        }
    }

    void FlushToLIPP() {
        auto flush_start = std::chrono::high_resolution_clock::now();

        auto buffer = dp_flushing_.Export();
        dp_flushing_.Clear();

        std::sort(buffer.begin(), buffer.end(), [](const auto& a, const auto& b) {
            return a.key < b.key;
        });

        for (const auto& kv : buffer) {
            lipp_index_.Insert(kv, 0);
        }

        auto flush_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(flush_end - flush_start);
        last_flush_duration_.store(duration.count(), std::memory_order_relaxed);

        if (duration > std::chrono::milliseconds(200)) {
            flush_threshold_.store(std::max(size_t(50000), flush_threshold_.load() / 2));
        } else {
            flush_threshold_.store(std::min(size_t(1000000), flush_threshold_.load() + 50000));
        }
    }
};
