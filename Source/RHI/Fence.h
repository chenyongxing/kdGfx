#pragma once

#include "BaseTypes.h"

namespace kdGfx
{
    // 整型值控制同步
    class Fence
    {
    public:
        virtual ~Fence() = default;

        virtual void signal(uint64_t value) = 0;
        virtual void wait(uint64_t value) = 0;
        virtual uint64_t getCompletedValue() = 0;
    };
}
