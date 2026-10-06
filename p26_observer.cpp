// P26 - Observer, fixed capacity (about 30 min).
//
// Bus notifies listeners in subscription order. No heap, and at most 4 listeners.
//
//   subscribe(IListener&)    false if full, or if this listener is already in
//   unsubscribe(IListener&)  no-op if it was not subscribed; remaining listeners
//                            keep their relative order
//   publish(code)            calls on_event(code) once per listener that is still
//                            subscribed when its turn comes
//   count()
//
// During publish a listener may unsubscribe itself or another listener, and may
// subscribe a new one:
//   - a listener removed before its turn is not called
//   - a listener added during this publish is not called until the next publish
//   - everyone who was subscribed at the start, and not removed before their
//     turn, is called exactly once
//
// IListener has a public virtual destructor and is not copyable.
#include "check.hpp"
#include <array>
#include <optional>
#include <cstdint>
#include <type_traits>

// ===== YOUR CODE =====

class IListener {
public:
    virtual ~IListener() = default;
    IListener(const IListener&) = delete;
    IListener& operator=(const IListener&) = delete;
    virtual void on_event(int code) = 0;

protected:
    IListener() = default;
};

class Bus {
public:
    bool subscribe(IListener& listener) {
        if (count_ == capacity_ || index_of(&listener) < count_) {
            return false;
        }
        slots_[count_++] = &listener;
        return true;
    }

    void unsubscribe(IListener& listener) {
        const std::size_t index = index_of(&listener);
        if (index == count_) {
            return;
        }
        for (std::size_t i = index + 1; i < count_; ++i) {
            slots_[i - 1] = slots_[i];
        }
        slots_[--count_] = nullptr;
    }

    void publish(int code) {
        // Snapshot who was subscribed when this publish started. A listener
        // added from a callback is not in the snapshot, so it waits until the
        // next publish. A listener removed before its turn fails the membership
        // check and is skipped.
        std::array<IListener*, capacity_> snapshot{};
        const std::size_t n = count_;
        for (std::size_t i = 0; i < n; ++i) {
            snapshot[i] = slots_[i];
        }
        for (std::size_t i = 0; i < n; ++i) {
            if (index_of(snapshot[i]) < count_) {
                snapshot[i]->on_event(code);
            }
        }
    }

    int count() const { return static_cast<int>(count_); }

private:
    static constexpr std::size_t capacity_ = 4;
    std::array<IListener*, capacity_> slots_{};
    std::size_t count_ = 0;

    std::size_t index_of(IListener* listener) const {
        for (std::size_t i = 0; i < count_; ++i) {
            if (slots_[i] == listener) {
                return i;
            }
        }
        return count_;
    }
};

// ===== TESTS (do not edit) =====

namespace {

struct Log {
    int ids[8]{};
    int n = 0;

    void add(int id) {
        if (n < 8) {
            ids[n++] = id;
        }
    }

    bool operator==(const Log& other) const {
        if (n != other.n) {
            return false;
        }
        for (int i = 0; i < n; ++i) {
            if (ids[i] != other.ids[i]) {
                return false;
            }
        }
        return true;
    }
};

class Probe : public IListener {
public:
    Probe(int id, Log& log) : id_(id), log_(log) {}
    void on_event(int) override { log_.add(id_); }

private:
    int id_;
    Log& log_;
};

class SelfUnsub : public IListener {
public:
    SelfUnsub(int id, Bus& bus, Log& log) : id_(id), bus_(bus), log_(log) {}
    void on_event(int) override {
        log_.add(id_);
        bus_.unsubscribe(*this);
    }

private:
    int id_;
    Bus& bus_;
    Log& log_;
};

class DropOther : public IListener {
public:
    DropOther(Bus& bus, IListener& victim, Log& log) : bus_(bus), victim_(victim), log_(log) {}
    void on_event(int) override {
        log_.add(1);
        bus_.unsubscribe(victim_);
    }

private:
    Bus& bus_;
    IListener& victim_;
    Log& log_;
};

class Adds : public IListener {
public:
    Adds(int id, Bus& bus, IListener& extra, Log& log)
        : id_(id), bus_(bus), extra_(extra), log_(log) {}

    void on_event(int) override {
        log_.add(id_);
        if (!added_) {
            added_ = bus_.subscribe(extra_);
        }
    }

private:
    int id_;
    Bus& bus_;
    IListener& extra_;
    Log& log_;
    bool added_ = false;
};

}  // namespace

int main() {
    static_assert(std::has_virtual_destructor_v<IListener>);
    static_assert(!std::is_copy_constructible_v<IListener>);
    static_assert(std::is_abstract_v<IListener>);

    Log log;
    Probe a(10, log);
    Probe b(20, log);
    Probe c(30, log);
    Probe d(40, log);
    Probe e(50, log);
    Bus bus;
    CHECK(bus.count() == 0);
    CHECK(bus.subscribe(a));
    CHECK(bus.subscribe(b));
    CHECK(bus.subscribe(c));
    CHECK(bus.subscribe(d));
    CHECK(!bus.subscribe(e));
    CHECK(!bus.subscribe(a));
    CHECK(bus.count() == 4);

    bus.publish(1);
    CHECK((log == Log{{10, 20, 30, 40}, 4}));

    bus.unsubscribe(b);
    bus.unsubscribe(b);
    CHECK(bus.count() == 3);
    CHECK(bus.subscribe(e));
    log = {};
    bus.publish(1);
    CHECK((log == Log{{10, 30, 40, 50}, 4}));

    Bus reenter;
    Log dropped;
    Probe victim(7, dropped);
    Probe tail(8, dropped);
    DropOther drop(reenter, victim, dropped);
    CHECK(reenter.subscribe(drop));
    CHECK(reenter.subscribe(victim));
    CHECK(reenter.subscribe(tail));
    reenter.publish(1);
    CHECK((dropped == Log{{1, 8}, 2}));
    CHECK(reenter.count() == 2);

    Bus self;
    Log self_log;
    SelfUnsub first(1, self, self_log);
    Probe second(2, self_log);
    CHECK(self.subscribe(first));
    CHECK(self.subscribe(second));
    self.publish(1);
    CHECK((self_log == Log{{1, 2}, 2}));
    CHECK(self.count() == 1);
    self_log = {};
    self.publish(1);
    CHECK((self_log == Log{{2}, 1}));

    Bus late;
    Log late_log;
    Probe extra(9, late_log);
    Adds adder(4, late, extra, late_log);
    CHECK(late.subscribe(adder));
    late.publish(1);
    CHECK((late_log == Log{{4}, 1}));
    CHECK(late.count() == 2);
    late_log = {};
    late.publish(1);
    CHECK((late_log == Log{{4, 9}, 2}));
    return report();
}
