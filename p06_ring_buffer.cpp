// P06 - Fixed-capacity ring buffer (about 25 min).
//
// RingBuffer<T, N> stores at most N elements and overwrites nothing unless asked.
// It must work for any N (the tests use 3) and for std::string, not only int.
//
//   push(value)            false when full; the value is not stored
//   push_overwrite(value)  if full, drop the oldest element, then store value
//   pop()                  the oldest element, or nullopt when empty
//   size(), empty(), full(), capacity()
//
// Order is FIFO. After filling, popping one, and pushing again, the new element
// is the last one returned by pop. Empty and full must be distinguishable
// without wasting a slot.
#include "check.hpp"
#include <bits/stdc++.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// ===== YOUR CODE =====

template <typename T, std::size_t N>
class RingBuffer {
public:

    RingBuffer()
    {
        this->new_element = storage_.begin();
        this->old_element = storage_.begin();
        count_ = 0;
    }
    ~RingBuffer() = default;
    // False when full.
    bool push(const T& value) 
    {
        if(this->full())
        {
            return false;
        }
        *new_element = value;
        new_element++;
        if(new_element == storage_.end())
        {
            new_element = storage_.begin();
        }
        count_++;    
        return true; 
    }
    // Drops the oldest element when full.
    void push_overwrite(const T& value) 
    {
        if(this->full())
        {
            *old_element = value;
            new_element = old_element;
            old_element++;
            if(old_element == storage_.end())
            {
                old_element = storage_.begin();
            }
        } 
        else
        {
            this->push(value);
        }
    }
    // Oldest element; nullopt when empty.
    std::optional<T> pop()
    {
        if(!this->empty())
        {
            T element = *old_element;
            old_element++;
            if(old_element == storage_.end())
            {
                old_element = storage_.begin();
            }
            count_--;
            return element;
        } 
        return std::nullopt; 
    }
    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return this->size() == this->capacity(); }
    static constexpr std::size_t capacity() { return N; }

private:
    std::size_t count_;
    std::array<T, N>::iterator new_element, old_element;
    std::array<T, N> storage_{};
};

// ===== TESTS (do not edit) =====

int main() {
    RingBuffer<int, 3> rb;
    CHECK(rb.empty());
    CHECK(rb.capacity() == 3);
    CHECK(!rb.pop().has_value());
    CHECK(rb.push(1));
    CHECK(rb.push(2));
    CHECK(rb.push(3));
    CHECK(rb.full());
    CHECK(rb.size() == 3);
    CHECK(!rb.push(4));
    CHECK(rb.pop() == 1);
    CHECK(rb.push(4));
    CHECK(rb.pop() == 2);
    CHECK(rb.pop() == 3);
    CHECK(rb.pop() == 4);
    CHECK(rb.empty());
    CHECK(!rb.pop().has_value());

    RingBuffer<int, 3> ow;
    for (int i = 1; i <= 5; ++i) {
        ow.push_overwrite(i);
    }
    CHECK(ow.size() == 3);
    CHECK(ow.pop() == 3);
    CHECK(ow.pop() == 4);
    CHECK(ow.pop() == 5);
    CHECK(ow.empty());

    RingBuffer<std::uint32_t, 8> wrap;
    bool order_ok = true;
    std::uint32_t next_in = 0;
    std::uint32_t next_out = 0;
    for (int round = 0; round < 1000; ++round) {
        for (int k = 0; k < 5; ++k) {
            order_ok = wrap.push(next_in++) && order_ok;
        }
        for (int k = 0; k < 5; ++k) {
            const auto v = wrap.pop();
            order_ok = v.has_value() && *v == next_out++ && order_ok;
        }
    }
    CHECK(order_ok);
    CHECK(wrap.empty());

    RingBuffer<std::string, 2> strings;
    CHECK(strings.push("brake"));
    CHECK(strings.push("light"));
    CHECK(strings.pop() == std::string("brake"));
    return report();
}
