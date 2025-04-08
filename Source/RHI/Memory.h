#pragma once

#include "BaseTypes.h"

namespace kdGfx
{
    class Memory
    {
    public:
        virtual ~Memory() = default;

        inline const MemoryDesc& getDesc() const { return _desc; }

    protected:
        MemoryDesc _desc;
    };
}
