#pragma once

#include <cstdio>

namespace check_detail {
inline int checks = 0;
inline int failures = 0;
}  // namespace check_detail

// Call only from the main thread.
#define CHECK(expr)                                                  \
    do {                                                             \
        ++check_detail::checks;                                      \
        if (!(expr)) {                                               \
            ++check_detail::failures;                                \
            std::printf("  FAIL line %d: %s\n", __LINE__, #expr);    \
        }                                                            \
    } while (false)

// Flushes, because a sanitizer that fails at exit ends the process before stdout is flushed.
inline int report() {
    const int status = check_detail::failures == 0 ? 0 : 1;
    if (status == 0) {
        std::printf("PASS: all %d checks passed\n", check_detail::checks);
    } else {
        std::printf("FAILED: %d of %d checks\n", check_detail::failures, check_detail::checks);
    }
    std::fflush(stdout);
    return status;
}
