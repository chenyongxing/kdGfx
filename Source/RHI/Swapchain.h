#pragma once

#include "BaseTypes.h"
#include "Texture.h"
#include "Fence.h"

namespace kdGfx
{
    class Swapchain
    {
    public:
        virtual ~Swapchain() = default;

        virtual std::shared_ptr<Texture> getBackBuffer(uint32_t index) = 0;
        // 返回的frame index超出数量，需要重创交换链
        virtual uint32_t nextTexture(const std::shared_ptr<Fence>& fence, uint64_t signalValue) = 0;
        virtual void present(const std::shared_ptr<Fence>& fence, uint64_t waitValue, uint64_t signalValue) = 0;

        inline Format getFormat() const { return _desc.format; }
        inline uint32_t getWidth() const { return _desc.width; }
        inline uint32_t getHeight() const { return _desc.height; }
        inline uint32_t getFrameCount() const { return _desc.frameCount; }

    protected:
        SwapchainDesc _desc;
    };
}
