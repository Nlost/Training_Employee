// P19 - Bounded blocking queue (about 35 min).
//
// BlockingQueue<T, N> holds at most N items. Several threads may push and pop.
//
//   push(value)      blocks while the queue is full. Returns false if it is closed.
//   try_push(value)  never blocks. False when full or closed.
//   pop()            blocks while empty. After close(), still returns whatever
//                    is left, one item at a time, then nullopt.
//   close()          rejects new items and wakes every thread blocked in push or pop.
//
// A waiting thread must not wake up and then find the queue empty (or full)
// because the wakeup was spurious or another thread took the item. close()
// has to be visible to those waiters, or they sleep forever.
#include "check.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

// ===== YOUR CODE =====

template <typename T, std::size_t N>
class BlockingQueue {
public:
    // Blocks while full. False if the queue is closed.
    bool push(T value)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        not_full_.wait(lock, [this] { return closed_ || used_ < N; });
        if (closed_) {
            return false;
        }
        put(std::move(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }
    // Never blocks. False if full or closed.
    bool try_push(T value)
    {
        std::unique_lock<std::mutex> lock(mtx_);
        if (closed_ || used_ == N) {
            return false;
        }
        put(std::move(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }
    // Blocks while empty. After close(), still returns the remaining items, then nullopt.
    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        not_empty_.wait(lock, [this] { return closed_ || used_ > 0; });
        if (used_ == 0) {  // closed and drained
            return std::nullopt;
        }
        T value = std::move(queue_[head_]);
        head_ = (head_ + 1) % N;
        --used_;
        lock.unlock();
        not_full_.notify_one();
        return value;
    }
    // Wakes every blocked push() and pop().
    void close()
    {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

private:
    // Caller must hold mtx_ and have checked used_ < N.
    void put(T&& value)
    {
        queue_[(head_ + used_) % N] = std::move(value);
        ++used_;
    }

    std::size_t head_ = 0;  // index of the oldest item
    std::size_t used_ = 0;  // number of items stored
    bool closed_ = false;
    std::array<T, N> queue_{};
    std::mutex mtx_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
};

// ===== TESTS (do not edit) =====

int main() {
    {
        BlockingQueue<int, 2> q;
        CHECK(q.try_push(1));
        CHECK(q.try_push(2));
        CHECK(!q.try_push(3));
        CHECK(q.pop() == 1);
        CHECK(q.pop() == 2);
        q.close();
        CHECK(!q.push(4));
        CHECK(!q.try_push(5));
        CHECK(!q.pop().has_value());
    }
    {
        BlockingQueue<int, 4> q;
        CHECK(q.push(7));
        q.close();
        CHECK(q.pop() == 7);
        CHECK(!q.pop().has_value());
    }
    {  // 4 producers, 3 consumers
        constexpr std::size_t kProducers = 4;
        constexpr std::size_t kConsumers = 3;
        constexpr std::uint64_t kItemsEach = 25'000;
        BlockingQueue<std::uint64_t, 16> q;
        std::array<std::uint64_t, kConsumers> sums{};
        std::array<std::uint64_t, kConsumers> counts{};
        std::vector<std::thread> consumers;
        for (std::size_t c = 0; c < kConsumers; ++c) {
            consumers.emplace_back([&, c] {
                while (const auto v = q.pop()) {
                    sums[c] += *v;
                    ++counts[c];
                }
            });
        }
        std::vector<std::thread> producers;
        for (std::size_t p = 0; p < kProducers; ++p) {
            producers.emplace_back([&] {
                for (std::uint64_t i = 1; i <= kItemsEach; ++i) {
                    if (!q.push(i)) {
                        return;
                    }
                }
            });
        }
        for (auto& t : producers) {
            t.join();
        }
        q.close();
        for (auto& t : consumers) {
            t.join();
        }
        std::uint64_t total_sum = 0;
        std::uint64_t total_count = 0;
        for (std::size_t c = 0; c < kConsumers; ++c) {
            total_sum += sums[c];
            total_count += counts[c];
        }
        CHECK(total_count == kProducers * kItemsEach);
        CHECK(total_sum == kProducers * kItemsEach * (kItemsEach + 1) / 2);
    }
    {  // close() wakes a consumer blocked in pop()
        BlockingQueue<int, 4> q;
        std::atomic<bool> got_nullopt{false};
        std::thread consumer([&] { got_nullopt = !q.pop().has_value(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.close();
        consumer.join();
        CHECK(got_nullopt);
    }
    {  // close() wakes a producer blocked in push()
        BlockingQueue<int, 1> q;
        CHECK(q.push(1));
        std::atomic<bool> push_failed{false};
        std::thread producer([&] { push_failed = !q.push(2); });
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.close();
        producer.join();
        CHECK(push_failed);
    }
    return report();
}
