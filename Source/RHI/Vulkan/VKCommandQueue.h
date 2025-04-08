#pragma once

#include <vulkan/vulkan.h>

#include "../BaseTypes.h"
#include "../CommandQueue.h"

namespace kdGfx
{
    class VKDevice;

    class VKCommandQueue : public CommandQueue
    {
    public:
        VKCommandQueue(VKDevice& device, uint32_t queueFamilyIndex);

        void signal(const std::shared_ptr<Fence>& fence, uint64_t value) override;
        void wait(const std::shared_ptr<Fence>& fence, uint64_t value) override;
        void waitIdle() override;
        void submit(const std::vector<std::shared_ptr<CommandList>>& commandLists) override;
        
        inline VkQueue getQueue() const { return _queue; }

    private:
        VKDevice& _device;
        uint32_t _queueFamilyIndex;
        VkQueue _queue = VK_NULL_HANDLE;
    };
}
