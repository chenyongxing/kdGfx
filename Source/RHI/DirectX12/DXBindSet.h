#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../BindSet.h"
#include "DXDevice.h"
#include "DXBindSetLayout.h"
#include "DescriptorAllocator.h"

namespace kdGfx
{
    struct DXBindSlot
    {
        enum Type
        {
            ConstantBuffer = 0,
            ShaderResource = 1,
            UnorderedAccess = 2,
            Sampler = 3
        };
        Type type = ConstantBuffer;
        uint32_t shaderRegister = 0;
        uint32_t space = 0;

        inline bool operator<(const DXBindSlot& other) const
        {
            if (type != other.type) return type < other.type;
            if (space != other.space) return space < other.space;
            return shaderRegister < other.shaderRegister;
        }

        inline void setType(BindEntryType bindEntryType)
        {
            switch (bindEntryType)
            {
            case BindEntryType::ConstantBuffer:
                type = DXBindSlot::ConstantBuffer;
                break;
            case BindEntryType::ReadedBuffer:
            case BindEntryType::SampledTexture:
                type = DXBindSlot::ShaderResource;
                break;
            case BindEntryType::StorageBuffer:
            case BindEntryType::StorageTexture:
                type = DXBindSlot::UnorderedAccess;
                break;
            case BindEntryType::Sampler:
                type = DXBindSlot::Sampler;
                break;
            default:
                assert(false);
                break;
            }
        }

        inline static Type getType(BindEntryType bindEntryType)
        {
            switch (bindEntryType)
            {
            case BindEntryType::ConstantBuffer:
                return DXBindSlot::ConstantBuffer;
            case BindEntryType::ReadedBuffer:
            case BindEntryType::SampledTexture:
                return DXBindSlot::ShaderResource;
            case BindEntryType::StorageBuffer:
            case BindEntryType::StorageTexture:
                return DXBindSlot::UnorderedAccess;
            case BindEntryType::Sampler:
                return DXBindSlot::Sampler;
            default:
                assert(false);
                return DXBindSlot::ConstantBuffer;
            }
        }
    };

    // bindless需要分配实际大小，在bind时候生成Cbv Srv Uav
    class DXBindSet : public BindSet
    {
    public:
        DXBindSet(DXDevice& device, const std::shared_ptr<BindSetLayout>& layout);
        virtual ~DXBindSet();

        void bindBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer) override;
        void bindTexture(uint32_t binding, const std::shared_ptr<TextureView>& textureView) override;
        void bindSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler) override;
		void bindTexture(uint32_t binding, TextureView* textureView) override;
        void bindTextures(uint32_t binding, const std::vector<std::shared_ptr<TextureView>>& textureViews) override;

        inline D3D12_GPU_DESCRIPTOR_HANDLE getSlotGPUHandle(const DXBindSlot& slot) const
        {
            DescriptorID id = _slotDescriptorsMap.at(slot).at(0);
            if (slot.type == DXBindSlot::Sampler)
                return _device.getSamplerDA()->getGpuHandle(id);
            else
                return _device.getCbvSrvUavDA()->getGpuHandle(id);
        }

    private:
        void _clearSlotDescriptors(uint32_t binding, DXBindSlot::Type type);
        DXBindSlot _findSlot(uint32_t binding, DXBindSlot::Type type);
        
        DXDevice& _device;
        std::map<DXBindSlot, std::vector<DescriptorID>> _slotDescriptorsMap;
    };
}
