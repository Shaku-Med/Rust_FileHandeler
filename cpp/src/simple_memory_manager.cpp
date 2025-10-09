#include <vector>
#include <memory>
#include <cstdint>
#include <mutex>
#include <cstdlib>

class SimpleMemoryManager {
public:
    static SimpleMemoryManager& getInstance() {
        static SimpleMemoryManager instance;
        return instance;
    }
    
    void* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (maxMemory_ > 0 && totalAllocated_ + size > maxMemory_) {
            return nullptr;
        }
        
        void* ptr = std::malloc(size);
        if (ptr) {
            totalAllocated_ += size;
            peakAllocated_ = std::max(peakAllocated_, totalAllocated_);
        }
        
        return ptr;
    }
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        std::free(ptr);
    }
    
    template<typename T>
    T* allocateArray(size_t count) {
        return static_cast<T*>(allocate(sizeof(T) * count));
    }
    
    template<typename T>
    void deallocateArray(T* ptr) {
        deallocate(static_cast<void*>(ptr));
    }
    
    size_t getTotalAllocated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return totalAllocated_;
    }
    
    size_t getPeakAllocated() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return peakAllocated_;
    }
    
    void resetStats() {
        std::lock_guard<std::mutex> lock(mutex_);
        totalAllocated_ = 0;
        peakAllocated_ = 0;
    }
    
    void setMaxMemory(size_t maxMemory) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxMemory_ = maxMemory;
    }
    
    size_t getMaxMemory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxMemory_;
    }
    
    bool isMemoryAvailable(size_t size) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return maxMemory_ == 0 || totalAllocated_ + size <= maxMemory_;
    }

private:
    SimpleMemoryManager() = default;
    ~SimpleMemoryManager() = default;
    SimpleMemoryManager(const SimpleMemoryManager&) = delete;
    SimpleMemoryManager& operator=(const SimpleMemoryManager&) = delete;
    
    mutable std::mutex mutex_;
    size_t totalAllocated_ = 0;
    size_t peakAllocated_ = 0;
    size_t maxMemory_ = 0;
};