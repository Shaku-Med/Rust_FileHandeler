#include "memory_manager.h"
#include <algorithm>
#include <mutex>

MemoryManager& MemoryManager::getInstance() {
    static MemoryManager instance;
    return instance;
}

void* MemoryManager::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (maxMemory > 0 && totalAllocated + size > maxMemory) {
        return nullptr;
    }
    
    void* ptr = std::malloc(size);
    if (ptr) {
        totalAllocated += size;
        peakAllocated = std::max(peakAllocated, totalAllocated);
    }
    
    return ptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    std::free(ptr);
}

size_t MemoryManager::getTotalAllocated() const {
    std::lock_guard<std::mutex> lock(mutex);
    return totalAllocated;
}

size_t MemoryManager::getPeakAllocated() const {
    std::lock_guard<std::mutex> lock(mutex);
    return peakAllocated;
}

void MemoryManager::resetStats() {
    std::lock_guard<std::mutex> lock(mutex);
    totalAllocated = 0;
    peakAllocated = 0;
}

void MemoryManager::setMaxMemory(size_t maxMemory) {
    std::lock_guard<std::mutex> lock(mutex);
    this->maxMemory = maxMemory;
}

size_t MemoryManager::getMaxMemory() const {
    std::lock_guard<std::mutex> lock(mutex);
    return maxMemory;
}

bool MemoryManager::isMemoryAvailable(size_t size) const {
    std::lock_guard<std::mutex> lock(mutex);
    return maxMemory == 0 || totalAllocated + size <= maxMemory;
}