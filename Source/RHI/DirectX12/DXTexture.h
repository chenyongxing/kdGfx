#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../Texture.h"
#include "DXDevice.h"
#include "DXMemory.h"
#include "DescriptorAllocator.h"

namespace kdGfx
{
    class DXTexture : public Texture
    {
    public:
        DXTexture(DXDevice& device, const TextureDesc& desc);
        DXTexture(DXDevice& device, const TextureDesc& desc, const std::shared_ptr<DXMemory>& memory, size_t offset);
        DXTexture(DXDevice& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const TextureDesc& desc);

        std::shared_ptr<TextureView> createView(const TextureViewDesc& desc) override;
        static D3D12_RESOURCE_DESC getResourceDesc(DXDevice& device, const TextureDesc& desc);

        inline Microsoft::WRL::ComPtr<ID3D12Resource> getResource() const { return _resource; }
        inline DXGI_FORMAT getDxgiFormat() const { return _dxgiFormat; }

    private:
        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12Resource> _resource;
        DXGI_FORMAT _dxgiFormat = DXGI_FORMAT_UNKNOWN;
    };

    class DXTextureView : public TextureView
    {
    public:
        DXTextureView(DXDevice& device, DXTexture& texture, const TextureViewDesc& desc);
        virtual ~DXTextureView();

        inline const DXTexture& getTexture() const { return _texture; }
        inline const D3D12_CPU_DESCRIPTOR_HANDLE getRtvCPUHandle() const 
        {
            return _device.getRtvDA()->getCpuHandle(_rtvID);
        }
        inline const D3D12_CPU_DESCRIPTOR_HANDLE getDsvCPUHandle() const
        {
            return _device.getDsvDA()->getCpuHandle(_dsvID);
        }

    private:
        DXDevice& _device;
        DXTexture& _texture;

        DescriptorID _rtvID = 0;
        DescriptorID _dsvID = 0;
    };

    class DXSampler : public Sampler
    {
    public:
        DXSampler(DXDevice& device, const SamplerDesc& desc);
        virtual ~DXSampler();

        inline const DescriptorID getDescriptorID() const { return _descriptorID; };

    private:
        DXDevice& _device;
        DescriptorID _descriptorID = 0;
    };
}
