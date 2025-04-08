#pragma once

#include "../PCH.h"
#include "Fence.h"
#include "CommandList.h"

namespace kdGfx
{
    // 执行命令的队列
    class CommandQueue
    {
    public:
        virtual ~CommandQueue() = default;

        virtual void signal(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
        virtual void wait(const std::shared_ptr<Fence>& fence, uint64_t value) = 0;
        virtual void waitIdle() = 0;
        virtual void submit(const std::vector<std::shared_ptr<CommandList>>& commandLists) = 0;
    };
}
