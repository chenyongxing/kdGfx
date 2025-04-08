#pragma once

#include <wrl.h>
#include <dxgi1_4.h>
#include <directx/d3d12.h>

#include "../Swapchain.h"
#include "DXCommandQueue.h"
#include "DXTexture.h"

namespace kdGfx
{
    class DXDevice;

    class DXSwapchain : public Swapchain
    {
    public:
        DXSwapchain(DXDevice& device, DXCommandQueue& queue, const SwapchainDesc& desc);

        std::shared_ptr<Texture> getBackBuffer(uint32_t index) override;
        uint32_t nextTexture(const std::shared_ptr<Fence>& fence, uint64_t signalValue) override;
        void present(const std::shared_ptr<Fence>& fence, uint64_t waitValue, uint64_t signalValue) override;
        void destroy();

        inline const Microsoft::WRL::ComPtr<IDXGISwapChain3>& getSwapchain() const { return _swapChain; }

    private:
        DXDevice& _device;
        DXCommandQueue& _queue;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
        std::vector<std::shared_ptr<DXTexture>> _colorAttachments;
    };
}
