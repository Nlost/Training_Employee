// P17 - Virtual interface vs CRTP (about 30 min).
//
// Two ways to read a sensor. Both averages must agree.
//
// Runtime polymorphism
//   ConstantSensor(value)          read() always returns value
//   RampSensor(start, step)        read() returns start, then start+step, then
//                                  start+2*step, ... and counts its destructions
//   average(sensors, samples_each) reads every sensor that many times and returns
//                                  the integer average, truncated toward zero.
//                                  No sensors, or zero samples, returns 0.
//
// Static polymorphism
//   SensorBase<Derived>::read() calls Derived::read_impl() with no virtual call.
//   CrtpConstant and CrtpRamp behave like the two classes above.
//   average_static(samples_each, sensors...) is the same average, written with
//   a fold expression so it accepts any number of sensors.
//   sizeof(CrtpConstant) equals sizeof(int32_t): no vptr.
//
// ISensor, as given, has one bug. Deleting a RampSensor through an ISensor
// pointer never runs ~RampSensor. Find it and fix it. The last test owns a
// RampSensor in a unique_ptr<ISensor> and checks that the destructor ran.
#include "check.hpp"
#include <iostream>
#include <array>
#include <cstdint>
#include <memory>
#include <span>

// ===== YOUR CODE =====

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual std::int32_t read() = 0;
};

class ConstantSensor : public ISensor {
public:
    explicit ConstantSensor(std::int32_t value) : value_(value) {}
    std::int32_t read() override { return value_; }

private:
    std::int32_t value_;
};

// Returns start, start + step, start + 2 * step, ...
class RampSensor : public ISensor {
public:
    RampSensor(std::int32_t start, std::int32_t step) : start_(start), step_(step), count_(0) {}
    ~RampSensor() { ++destroyed; }
    std::int32_t read() override 
    { 
        return start_ + (count_++) * step_; 
    }

    static inline int destroyed = 0;

private:
    std::int32_t start_;
    std::int32_t step_;
    std::int32_t count_;
};

// Reads every sensor samples_each times; returns the integer average (truncated), 0 if no samples.
std::int32_t average(std::span<ISensor* const> sensors, int samples_each) 
{ 
    if(sensors.empty() || samples_each == 0)
    {
        return 0;    
    }
    std::int64_t sum = 0;
    std::int32_t count = 0;
    for(int i = 0; i< samples_each; i++)
    {
        for(auto sensor : sensors)
        {
            sum += static_cast<int64_t>(sensor->read());
            count++;
        }
    }
    std::int32_t result = static_cast<int32_t>(sum / static_cast<int64_t>(count));
    return result;
}

// Static polymorphism: SensorBase::read() calls Derived::read_impl() without a virtual call.
template <typename Derived>
class SensorBase {
public:
    std::int32_t read() { return static_cast<Derived&>(*this).read_impl(); }
};

class CrtpConstant : public SensorBase<CrtpConstant> {
public:
    explicit CrtpConstant(std::int32_t value) : value_(value) {}
    std::int32_t read_impl() { return value_; }

private:
    std::int32_t value_;
};

class CrtpRamp : public SensorBase<CrtpRamp> {
public:
    CrtpRamp(std::int32_t start, std::int32_t step) : start_(start), step_(step) {}
    std::int32_t read_impl() { return start_ + (count_++) * step_; }

private:
    std::int32_t start_;
    std::int32_t step_;
    std::int32_t count_ = 0;
};

// Same result as average(), for any number of CRTP sensors (use a fold expression).
template <typename... Sensors>
std::int32_t average_static(int samples_each, Sensors&... sensors) {
    constexpr std::int64_t n_sensors = sizeof...(Sensors);
    if (n_sensors == 0 || samples_each <= 0) {
        return 0;
    }
    std::int64_t sum = 0;
    for (int i = 0; i < samples_each; ++i) {
        // Fold over the comma-free sum: each sensor is read once per sample.
        sum += (std::int64_t{0} + ... + static_cast<std::int64_t>(sensors.read()));
    }
    return static_cast<std::int32_t>(sum / (n_sensors * samples_each));
}

// ===== TESTS (do not edit) =====

int main() {
    {
        ConstantSensor ten(10);
        RampSensor ramp(0, 10);
        const std::array<ISensor*, 2> sensors{&ten, &ramp};
        CHECK(average(sensors, 4) == 12);
        ConstantSensor negative(-7);
        const std::array<ISensor*, 1> one{&negative};
        CHECK(average(one, 3) == -7);
        CHECK(average(std::span<ISensor* const>{}, 5) == 0);
    }
    {
        CrtpConstant ten(10);
        CrtpRamp ramp(0, 10);
        CHECK(average_static(4, ten, ramp) == 12);
        CrtpConstant negative(-7);
        CHECK(average_static(3, negative) == -7);
    }
    CHECK(sizeof(CrtpConstant) == sizeof(std::int32_t));
    CHECK(sizeof(ConstantSensor) > sizeof(CrtpConstant));

    const int destroyed_before = RampSensor::destroyed;
    {
        std::unique_ptr<ISensor> owned = std::make_unique<RampSensor>(0, 1);
        CHECK(owned->read() == 0);
        CHECK(owned->read() == 1);
    }
    CHECK(RampSensor::destroyed == destroyed_before + 1);
    return report();
}
