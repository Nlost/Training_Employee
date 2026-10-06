#!/usr/bin/env bash
# Usage: ./run.sh <problem number> [asan|tsan|none]
#   ./run.sh 1       build p01 with AddressSanitizer + UBSan and run its tests
#   ./run.sh 19      problems 18-21 use ThreadSanitizer by default
set -uo pipefail
cd "$(dirname "$0")"

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <problem number> [asan|tsan|none]"
    exit 2
fi

num=$(printf '%02d' "$((10#$1))")
src=$(ls p"${num}"_*.cpp 2>/dev/null | head -n1)
if [[ -z "$src" ]]; then
    echo "No file p${num}_*.cpp"
    exit 2
fi

mode=${2:-}
if [[ -z "$mode" ]]; then
    if (( 10#$num >= 18 && 10#$num <= 21 )); then mode=tsan; else mode=asan; fi
fi
case "$mode" in
    asan) sanitizers=(-fsanitize=address,undefined -fno-omit-frame-pointer) ;;
    tsan) sanitizers=(-fsanitize=thread) ;;
    none) sanitizers=() ;;
    *) echo "Mode must be asan, tsan or none"; exit 2 ;;
esac

mkdir -p build
g++ -std=c++20 -g -O1 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow \
    "${sanitizers[@]}" -pthread "$src" -o "build/p${num}" || exit 1

launcher=()
if [[ "$mode" == tsan ]]; then
    # Recent kernels randomize addresses more than ThreadSanitizer supports
    # ("FATAL: ThreadSanitizer: unexpected memory mapping"), so run without ASLR.
    launcher=(setarch "$(uname -m)" -R)
fi

export ASAN_OPTIONS=${ASAN_OPTIONS:-exitcode=86}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-halt_on_error=1:exitcode=86:print_stacktrace=1}

echo "--- running ${src} (${mode}) ---"
timeout 20 "${launcher[@]}" "./build/p${num}"
status=$?
if (( status == 124 )); then
    echo "TIMEOUT: still running after 20 s (deadlock or endless loop?)"
elif [[ "$mode" == tsan && $status -eq 66 ]]; then
    echo "NOT DONE: ThreadSanitizer reported a data race (see WARNING above), even if the checks passed"
elif [[ "$mode" == asan && $status -ne 0 && $status -ne 1 ]]; then
    echo "NOT DONE: a sanitizer reported an error (see ERROR above)"
fi
exit $status
