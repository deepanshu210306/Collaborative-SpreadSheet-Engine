#pragma once
#include <vector>
#include <cstddef>

template <typename T>
class MemoryPool {
public:
    explicit MemoryPool(std::size_t initial = 64) : active_(0) {
        for (std::size_t i = 0; i < initial; ++i) {
            pool_.push_back(new T());
        }
    }

    ~MemoryPool() {
        for (std::size_t i = 0; i < pool_.size(); ++i) {
            delete pool_[i];
        }
        pool_.clear();
    }

    T* acquire() {
        ++active_;
        if (!pool_.empty()) {
            T* obj = pool_.back();
            pool_.pop_back();
            return obj;
        }
        return new T();
    }

    void release(T* obj) {
        if (!obj) return;
        --active_;
        pool_.push_back(obj);
    }

    std::size_t pooled() const { return pool_.size(); }
    std::size_t active() const { return active_; }

private:
    std::vector<T*> pool_;
    std::size_t active_;
};
