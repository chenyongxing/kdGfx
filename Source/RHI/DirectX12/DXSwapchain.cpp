#include "DXSwapchain.h"
#include "DXDevice.h"
#include "DXFence.h"

#include <Windows.h>

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXSwapchain::DXSwapchain(DXDevice& device, DXCommandQueue& queue, const SwapchainDesc& desc) :
		_device(device),
		_queue(queue)
	{
		_desc = desc;

        DXGI_FORMAT requestFormat = DXGI_FORMAT_UNKNOWN;
        DXGI_FORMAT requestFormats[] =
        {
            _device.toDxgiFormat(_desc.format),
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM
        };
        for (DXGI_FORMAT format : requestFormats)
        {
            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { format };
            if (SUCCEEDED(device.getDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))) &&
                (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET))
            {
                requestFormat = format;
                break;
            }
        }
        if (requestFormat == DXGI_FORMAT_UNKNOWN)
        {
            RuntimeError("failed to find dxgi swapchain available format");
        }
        _desc.format = _device.fromDxgiFormat(requestFormat);

        const DXInstance& instance = _device.getAdapter().getInstance();
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
        {
            .Width = _desc.width,
            .Height = _desc.height,
            .Format = requestFormat,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = _desc.frameCount,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        };
        ComPtr<IDXGISwapChain1> _swapChain1;
        // 需要先释放老的交换链。出错注意是不是由于引用计数机制导致Release不成功
        if (!desc.oldSwapchain.expired())
        {
            auto dxSwapchain = std::dynamic_pointer_cast<DXSwapchain>(desc.oldSwapchain.lock());
            dxSwapchain->destroy();
        }
        if (FAILED(instance.getFactory()->CreateSwapChainForHwnd(_queue.getQueue().Get(), 
            reinterpret_cast<HWND>(_desc.window), &swapChainDesc, nullptr, nullptr, &_swapChain1)))
        {
            RuntimeError("failed to create dxgi swapchain");
        }

        instance.getFactory()->MakeWindowAssociation(reinterpret_cast<HWND>(_desc.window), DXGI_MWA_NO_WINDOW_CHANGES);
        
        _swapChain1.As(&_swapChain);

        _colorAttachments.resize(_desc.frameCount);
        for (uint32_t i = 0; i < _desc.frameCount; i++)
        {
            ComPtr<ID3D12Resource> resource;
            _swapChain->GetBuffer(i, IID_PPV_ARGS(&resource));
            TextureDesc textureDesc =
            {
                .usage = TextureUsage::ColorAttachment | TextureUsage::CopySrc | TextureUsage::CopyDst,
                .format = _desc.format,
                .width = _desc.width,
                .height = _desc.height,
                .name = std::string("backBuffer").append(std::to_string(i))
            };
            _colorAttachments[i] = std::make_shared<DXTexture>(_device, resource, textureDesc);
        }
	}

    std::shared_ptr<Texture> DXSwapchain::getBackBuffer(uint32_t index)
    {
        return _colorAttachments.at(index);
    }

    uint32_t DXSwapchain::nextTexture(const std::shared_ptr<Fence>& fence, uint64_t signalValue)
    {
        uint32_t frame_index = _swapChain->GetCurrentBackBufferIndex();
        _queue.signal(fence, signalValue);
        return frame_index;
    }

    void DXSwapchain::present(const std::shared_ptr<Fence>& fence, uint64_t waitValue, uint64_t signalValue)
    {
        _queue.wait(fence, waitValue);

        if (_desc.vsync)
            _swapChain->Present(1, 0);
        else
            _swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
        
        _queue.signal(fence, signalValue);
    }

    void DXSwapchain::destroy()
    {
        _colorAttachments.clear();
        _swapChain.Reset();
    }
}
