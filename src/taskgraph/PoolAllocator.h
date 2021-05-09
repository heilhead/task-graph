#pragma once

#include <vector>
#include <atomic>
#include <cassert>
#include <array>

template<typename T>
struct PoolItem {
public:
    void* next = nullptr;
    std::atomic<uint64_t> version = 0u;

private:
    std::array<uint8_t, sizeof(T)> itemData;

public:
    T* data() {
        return (T*)itemData.data();
    }

    static PoolItem<T>* fromData(const T* data) {
        if (data == nullptr) {
            return nullptr;
        } else {
            // N.B. This code assumes that `itemData` is the last property.
            static constexpr size_t offset = sizeof(PoolItem<T>) - sizeof(T);
            return (PoolItem<T>*)((size_t)data - offset);
        }
    }
};

template<typename T>
struct PoolItemHandle {
private:
    uint64_t version;
    T* dataPtr;

public:
    explicit PoolItemHandle(PoolItem<T>* item = nullptr) {
        if (item != nullptr) {
            dataPtr = item->data();
            version = item->version;
        } else {
            dataPtr = nullptr;
            version = 0;
        }
    }

    explicit PoolItemHandle(T* inDataPtr) {
        auto* item = PoolItem<T>::fromData(inDataPtr);
        dataPtr = inDataPtr;
        version = item->version;
    }

    bool valid() {
        if (dataPtr == nullptr) {
            return false;
        } else {
            return item()->version == version;
        }
    }

    T* data() {
        return dataPtr;
    }

    PoolItem<T>* item() {
        return PoolItem<T>::fromData(dataPtr);
    }

    explicit operator bool() {
        return valid();
    }

    T* operator*() {
        return dataPtr;
    }

    T* operator->() {
        return dataPtr;
    }
};

template<typename T>
class PoolAllocator {
private:
    std::vector<PoolItem<T>> items;
    std::atomic<PoolItem<T>*> next;
    size_t currSize = 0;
    size_t maxCapacity;

public:
    explicit PoolAllocator(size_t inSize)
        :items { inSize }, maxCapacity { inSize } {
        assert(inSize > 0);

        for (auto i = 0u; i < inSize - 1; i++) {
            items[i].next = &items[i + 1];
            currSize++;
        }

        currSize++;
        next = &items[0];
    }

    template<typename ...Args>
    [[nodiscard]]
    PoolItem<T>* obtain(Args&& ...args) {
        PoolItem<T>* item;
        do {
            item = next;
            if (item == nullptr) {
                return nullptr;
            }
        } while (!next.compare_exchange_strong(item, (PoolItem<T>*)item->next));
        currSize--;

        item->next = this;
        new(item->data())T(std::forward<Args>(args)...);

        return item;
    }

    void release(PoolItemHandle<T>& handle) {
        assert(handle.valid());
        assert((void*)&items.front() <= handle.data());
        assert((void*)(&items.back() + sizeof(T)) >= handle.data());

        release(handle.item());
    }

    void release(PoolItem<T>* item) {
        item->data()->~T();
        item->version++;

        PoolItem<T>* oldNext;
        do {
            oldNext = next;
            item->next = oldNext;
        } while (!next.compare_exchange_strong(oldNext, item));

        currSize++;
    }

    [[nodiscard]] size_t size() const {
        return currSize;
    }

    [[nodiscard]] size_t capacity() const {
        return maxCapacity;
    }

    static PoolAllocator<T>* fromItem(PoolItem<T>* item) {
        if (item == nullptr) {
            return nullptr;
        } else {
            return (PoolAllocator<T>*)item->next;
        }
    }
};