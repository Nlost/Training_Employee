// P05 - Rule of five for a class that owns heap memory (about 25 min).
//
// ByteBuffer owns a new[] array. The constructor is given. Add the five special
// members so copies are independent and nothing leaks. LeakSanitizer reports
// the stub: copies share one allocation, and nothing deletes it.
//
//   destructor
//   copy constructor          deep copy of the bytes
//   copy assignment           deep copy; self-assignment must stay valid
//   move constructor          steal the buffer; noexcept
//   move assignment           steal the buffer; noexcept
//
// A moved-from buffer is empty: data() == nullptr and size() == 0.
// Two copies must not share memory: writing one does not change the other.
#include "check.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

// ===== YOUR CODE =====

class ByteBuffer {
public:
    ByteBuffer(std::size_t size = 0)
        : data_(size > 0 ? new std::uint8_t[size]{} : nullptr), size_(size) {}

    // TODO: destructor, copy constructor, copy assignment, move constructor, move assignment.
    // A moved-from buffer is empty: size() == 0 and data() == nullptr.
    ~ByteBuffer()
    {
        delete[] data_;
        data_ = nullptr;
        this->size_ = 0;
    }
    ByteBuffer(ByteBuffer& other) noexcept
    {
        this->size_ = other.size();
        if(size_ > 0)
        {
            data_ = new std::uint8_t[size_]{};
            for(size_t i=0; i<size(); i++)
            {
                this->data_[i] = other[i];
            }
        }
        else
        {
            data_ = nullptr;
        }
    }

    ByteBuffer& operator=(const ByteBuffer& other)
    {
        this->size_ = other.size();
        if(this == &other){}
        else
        {
            delete[] data_;
            if(size_ > 0)
            {
                data_ = new std::uint8_t[size_]{};
                for(size_t i=0; i <this->size(); i++)
                {
                this->data_[i] = other[i];
                }
            }
            else
            {
                data_ = nullptr;
            }   
        }
        return *this;
    }

    ByteBuffer(ByteBuffer &&other) noexcept
    {
        this->size_ = other.size();
        data_ = other.data();
        other.size_=  0;
        other.data_ = nullptr;
    }

    ByteBuffer& operator=(ByteBuffer&& other) noexcept
    {
        if(this != &other)
        {
            delete[] data_;
            this->size_ = other.size();
            this->data_ = other.data();
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    std::size_t size() const { return size_; }
    std::uint8_t* data() { return data_; }
    const std::uint8_t* data() const { return data_; }
    std::uint8_t& operator[](std::size_t i) { return data_[i]; }
    std::uint8_t operator[](std::size_t i) const { return data_[i]; }

private:
    std::uint8_t* data_;
    std::size_t size_;
};

// ===== TESTS (do not edit) =====

int main() {
    CHECK(std::is_nothrow_move_constructible_v<ByteBuffer>);
    CHECK(std::is_nothrow_move_assignable_v<ByteBuffer>);
    {
        ByteBuffer zeros(3);
        CHECK(zeros.size() == 3);
        CHECK(zeros[0] == 0 && zeros[2] == 0);
    }
    {
        ByteBuffer a(4);
        a[0] = 1;
        ByteBuffer b = a;
        b[0] = 2;
        CHECK(a[0] == 1);
        CHECK(b[0] == 2);
        CHECK(b.size() == 4);
        CHECK(a.data() != b.data());

        ByteBuffer c(2);
        c = a;
        CHECK(c.size() == 4);
        CHECK(c[0] == 1);
        CHECK(c.data() != a.data());

        ByteBuffer& same = a;
        a = same;
        CHECK(a.size() == 4);
        CHECK(a[0] == 1);

        const std::uint8_t* storage = a.data();
        ByteBuffer m = std::move(a);
        CHECK(m.data() == storage);
        CHECK(m.size() == 4);
        CHECK(a.data() == nullptr);
        CHECK(a.size() == 0);

        ByteBuffer n(8);
        n = std::move(m);
        CHECK(n.data() == storage);
        CHECK(m.data() == nullptr);
        CHECK(m.size() == 0);

        ByteBuffer& self = n;
        n = std::move(self);
        CHECK(n.data() == storage);
        CHECK(n.size() == 4);

        ByteBuffer empty;
        ByteBuffer empty_copy = empty;
        CHECK(empty_copy.size() == 0);
        CHECK(empty_copy.data() == nullptr);
    }
    return report();
}
