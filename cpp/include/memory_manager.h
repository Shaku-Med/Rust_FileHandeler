#pragma once

#include <vector>
#include <memory>
#include <cstdint>

class MemoryManager {
public:
    static MemoryManager& getInstance();
    
    void* allocate(size_t size);
    void deallocate(void* ptr);
    
    template<typename T>
    T* allocateArray(size_t count) {
        return static_cast<T*>(allocate(sizeof(T) * count));
    }
    
    template<typename T>
    void deallocateArray(T* ptr) {
        deallocate(static_cast<void*>(ptr));
    }
    
    size_t getTotalAllocated() const;
    size_t getPeakAllocated() const;
    void resetStats();
    
    void setMaxMemory(size_t maxMemory);
    size_t getMaxMemory() const;
    
    bool isMemoryAvailable(size_t size) const;

private:
    MemoryManager() = default;
    ~MemoryManager() = default;
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    
    size_t totalAllocated = 0;
    size_t peakAllocated = 0;
    size_t maxMemory = 0;
};