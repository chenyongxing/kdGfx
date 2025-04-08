#pragma once

#include <vulkan/vulkan.h>

#include "../Fence.h"

namespace kdGfx
{
    class VKDevice;

    class VKFence : public Fence
    {
    public:
        VKFence(VKDevice& device, uint64_t initialValue);
        virtual ~VKFence();

        void signal(uint64_t value) override;
        void wait(uint64_t value) override;
        uint64_t getCompletedValue() override;
        
        inline VkSemaphore getTimelineSemaphore() const { return _timelineSemaphore; }

    private:
        VKDevice& _device;
        VkSemaphore _timelineSemaphore = VK_NULL_HANDLE;
    };
}
