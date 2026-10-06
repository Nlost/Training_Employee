// P18 - Data shared between an "interrupt" and the main loop (about 20 min).
// Run with ThreadSanitizer (the default for this problem): ./run.sh 18
//
// A second thread plays the interrupt. timer_isr writes one sample and raises
// a ready flag; poll_sample takes that sample and clears the flag. If a new
// interrupt arrives before the sample was taken, count an overrun and keep
// the sample that is still waiting.
//
// The code below works by luck and has data races. The CHECKs can pass anyway.
// You are done when ThreadSanitizer stays quiet.
//
// Do not add a mutex. An interrupt handler must not block on a lock.
// Publish the sample so the reader cannot observe a half-written Sample:
// the flag is what hands the sample from one side to the other.
// The counters need the same treatment, or exactly one writer.
#include "check.hpp"

#include <atomic>
#include <cstdint>
#include <optional>
#include <thread>

struct Sample {
    std::uint32_t seq;
    std::uint32_t twice;
};

// ===== YOUR CODE =====
// This code has data races. Fix it without a mutex: an interrupt handler must never block.

Sample g_sample{};
std::atomic<bool> g_ready{false};
std::atomic<std::uint32_t> g_ticks{0};
std::atomic<std::uint32_t> g_overruns {0};

// Runs in "interrupt context" (the interrupt thread).
void timer_isr(std::uint32_t seq) {
    ++g_ticks;
    if (g_ready.load(std::memory_order_acquire)) {
        ++g_overruns;
        return;
    }
    g_sample = Sample{seq, seq * 2};
    g_ready.store(true, std::memory_order_release);
}



// Runs in the main loop.
std::optional<Sample> poll_sample() {
    if (!g_ready.load(std::memory_order_acquire)) {
        return std::nullopt;
    }
    const Sample sample = g_sample;
    g_ready.store(false, std::memory_order_release);
    return sample;
}

// ===== TESTS (do not edit) =====

int main() {
    constexpr std::uint32_t kInterrupts = 200'000;
    std::atomic<bool> done{false};
    std::thread interrupt_source([&] {
        for (std::uint32_t seq = 1; seq <= kInterrupts; ++seq) {
            timer_isr(seq);
            if (seq % 64 == 0) {
                std::this_thread::yield();
            }
        }
        done.store(true);
    });

    std::uint32_t received = 0;
    std::uint32_t last_seq = 0;
    bool consistent = true;
    bool increasing = true;
    for (;;) {
        const bool finished = done.load();
        while (const auto sample = poll_sample()) {
            ++received;
            consistent = consistent && sample->twice == sample->seq * 2;
            increasing = increasing && sample->seq > last_seq;
            last_seq = sample->seq;
        }
        if (finished) {
            break;
        }
    }
    interrupt_source.join();

    CHECK(consistent);
    CHECK(increasing);
    CHECK(received > 0);
    CHECK(g_ticks == kInterrupts);
    CHECK(received + g_overruns == kInterrupts);
    return report();
}
