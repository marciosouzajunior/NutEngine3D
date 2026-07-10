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

} // namespace nut
