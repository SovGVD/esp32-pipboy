#pragma once
#include <atomic>
#include <cstdlib>
constexpr unsigned MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2;
namespace host { inline std::atomic<bool> failPsram{false}; }
inline void* heap_caps_calloc(size_t count, size_t size, unsigned) {
    return host::failPsram ? nullptr : std::calloc(count, size);
}
inline void heap_caps_free(void* ptr) { std::free(ptr); }
