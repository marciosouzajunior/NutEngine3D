#include <new>
#include <stddef.h>
#include <stdint.h>

#ifdef ARDUINO

namespace {

// Tiny bump allocator used only by the Nano firmware target.
//
// Why this exists:
// - AVR heap behavior is hard to reason about under tight RAM pressure.
// - We only need a small amount of dynamic allocation during startup/runtime.
// - A fixed arena gives us deterministic failure instead of heap fragmentation.
//
// Important limitation:
// - this allocator never frees individual allocations
// - delete/delete[] are intentionally no-ops
// - memory is consumed monotonically until the board resets
//
// So this is only safe because the Nano runtime allocates a very small, bounded
// set of objects and then keeps running with that state.
constexpr size_t kNanoArenaSize = 768;

alignas(max_align_t) static uint8_t g_nanoArena[kNanoArenaSize];
static size_t g_nanoArenaOffset = 0;

void* nanoAllocate(size_t size) noexcept {
    if (size == 0) {
        size = 1;
    }

    const size_t alignment = alignof(max_align_t);
    const size_t alignedOffset = (g_nanoArenaOffset + (alignment - 1)) & ~(alignment - 1);
    if (alignedOffset + size > kNanoArenaSize) {
        return nullptr;
    }

    void* result = g_nanoArena + alignedOffset;
    g_nanoArenaOffset = alignedOffset + size;
    return result;
}

} // namespace

// Replace global new/delete for the Nano target so any remaining dynamic
// allocations go through the fixed arena above instead of the AVR heap.
void* operator new(size_t size) noexcept {
    return nanoAllocate(size);
}

void* operator new[](size_t size) noexcept {
    return nanoAllocate(size);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    return nanoAllocate(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    return nanoAllocate(size);
}

void operator delete(void*) noexcept {}
void operator delete[](void*) noexcept {}
void operator delete(void*, const std::nothrow_t&) noexcept {}
void operator delete[](void*, const std::nothrow_t&) noexcept {}

#if __cplusplus >= 201402L
void operator delete(void*, size_t) noexcept {}
void operator delete[](void*, size_t) noexcept {}
#endif

#endif
