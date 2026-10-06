// P10 - Input debouncer (about 15 min).
//
// Debouncer<N> cleans a noisy digital input. The output changes only after N
// consecutive samples that differ from the current output. A single spike shorter
// than N samples is ignored.
//
//   Debouncer(initial)   starting output; default false
//   update(sample)       take one sample, return the debounced state
//   state()              the current debounced state
//   rising_edge()        true only on the update() that changed false -> true
//   falling_edge()       true only on the update() that changed true -> false
//
// The edge flags are false on the next update, even if the level stays the same.
// A sample that matches the current state restarts the count toward a change.
#include "check.hpp"
#include <algorithm>
#include <array>
#include <initializer_list>

// ===== YOUR CODE =====

template <unsigned N>
class Debouncer {
public:
    explicit Debouncer(bool initial = false) 
    {
        this->state_ = initial;
        typename std::array<bool, N>::iterator it;
        for(it=values_.begin(); it<values_.end(); it++)
        {
            *it = initial;
        }
        if(initial)
        {
            this->countOne_ = N;
            this->countZero_ = 0;
        }
        else
        {
            this->countZero_ = N;
            this->countOne_ = 0;
        }
    }
    // Called once per sample period; returns the debounced state.
    bool update(bool sample) 
    { 
        typename std::array<bool, N>::iterator it = this->values_.begin();
        if(*(values_.begin()))
        {
            countOne_--;
        }
        else
        {
            countZero_--;
        } 
        for(it=values_.begin(); it<values_.end() -1; it++)
        {
            *it = *(it+1);
        }
        *(values_.end() - 1) = sample;
        if(sample)
        {
            countOne_++;
        }
        else
        {
            countZero_++;
        }
        if(countOne_ == N)
        {
            this->prev_ = this->state_;
            this->state_ = true;
        }
        if(countZero_ == N)
        {
            this->prev_ = this->state_;
            this->state_ = false;
        }
        return state_; 
    }
    bool state() const { return this->state_; }
    // True only right after the update() that changed the state false -> true.
    bool rising_edge() const 
    { 
        if(this->state_ && !this->prev_)
        {
            return true;
        } 
        return false;
    }
    // True only right after the update() that changed the state true -> false.
    bool falling_edge() const 
    { 
        if(!this->state_ && this->prev_)
        {
            return true;
        } 
        return false; 
    }

private:
    bool state_, prev_;
    unsigned countOne_, countZero_;
    std::array<bool, N> values_{};
};

// ===== TESTS (do not edit) =====

int main() {
    Debouncer<3> d;
    CHECK(!d.state());
    CHECK(!d.update(true));
    CHECK(!d.update(true));
    CHECK(d.update(true));
    CHECK(d.rising_edge());
    CHECK(d.update(true));
    CHECK(!d.rising_edge());

    for (const bool sample : {false, true, false, true}) {
        CHECK(d.update(sample));
    }
    CHECK(d.update(false));
    CHECK(d.update(false));
    CHECK(!d.update(false));
    CHECK(d.falling_edge());
    CHECK(!d.rising_edge());
    CHECK(!d.update(false));
    CHECK(!d.falling_edge());

    Debouncer<3> glitch;
    for (const bool sample : {true, true, false, true, true}) {
        glitch.update(sample);
    }
    CHECK(!glitch.state());
    CHECK(glitch.update(true));

    Debouncer<1> immediate;
    CHECK(immediate.update(true));
    CHECK(immediate.rising_edge());
    CHECK(!immediate.update(false));
    CHECK(immediate.falling_edge());

    Debouncer<3> starts_high(true);
    CHECK(starts_high.state());
    CHECK(starts_high.update(false));
    CHECK(starts_high.update(false));
    CHECK(!starts_high.update(false));
    return report();
}
