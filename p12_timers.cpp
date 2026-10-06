// P12 - Software timers and 32-bit tick wrap-around (about 35 min).
//
// A millisecond counter is a uint32_t. It wraps after about 49.7 days.
// Comparing with `now > start + timeout` is wrong once the sum wraps.
//
//   deadline_reached(now, deadline)
//       true when now is at or past deadline, including across the wrap,
//       as long as the two instants are less than 2^31 ms apart.
//
//   TimerService holds 8 slots.
//       start(now, period_ms, periodic, callback, context)
//           returns an id, or nullopt when every slot is in use.
//           The callback is a function pointer plus a void* (no std::function).
//       stop(id)     the slot can be started again.
//       tick(now)    calls each expired timer at most once.
//
// A periodic timer must not drift and must not catch up in a burst: the next
// deadline is the previous deadline plus the period, repeated until it lies
// after now. A late tick therefore fires the callback once, not many times.
#include "check.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

using TimerCallback = void (*)(void* context);

// ===== YOUR CODE =====

// True when now is at or after deadline. Must work across the 2^32 wrap-around,
// as long as now and deadline are less than 2^31 ms apart.
constexpr bool deadline_reached(std::uint32_t now, std::uint32_t deadline)
{
    // Signed difference: negative when now is still before deadline across the wrap.
    return static_cast<std::int32_t>(now - deadline) >= 0;
}

class TimerService {
public:
    static constexpr std::size_t kCapacity = 8;

    // Starts a timer that expires period_ms after now. Returns its id, or nullopt when all slots are used.
    std::optional<std::size_t> start(std::uint32_t now, std::uint32_t period_ms, bool periodic,
                                     TimerCallback callback, void* context)
    {
        for (std::size_t id = 0; id < kCapacity; ++id) {
            if (slots_[id].active) {
                continue;
            }
            Slot& slot = slots_[id];
            slot.active = true;
            slot.periodic = periodic;
            slot.period = period_ms;
            slot.deadline = now + period_ms;
            slot.callback = callback;
            slot.context = context;
            return id;
        }
        return std::nullopt;
    }

    void stop(std::size_t id)
    {
        if (id < kCapacity) {
            slots_[id].active = false;
        }
    }

    // Calls every expired timer at most once. A periodic timer's next deadline is the first
    // deadline + k * period that lies after now: no drift, and no burst of catch-up calls.
    void tick(std::uint32_t now)
    {
        for (Slot& slot : slots_) {
            if (!slot.active || !deadline_reached(now, slot.deadline)) {
                continue;
            }
            slot.callback(slot.context);
            if (!slot.periodic) {
                slot.active = false;
                continue;
            }
            do {
                slot.deadline += slot.period;
            } while (deadline_reached(now, slot.deadline));
        }
    }

private:
    struct Slot {
        bool active = false;
        bool periodic = false;
        std::uint32_t period = 0;
        std::uint32_t deadline = 0;
        TimerCallback callback = nullptr;
        void* context = nullptr;
    };

    std::array<Slot, kCapacity> slots_{};
};

// ===== TESTS (do not edit) =====

void count_call(void* context) { ++*static_cast<int*>(context); }

int main() {
    CHECK(deadline_reached(100, 100));
    CHECK(!deadline_reached(99, 100));
    CHECK(deadline_reached(101, 100));
    CHECK(!deadline_reached(0xFFFF'FFF0u, 0x10u));
    CHECK(deadline_reached(0x10u, 0xFFFF'FFF0u));
    CHECK(!deadline_reached(0x0Fu, 0x10u));
    constexpr bool kCompileTime = deadline_reached(5, 3);
    CHECK(kCompileTime);

    {  // one-shot timer across the wrap-around
        TimerService timers;
        int fired = 0;
        const auto id = timers.start(0xFFFF'FF00u, 0x200u, false, count_call, &fired);
        CHECK(id.has_value());
        timers.tick(0xFFFF'FFF0u);
        CHECK(fired == 0);
        timers.tick(0x0000'00FFu);
        CHECK(fired == 0);
        timers.tick(0x0000'0100u);
        CHECK(fired == 1);
        timers.tick(0x0000'0200u);
        CHECK(fired == 1);
    }
    {  // periodic timer: no drift, at most one call per tick
        TimerService timers;
        int fired = 0;
        CHECK(timers.start(0, 10, true, count_call, &fired).has_value());
        timers.tick(5);
        CHECK(fired == 0);
        timers.tick(10);
        CHECK(fired == 1);
        timers.tick(20);
        CHECK(fired == 2);
        timers.tick(30);
        CHECK(fired == 3);
        timers.tick(55);
        CHECK(fired == 4);
        timers.tick(59);
        CHECK(fired == 4);
        timers.tick(60);
        CHECK(fired == 5);
    }
    {  // stop
        TimerService timers;
        int fired = 0;
        const auto id = timers.start(0, 10, false, count_call, &fired);
        CHECK(id.has_value());
        if (id) {
            timers.stop(*id);
        }
        timers.tick(100);
        CHECK(fired == 0);
    }
    {  // capacity and slot reuse
        TimerService timers;
        int fired = 0;
        std::array<std::optional<std::size_t>, TimerService::kCapacity> ids{};
        for (auto& id : ids) {
            id = timers.start(0, 1000, false, count_call, &fired);
        }
        bool all_started = true;
        for (const auto& id : ids) {
            all_started = all_started && id.has_value();
        }
        CHECK(all_started);
        CHECK(!timers.start(0, 1000, false, count_call, &fired).has_value());
        if (ids[3]) {
            timers.stop(*ids[3]);
        }
        CHECK(timers.start(0, 1000, false, count_call, &fired).has_value());
        timers.tick(1000);
        CHECK(fired == 8);
    }
    {  // independent timers and contexts
        TimerService timers;
        int fast = 0;
        int slow = 0;
        CHECK(timers.start(0, 10, true, count_call, &fast).has_value());
        CHECK(timers.start(0, 25, true, count_call, &slow).has_value());
        for (std::uint32_t t = 0; t <= 100; t += 5) {
            timers.tick(t);
        }
        CHECK(fast == 10);
        CHECK(slow == 4);
    }
    return report();
}
