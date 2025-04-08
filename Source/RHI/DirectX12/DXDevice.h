#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../Device.h"
#include "DXAdapter.h"
#include "DXCommandQueue.h"
#include "DescriptorAllocator.h"

namespace kdGfx
{
    class DXDevice : public Device
    {
    public:
        DXDevice(DXAdapter& adapter);

        std::shared_ptr<CommandQueue> getCommandQueue(CommandListType type) override;
        size_t getTextureAllocateSize(const TextureDesc& desc) override;
        std::shared_ptr<CommandList> createCommandList(CommandListType type) override;
        std::shared_ptr<Fence> createFence(uint64_t initialValue) override;
        std::shared_ptr<Memory> createMemory(const MemoryDesc& desc) override;
        std::shared_ptr<Buffer> createBuffer(const BufferDesc& desc) override;
        std::shared_ptr<Buffer> createBuffer(const BufferDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset) override;
        std::shared_ptr<Texture> createTexture(const TextureDesc& desc) override;
        std::shared_ptr<Texture> createTexture(const TextureDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset) override;
        std::shared_ptr<Sampler> createSampler(const SamplerDesc& desc) override;
        std::shared_ptr<Swapchain> createSwapchain(const SwapchainDesc& desc) override;
        std::shared_ptr<BindSetLayout> createBindSetLayout(const std::vector<BindEntryLayout>& entryLayouts) override;
        std::shared_ptr<BindSet> createBindSet(const std::shared_ptr<BindSetLayout>& layout) override;
        std::shared_ptr<Pipeline> createComputePipeline(const ComputePipelineDesc& desc) override;
        std::shared_ptr<Pipeline> createRasterPipeline(const RasterPipelineDesc& desc) override;

        ID3D12CommandSignature* getCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE type, uint32_t stride);
        Format fromDxgiFormat(DXGI_FORMAT format) const;
        DXGI_FORMAT toDxgiFormat(Format format) const;
        D3D12_RESOURCE_STATES toDxResourceStates(TextureState state) const;

        inline const DXAdapter& getAdapter() const { return _adapter; }
        inline Microsoft::WRL::ComPtr<ID3D12Device> getDevice() const { return _device; }
        inline DescriptorAllocator* getCbvSrvUavDA() const { return _cbvSrvUavDA.get(); }
        inline DescriptorAllocator* getSamplerDA() const { return _samplerDA.get(); }
        inline DescriptorAllocator* getRtvDA() const { return _rtvDA.get(); }
        inline DescriptorAllocator* getDsvDA() const { return _dsvDA.get(); }

    private:
        DXAdapter& _adapter;
        Microsoft::WRL::ComPtr<ID3D12Device> _device;
        Microsoft::WRL::ComPtr<ID3D12Device5> _device5;
        std::unordered_map<CommandListType, std::shared_ptr<DXCommandQueue>> _queues;
        std::unique_ptr<DescriptorAllocator> _cbvSrvUavDA;
        std::unique_ptr<DescriptorAllocator> _samplerDA;
        std::unique_ptr<DescriptorAllocator> _rtvDA;
        std::unique_ptr<DescriptorAllocator> _dsvDA;
        std::map<std::tuple<D3D12_INDIRECT_ARGUMENT_TYPE, uint32_t>,
            Microsoft::WRL::ComPtr<ID3D12CommandSignature>> _commandSignatures;
    };
}
