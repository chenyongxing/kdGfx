#pragma once

#include "Memory.h"

namespace kdGfx
{
    class Buffer
    {
    public:
        // 常量buffer注意要对齐
        virtual ~Buffer() = default;

        virtual void* map() = 0;
        virtual void unmap() = 0;

        inline const BufferDesc& getDesc() const { return _desc; }
        inline size_t getSize() const { return _desc.size; }
        // 是否在外部分配的显存
        inline bool isPlaced() const { return !_memory.expired(); }
        inline std::shared_ptr<Memory> getMemory() const { return _memory.lock(); }

    protected:
        BufferDesc _desc;
        std::weak_ptr<Memory> _memory;
        size_t _memoryOffset = 0;
    };
}
