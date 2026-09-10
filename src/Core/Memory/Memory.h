#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>

namespace Lindo::Core {

    struct MemoryStats {
        std::size_t liveAllocations = 0;
        std::size_t liveBytes = 0;
        std::size_t totalAllocations = 0;
        std::size_t totalBytes = 0;
        std::size_t peakBytes = 0;
    };

    class Memory {
    public:
        template <typename T>
        class ArrayDeleter {
        public:
            ArrayDeleter() = default;
            ArrayDeleter(Memory* owner, std::size_t count) : m_owner(owner), m_count(count) {}

            void operator()(T* value) const noexcept {
                if (!value) return;
                for (std::size_t i = m_count; i > 0; --i) {
                    value[i - 1].~T();
                }
                m_owner->deallocate(value, sizeof(T) * m_count, alignof(T));
            }

        private:
            Memory* m_owner = nullptr;
            std::size_t m_count = 0;
        };

        template <typename T>
        using ArrayPtr = std::unique_ptr<T[], ArrayDeleter<T>>;

        static Memory& Get();

        void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t), const char* tag = nullptr);
        void deallocate(void* pointer, std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept;

        template <typename T>
        ArrayPtr<T> makeArray(std::size_t count, const char* tag = nullptr) {
            if (count == 0) return ArrayPtr<T>(nullptr, ArrayDeleter<T>(this, 0));

            auto* memory = static_cast<T*>(allocate(sizeof(T) * count, alignof(T), tag));
            std::size_t constructed = 0;
            try {
                for (; constructed < count; ++constructed) {
                    new (memory + constructed) T();
                }
            }
            catch (...) {
                for (std::size_t i = constructed; i > 0; --i) {
                    memory[i - 1].~T();
                }
                deallocate(memory, sizeof(T) * count, alignof(T));
                throw;
            }

            return ArrayPtr<T>(memory, ArrayDeleter<T>(this, count));
        }

        MemoryStats getStats() const;
        void report(const std::string& owner) const;
        bool hasLeaks() const;

    private:
        struct Allocation {
            std::size_t size = 0;
            std::size_t alignment = alignof(std::max_align_t);
            std::string tag;
        };

        Memory() = default;
        ~Memory() = default;
        Memory(const Memory&) = delete;
        Memory& operator=(const Memory&) = delete;

        mutable std::mutex m_mutex;
        std::unordered_map<void*, Allocation> m_allocations;
        MemoryStats m_stats;
    };
}
