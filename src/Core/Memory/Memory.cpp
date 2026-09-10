#include "Memory.h"
#include "Debug/DebugLogger.h"

#include <new>

namespace Lindo::Core {

    Memory& Memory::Get() {
        static Memory instance;
        return instance;
    }

    void* Memory::allocate(std::size_t size, std::size_t alignment, const char* tag) {
        if (size == 0) return nullptr;

        void* pointer = nullptr;
        if (alignment > alignof(std::max_align_t)) {
            pointer = ::operator new(size, std::align_val_t(alignment));
        }
        else {
            pointer = ::operator new(size);
        }

        {
            std::lock_guard lock(m_mutex);
            m_allocations.emplace(pointer, Allocation{
                size,
                alignment,
                tag ? tag : "unnamed"
            });
            ++m_stats.liveAllocations;
            m_stats.liveBytes += size;
            ++m_stats.totalAllocations;
            m_stats.totalBytes += size;
            if (m_stats.liveBytes > m_stats.peakBytes) {
                m_stats.peakBytes = m_stats.liveBytes;
            }
        }

        return pointer;
    }

    void Memory::deallocate(void* pointer, std::size_t size, std::size_t alignment) noexcept {
        if (!pointer) return;

        {
            std::lock_guard lock(m_mutex);
            auto it = m_allocations.find(pointer);
            if (it != m_allocations.end()) {
                m_stats.liveBytes -= it->second.size;
                --m_stats.liveAllocations;
                m_allocations.erase(it);
            }
            else {
                LOG_WARN("[Memory] Attempted to release an unknown allocation.");
            }
        }

        if (alignment > alignof(std::max_align_t)) {
            ::operator delete(pointer, size, std::align_val_t(alignment));
        }
        else {
            ::operator delete(pointer);
        }
    }

    MemoryStats Memory::getStats() const {
        std::lock_guard lock(m_mutex);
        return m_stats;
    }

    void Memory::report(const std::string& owner) const {
        const MemoryStats stats = getStats();
        LOG_INFO("[Memory] " + owner + ": live=" + std::to_string(stats.liveBytes) +
            " bytes in " + std::to_string(stats.liveAllocations) +
            " allocations, peak=" + std::to_string(stats.peakBytes) + " bytes.");

        if (stats.liveAllocations > 0) {
            std::lock_guard lock(m_mutex);
            for (const auto& [pointer, allocation] : m_allocations) {
                LOG_WARN("[Memory] Live allocation: " + std::to_string(allocation.size) +
                    " bytes, tag='" + allocation.tag + "'.");
            }
        }
    }

    bool Memory::hasLeaks() const {
        std::lock_guard lock(m_mutex);
        return !m_allocations.empty();
    }
}
