#include "DXDevice.h"
#include "DXCommandList.h"
#include "DXFence.h"
#include "DXMemory.h"
#include "DXBuffer.h"
#include "DXTexture.h"
#include "DXSwapchain.h"
#include "DXBindSetLayout.h"
#include "DXBindSet.h"
#include "DXPipeline.h"
#include <directx/d3dx12.h>

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXDevice::DXDevice(DXAdapter& adapter) :
		_adapter(adapter)
	{
        if (FAILED(D3D12CreateDevice(_adapter.getAdapter().Get(), D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&_device))))
        {
            RuntimeError("failed to create d3d12 device");
        }
        _device.As(&_device5);

        ComPtr<ID3D12DeviceConfiguration> deviceConfig;
        if (SUCCEEDED(_device->QueryInterface(IID_PPV_ARGS(&deviceConfig))))
        {
            D3D12_DEVICE_CONFIGURATION_DESC configDesc = deviceConfig->GetDesc();
            spdlog::info("d3d12 device sdk version: {}", configDesc.SDKVersion);
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
        if (SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5))))
        {
           _rayQuerySupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
        }

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_HIGHEST_SHADER_MODEL };
        if (SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
        {
            uint32_t major = (shaderModel.HighestShaderModel >> 4) & 0xF;
            uint32_t minor = shaderModel.HighestShaderModel & 0xF;
            spdlog::info("d3d12 highest shader model: {}_{}", major, minor);
        }

        _queues[CommandListType::General] = std::make_shared<DXCommandQueue>(*this, CommandListType::General);
        _queues[CommandListType::Compute] = std::make_shared<DXCommandQueue>(*this, CommandListType::Compute);
        _queues[CommandListType::Copy] = std::make_shared<DXCommandQueue>(*this, CommandListType::Copy);

        _cbvSrvUavDA = std::make_unique<DescriptorAllocator>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512);
        _samplerDA = std::make_unique<DescriptorAllocator>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 8);
        _rtvDA = std::make_unique<DescriptorAllocator>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 64);
        _dsvDA = std::make_unique<DescriptorAllocator>(_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 16);
	}

    std::shared_ptr<CommandQueue> DXDevice::getCommandQueue(CommandListType type)
    {
        return _queues[type];
    }

    size_t DXDevice::getTextureAllocateSize(const TextureDesc& desc)
    {
        D3D12_RESOURCE_DESC resourceDesc = std::move(DXTexture::getResourceDesc(*this, desc));
        D3D12_RESOURCE_ALLOCATION_INFO allocInfo = _device->GetResourceAllocationInfo(0, 1, &resourceDesc);
        return MemAlign(allocInfo.SizeInBytes, allocInfo.Alignment);
    }

    std::shared_ptr<CommandList> DXDevice::createCommandList(CommandListType type)
    {
        return std::make_shared<DXCommandList>(*this, type);
    }

    std::shared_ptr<Fence> DXDevice::createFence(uint64_t initialValue)
    {
        return std::make_shared<DXFence>(*this, initialValue);
    }

    std::shared_ptr<Memory> DXDevice::createMemory(const MemoryDesc& desc)
    {
        return std::make_shared<DXMemory>(*this, desc);
    }

    std::shared_ptr<Buffer> DXDevice::createBuffer(const BufferDesc& desc)
    {
        return std::make_shared<DXBuffer>(*this, desc);
    }

    std::shared_ptr<Buffer> DXDevice::createBuffer(const BufferDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset)
    {
        auto dxMemory = std::dynamic_pointer_cast<DXMemory>(memory);
        return std::make_shared<DXBuffer>(*this, desc, dxMemory, offset);
    }

    std::shared_ptr<Texture> DXDevice::createTexture(const TextureDesc& desc)
    {
        return std::make_shared<DXTexture>(*this, desc);
    }

    std::shared_ptr<Texture> DXDevice::createTexture(const TextureDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset)
    {
        auto dxMemory = std::dynamic_pointer_cast<DXMemory>(memory);
        return std::make_shared<DXTexture>(*this, desc, dxMemory, offset);
    }

    std::shared_ptr<Sampler> DXDevice::createSampler(const SamplerDesc& desc)
    {
        return std::make_shared<DXSampler>(*this, desc);
    }

    std::shared_ptr<Swapchain> DXDevice::createSwapchain(const SwapchainDesc& desc)
    {
        return std::make_shared<DXSwapchain>(*this, *_queues[CommandListType::General], desc);
    }

    std::shared_ptr<BindSetLayout> DXDevice::createBindSetLayout(const std::vector<BindEntryLayout>& entryLayouts)
    {
        return std::make_shared<DXBindSetLayout>(*this, entryLayouts);
    }

    std::shared_ptr<BindSet> DXDevice::createBindSet(const std::shared_ptr<BindSetLayout>& layout)
    {
        return std::make_shared<DXBindSet>(*this, layout);
    }

    std::shared_ptr<Pipeline> DXDevice::createComputePipeline(const ComputePipelineDesc& desc)
    {
        return std::make_shared<DXComputePipeline>(*this, desc);
    }

    std::shared_ptr<Pipeline> DXDevice::createRasterPipeline(const RasterPipelineDesc& desc)
    {
        return std::make_shared<DXRasterPipeline>(*this, desc);
    }

    ID3D12CommandSignature* DXDevice::getCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride)
    {
        auto it = _commandSignatures.find(std::make_tuple(type, stride));
        if (it != _commandSignatures.end()) {
            return it->second.Get();
        }

        D3D12_INDIRECT_ARGUMENT_DESC signatureArg = { type };
        D3D12_COMMAND_SIGNATURE_DESC signatureDesc =
        {
            .ByteStride = stride,
            .NumArgumentDescs = 1,
            .pArgumentDescs = &signatureArg
        };
        
        ComPtr<ID3D12CommandSignature> commandSignature;
        if (FAILED(_device->CreateCommandSignature(&signatureDesc, nullptr, IID_PPV_ARGS(&commandSignature))))
        {
            spdlog::error("failed to create dx12 commandSignature");
        }
        _commandSignatures[std::forward_as_tuple(type, stride)] = commandSignature;
        return commandSignature.Get();
    }

    Format DXDevice::fromDxgiFormat(DXGI_FORMAT format) const
    {
        static std::unordered_map<DXGI_FORMAT, Format> mapping =
        {
            { DXGI_FORMAT_UNKNOWN, Format::Undefined },
            { DXGI_FORMAT_R8_UNORM, Format::R8Unorm },
            { DXGI_FORMAT_R16_UINT, Format::R16Uint },
            { DXGI_FORMAT_R32_UINT, Format::R32Uint },
            { DXGI_FORMAT_R8G8B8A8_UNORM, Format::RGBA8Unorm },
            { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Format::RGBA8Srgb },
            { DXGI_FORMAT_B8G8R8A8_UNORM, Format::BGRA8Unorm },
            { DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, Format::BGRA8Srgb },
            { DXGI_FORMAT_R16G16B16A16_UNORM, Format::RGBA16Unorm },
            { DXGI_FORMAT_R16G16B16A16_FLOAT, Format::RGBA16Sfloat },
            { DXGI_FORMAT_R32_FLOAT, Format::R32Sfloat },
            { DXGI_FORMAT_R32G32_FLOAT, Format::RG32Sfloat },
            { DXGI_FORMAT_R32G32B32_FLOAT, Format::RGB32Sfloat },
            { DXGI_FORMAT_R32G32B32A32_FLOAT, Format::RGBA32Sfloat },
            { DXGI_FORMAT_D16_UNORM, Format::D16Unorm },
            { DXGI_FORMAT_D24_UNORM_S8_UINT, Format::D24UnormS8Uint },
            { DXGI_FORMAT_D32_FLOAT, Format::D32Sfloat },
            { DXGI_FORMAT_D32_FLOAT_S8X24_UINT, Format::D32SfloatS8Uint }
        };
        assert(mapping.count(format));
        return mapping[format];
    }

    DXGI_FORMAT DXDevice::toDxgiFormat(Format format) const
    {
        static std::unordered_map<Format, DXGI_FORMAT> mapping =
        {
            { Format::Undefined,    DXGI_FORMAT_UNKNOWN },
            { Format::R8Unorm,      DXGI_FORMAT_R8_UNORM },
            { Format::R16Uint,      DXGI_FORMAT_R16_UINT },
            { Format::R32Uint,      DXGI_FORMAT_R32_UINT },
            { Format::RGBA8Unorm,   DXGI_FORMAT_R8G8B8A8_UNORM },
            { Format::RGBA8Srgb,    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
            { Format::BGRA8Unorm,   DXGI_FORMAT_B8G8R8A8_UNORM },
            { Format::BGRA8Srgb,    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB },
            { Format::RGBA16Unorm,  DXGI_FORMAT_R16G16B16A16_UNORM },
            { Format::RGBA16Sfloat, DXGI_FORMAT_R16G16B16A16_FLOAT },
            { Format::R32Sfloat,    DXGI_FORMAT_R32_FLOAT },
            { Format::RG32Sfloat,   DXGI_FORMAT_R32G32_FLOAT },
            { Format::RGB32Sfloat,  DXGI_FORMAT_R32G32B32_FLOAT },
            { Format::RGBA32Sfloat, DXGI_FORMAT_R32G32B32A32_FLOAT },
            { Format::D16Unorm,     DXGI_FORMAT_D16_UNORM },
            { Format::X8D24Unorm,   DXGI_FORMAT_D24_UNORM_S8_UINT },
            { Format::D32Sfloat,    DXGI_FORMAT_D32_FLOAT },
            { Format::D24UnormS8Uint,  DXGI_FORMAT_D24_UNORM_S8_UINT },
            { Format::D32SfloatS8Uint, DXGI_FORMAT_D32_FLOAT_S8X24_UINT }
        };
        assert(mapping.count(format));
        return mapping[format];
    }

    D3D12_RESOURCE_STATES DXDevice::toDxResourceStates(TextureState state) const
    {
        static std::unordered_map<TextureState, D3D12_RESOURCE_STATES> mapping =
        {
            { TextureState::Undefined,         D3D12_RESOURCE_STATE_COMMON },
            { TextureState::General,           D3D12_RESOURCE_STATE_COMMON },
            { TextureState::CopyDst,          D3D12_RESOURCE_STATE_COPY_DEST },
            { TextureState::CopySrc,        D3D12_RESOURCE_STATE_COPY_SOURCE },
            { TextureState::ColorAttachment,   D3D12_RESOURCE_STATE_RENDER_TARGET },
            { TextureState::DepthStencilRead,  D3D12_RESOURCE_STATE_DEPTH_READ },
            { TextureState::DepthStencilWrite, D3D12_RESOURCE_STATE_DEPTH_WRITE },
            { TextureState::ShaderRead,        D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE },
            { TextureState::Present,           D3D12_RESOURCE_STATE_PRESENT }
        };
        assert(mapping.count(state));
        return mapping[state];
    }
}
