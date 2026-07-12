#include <new>
#include <stddef.h>
#include <stdlib.h>

#ifdef ARDUINO

// The Nano runtime is intentionally heapless. These replacements prevent the
// Arduino heap allocator from being linked and turn an accidental allocation
// into a deterministic failure instead of hidden SRAM use or fragmentation.
//
// Ordinary new cannot report failure to its caller, so it aborts immediately.
// Nothrow new follows its contract and returns nullptr. Placement-new is a
// separate standard overload and remains available for preallocated storage.
void* operator new(size_t) noexcept {
    abort();
}

void* operator new[](size_t) noexcept {
    abort();
}

void* operator new(size_t, const std::nothrow_t&) noexcept {
    return nullptr;
}

void* operator new[](size_t, const std::nothrow_t&) noexcept {
    return nullptr;
}

// No heap allocation can succeed, so there is no storage to release.
void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*, const std::nothrow_t&) noexcept {}
void operator delete[](void*, const std::nothrow_t&) noexcept {}

#if __cplusplus >= 201402L
void operator delete(void*, size_t) noexcept {}
void operator delete[](void*, size_t) noexcept {}
#endif

#endif
