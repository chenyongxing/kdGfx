#pragma once

#include <vulkan/vulkan.h>

#include "../Swapchain.h"
#include "VKCommandQueue.h"
#include "VKTexture.h"

namespace kdGfx
{
    class VKDevice;

    class VKSwapchain : public Swapchain
    {
    public:
        VKSwapchain(VKDevice& device, VKCommandQueue& queue, const SwapchainDesc& desc);
        virtual ~VKSwapchain();

        std::shared_ptr<Texture> getBackBuffer(uint32_t index) override;
        uint32_t nextTexture(const std::shared_ptr<Fence>& fence, uint64_t signalValue) override;
        void present(const std::shared_ptr<Fence>& fence, uint64_t waitValue, uint64_t signalValue) override;

        inline VkSurfaceKHR getSurface() const { return _surface; }
        inline VkSwapchainKHR getSwapchain() const { return _swapchain; }

    private:
        void _getOrCreateSurface();
        void _getSurfaceAttribute();

        VKDevice& _device;
        VKCommandQueue& _queue;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
        VkSurfaceCapabilitiesKHR _surfaceCap = {};
        VkSurfaceFormatKHR _surfaceFormat = {};
        VkPresentModeKHR _presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

        VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
        std::vector<std::shared_ptr<VKTexture>> _colorAttachments;
        uint32_t _frameIndex = 0;
        VkSemaphore _imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore _renderingFinishedSemaphore = VK_NULL_HANDLE;
    };
}
