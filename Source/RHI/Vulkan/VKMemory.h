#pragma once

#include "../Memory.h"
#include "VKDevice.h"

namespace kdGfx
{
    class VKMemory : public Memory
    {
    public:
        VKMemory(VKDevice& device, const MemoryDesc& desc);
        virtual ~VKMemory();

        inline VkDeviceMemory getMemory() const { return _memory; }

    private:
        VKDevice& _device;
        VkDeviceMemory _memory = VK_NULL_HANDLE;
    };
}
