// P04 - RAII wrapper for an OS handle (about 25 min).
//
// fake_os is a C driver: open_device returns an int (negative means failure),
// close_device releases it. Write UniqueDevice so the handle cannot leak.
//
//   constructor from a name   opens the device
//   destructor                closes it, if one is still owned
//   move constructor / move assignment   transfer ownership; both noexcept
//   copies                    deleted
//   valid()                   true when the handle is non-negative
//   fd()                      the raw handle
//   release()                 give up ownership and return the handle, without closing
//   reset()                   close now, if any
//
// Every successful open is closed exactly once. Move-assigning an object to
// itself must not close the handle. A moved-from object holds no handle.
#include "check.hpp"

#include <type_traits>
#include <utility>

// A fake C driver API. A negative handle means the open failed.
namespace fake_os {
inline int open_calls = 0;
inline int close_calls = 0;
inline int last_closed = -1;

inline int open_device(const char* name) {
    ++open_calls;
    return name[0] == '\0' ? -1 : 100 + open_calls;
}
inline void close_device(int fd) {
    ++close_calls;
    last_closed = fd;
}
inline void reset_counters() {
    open_calls = 0;
    close_calls = 0;
    last_closed = -1;
}
}  // namespace fake_os

// ===== YOUR CODE =====

class UniqueDevice {
public:
    UniqueDevice() = default;
    explicit UniqueDevice(const char* name) 
    {
        this->fd_ = fake_os::open_device(name);
    }

    ~UniqueDevice()
    {
        this->reset();
    }
    UniqueDevice(const UniqueDevice&) = delete;
    UniqueDevice& operator=(const UniqueDevice&) = delete;
    UniqueDevice(UniqueDevice&& other) noexcept
    {
        this->fd_ = other.release();
    }

    UniqueDevice& operator=(UniqueDevice&& other) noexcept
    {
        if(this != &other)
        {
            this->reset();
            this->fd_ = other.release();
        }
        return *this;
    }
    bool valid() const { return this->fd_ >= 0; }
    int fd() const { return fd_; }
    int release() 
    { 
        const int old = this->fd_;
        this->fd_ = -1;
        return old; 
    }
    // Closes the handle now (if any).
    void reset() {
        if(this->valid())
        {
            fake_os::close_device(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

// ===== TESTS (do not edit) =====

int main() {
    CHECK(!std::is_copy_constructible_v<UniqueDevice>);
    CHECK(!std::is_copy_assignable_v<UniqueDevice>);
    CHECK(std::is_nothrow_move_constructible_v<UniqueDevice>);
    CHECK(std::is_nothrow_move_assignable_v<UniqueDevice>);

    fake_os::reset_counters();
    {
        UniqueDevice dev("can0");
        CHECK(dev.valid());
        CHECK(dev.fd() == 101);
        CHECK(fake_os::open_calls == 1);
        CHECK(fake_os::close_calls == 0);
    }
    CHECK(fake_os::close_calls == 1);
    CHECK(fake_os::last_closed == 101);

    fake_os::reset_counters();
    {
        UniqueDevice bad("");
        CHECK(!bad.valid());
    }
    CHECK(fake_os::close_calls == 0);

    fake_os::reset_counters();
    {
        UniqueDevice a("a");
        const int fd = a.fd();
        UniqueDevice b(std::move(a));
        CHECK(!a.valid());
        CHECK(b.valid());
        CHECK(b.fd() == fd);
        CHECK(fake_os::close_calls == 0);
    }
    CHECK(fake_os::close_calls == 1);

    fake_os::reset_counters();
    {
        UniqueDevice a("a");
        UniqueDevice b("b");
        const int fd_a = a.fd();
        const int fd_b = b.fd();
        a = std::move(b);
        CHECK(fake_os::close_calls == 1);
        CHECK(fake_os::last_closed == fd_a);
        CHECK(a.fd() == fd_b);
        CHECK(!b.valid());
    }
    CHECK(fake_os::close_calls == 2);

    fake_os::reset_counters();
    {
        UniqueDevice a("a");
        UniqueDevice& same = a;
        a = std::move(same);
        CHECK(a.valid());
        CHECK(fake_os::close_calls == 0);
    }
    CHECK(fake_os::close_calls == 1);

    fake_os::reset_counters();
    {
        UniqueDevice a("a");
        const int released = a.release();
        CHECK(released == 101);
        CHECK(!a.valid());
    }
    CHECK(fake_os::close_calls == 0);

    fake_os::reset_counters();
    {
        UniqueDevice a("a");
        a.reset();
        CHECK(!a.valid());
        CHECK(fake_os::close_calls == 1);
        a.reset();
        CHECK(fake_os::close_calls == 1);
    }
    CHECK(fake_os::close_calls == 1);

    fake_os::reset_counters();
    {
        UniqueDevice empty;
        CHECK(!empty.valid());
    }
    CHECK(fake_os::close_calls == 0);
    return report();
}
