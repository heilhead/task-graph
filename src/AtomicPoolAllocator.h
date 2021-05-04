#pragma once

#include <vector>
#include <atomic>
#include <cassert>

template<typename T>
class AtomicPoolAllocator {
    static_assert(sizeof(T) >= sizeof(void*), "pool item size is too small");

private:
    std::vector<uint8_t> data;
    std::atomic<void*> next;

public:
    explicit AtomicPoolAllocator(size_t elemCount)
        :data(sizeof(T) * elemCount) {
        constexpr auto align = sizeof(T);

        for (auto i = 0u; i < elemCount; i++) {
            void** ptr = (void**)(data.data() + i * align);
            *ptr = i < elemCount - 1u ? (void*)(data.data() + (i + 1u) * align) : nullptr;
        }

        next = data.data();
    }

    template<typename ...Args>
    T* obtain(Args&& ...args) {
        void** ptr;
        void* n;
        do {
            n = next;

            if (n == nullptr) {
                return nullptr;
            }

            ptr = (void**)n;
        } while (!next.compare_exchange_strong(n, *ptr));

        new(ptr)T(std::forward<Args>(args)...);

        return (T*)ptr;
    }

    void release(T* elem) {
        assert(data.data() <= (void*)elem);
        assert(data.data() + data.size() > (void*)elem);

        elem->~T();

        void* n;
        do {
            n = next;
            *(void**)elem = n;
        } while (!next.compare_exchange_strong(n, elem));
    }
};