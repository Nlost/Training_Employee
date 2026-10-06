// P25 - Factory (about 25 min).
//
// Callers ask for a sensor by kind. make_sensor is the only function that names
// the concrete classes; everyone else keeps a unique_ptr<ISensor>.
//
//   SensorKind::Constant   read() always returns a; b is ignored
//   SensorKind::Ramp       read() returns a, then a + b, then a + 2*b, ...
//   SensorKind::Unknown    returns nullptr
//
// RampSensor::destroyed counts destructor calls, including when the sensor is
// deleted through unique_ptr<ISensor>. ISensor has a public virtual destructor
// and is not copyable.
#include "check.hpp"

#include <cstdint>
#include <memory>
#include <type_traits>

// ===== YOUR CODE =====

enum class SensorKind { Constant, Ramp, Unknown };

class ISensor {
public:
    virtual ~ISensor() = default;
    ISensor(const ISensor&) = delete;
    ISensor& operator=(const ISensor&) = delete;

    virtual std::int32_t read() = 0;

protected:
    ISensor() = default;
    std::int32_t a,b;
};

class ConstantSensor : public ISensor {
public:
    explicit ConstantSensor(std::int32_t value) 
    {
        this->a = value;
    }
    std::int32_t read() override { return this->a; }
};

class RampSensor : public ISensor {
private:
    std::int32_t step_;
public:
    RampSensor(std::int32_t start, std::int32_t step) 
    {
        this->a = start;
        this->b = step;
        this->step_ = 0;
    }
    ~RampSensor() override { ++destroyed; }
    std::int32_t read() override 
    { 
        std::int32_t result = this->a + this->step_ * this->b;
        this->step_++;
        return result; 
    }

    static inline int destroyed = 0;
};

// Must not return null for Constant or Ramp: the tests call read() on the result.
std::unique_ptr<ISensor> make_sensor(SensorKind kind, std::int32_t a, std::int32_t b) {
    if(kind == SensorKind::Constant)
    {
        return std::make_unique<ConstantSensor>(a);
    }
    else if(kind == SensorKind::Ramp)
    {
        return std::make_unique<RampSensor>(a, b);
        
    }
    else
    {
        return nullptr;
    }
    
}

// ===== TESTS (do not edit) =====

int main() {
    static_assert(std::has_virtual_destructor_v<ISensor>);
    static_assert(!std::is_copy_constructible_v<ISensor>);
    static_assert(std::is_abstract_v<ISensor>);

    {
        const std::unique_ptr<ISensor> sensor = make_sensor(SensorKind::Constant, 42, 99);
        CHECK(sensor != nullptr);
        CHECK(sensor->read() == 42);
        CHECK(sensor->read() == 42);
    }
    {
        const std::unique_ptr<ISensor> sensor = make_sensor(SensorKind::Ramp, 10, -3);
        CHECK(sensor != nullptr);
        CHECK(sensor->read() == 10);
        CHECK(sensor->read() == 7);
        CHECK(sensor->read() == 4);
    }
    CHECK(make_sensor(SensorKind::Unknown, 1, 2) == nullptr);

    const int destroyed_before = RampSensor::destroyed;
    {
        std::unique_ptr<ISensor> sensor = make_sensor(SensorKind::Ramp, 0, 1);
        CHECK(sensor != nullptr);
        CHECK(sensor->read() == 0);
        CHECK(sensor->read() == 1);
    }
    CHECK(RampSensor::destroyed == destroyed_before + 1);
    return report();
}
