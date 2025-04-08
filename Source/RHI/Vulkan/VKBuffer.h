#pragma once

#include <vulkan/vulkan.h>

#include "../Buffer.h"
#include "VKDevice.h"
#include "VKMemory.h"

namespace kdGfx
{
    class VKBuffer : public Buffer
    {
    public:
        VKBuffer(VKDevice& device, const BufferDesc& desc);
        VKBuffer(VKDevice& device, const BufferDesc& desc, const std::shared_ptr<VKMemory>& memory, size_t offset);
        virtual ~VKBuffer();

        void* map() override;
        void unmap() override;

        inline VkBuffer getBuffer() const { return _buffer; }

    private:
        VKDevice& _device;
        VkDeviceMemory _bufferMemory = VK_NULL_HANDLE;
        VkBuffer _buffer = VK_NULL_HANDLE;
        void* _mappedPtr = nullptr;
    };
}
