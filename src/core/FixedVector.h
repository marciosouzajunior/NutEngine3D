#pragma once

#include <stddef.h>

namespace nut {

template <typename T, size_t Capacity>
class FixedVector {
private:
    T m_items[Capacity];
    size_t m_size;

public:
    FixedVector() : m_items(), m_size(0) {}

    bool push_back(const T& value) {
        if (m_size >= Capacity) {
            return false;
        }

        m_items[m_size++] = value;
        return true;
    }

    void clear() {
        m_size = 0;
    }

    void reserve(size_t) {}

    size_t size() const {
        return m_size;
    }

    bool empty() const {
        return m_size == 0;
    }

    T& operator[](size_t index) {
        return m_items[index];
    }

    const T& operator[](size_t index) const {
        return m_items[index];
    }

    T* begin() {
        return m_items;
    }

    const T* begin() const {
        return m_items;
    }

    T* end() {
        return m_items + m_size;
    }

    const T* end() const {
        return m_items + m_size;
    }
};

// A scene with no hierarchy should not reserve a fake child-pointer array in
// every GameObject. Empty C++ arrays are not portable, so capacity zero needs
// an explicit representation with no items and no size counter.
template <typename T>
class FixedVector<T, 0> {
public:
    bool push_back(const T&) {
        return false;
    }

    void clear() {}
    void reserve(size_t) {}

    size_t size() const {
        return 0;
    }

    bool empty() const {
        return true;
    }

    // These operators only keep the interface compatible with FixedVector.
    // They must never execute: every valid caller checks size(), which is zero.
    T& operator[](size_t) {
        return *static_cast<T*>(nullptr);
    }

    const T& operator[](size_t) const {
        return *static_cast<const T*>(nullptr);
    }

    T* begin() {
        return nullptr;
    }

    const T* begin() const {
        return nullptr;
    }

    T* end() {
        return nullptr;
    }

    const T* end() const {
        return nullptr;
    }
};

} // namespace nut
