#pragma once

#include "BaseTypes.h"
#include "CommandQueue.h"
#include "CommandList.h"
#include "Fence.h"
#include "Memory.h"
#include "Buffer.h"
#include "Texture.h"
#include "Swapchain.h"
#include "BindSetLayout.h"
#include "BindSet.h"
#include "Pipeline.h"

namespace kdGfx
{
    // 逻辑设备，管理资源和命令队列
    class Device
    {
    public:
        virtual ~Device() = default;

        virtual std::shared_ptr<CommandQueue> getCommandQueue(CommandListType type) = 0;
        virtual size_t getTextureAllocateSize(const TextureDesc& desc) = 0;
        virtual std::shared_ptr<CommandList> createCommandList(CommandListType type) = 0;
        virtual std::shared_ptr<Fence> createFence(uint64_t initialValue) = 0;
        virtual std::shared_ptr<Memory> createMemory(const MemoryDesc& desc) = 0;
        virtual std::shared_ptr<Buffer> createBuffer(const BufferDesc& desc) = 0;
        virtual std::shared_ptr<Buffer> createBuffer(const BufferDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset) = 0;
        virtual std::shared_ptr<Texture> createTexture(const TextureDesc& desc) = 0;
        virtual std::shared_ptr<Texture> createTexture(const TextureDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset) = 0;
        virtual std::shared_ptr<Sampler> createSampler(const SamplerDesc& desc) = 0;
        virtual std::shared_ptr<Swapchain> createSwapchain(const SwapchainDesc& desc) = 0;
        virtual std::shared_ptr<BindSetLayout>createBindSetLayout(const std::vector<BindEntryLayout>& entryLayouts) = 0;
        virtual std::shared_ptr<BindSet> createBindSet(const std::shared_ptr<BindSetLayout>& layout) = 0;
        virtual std::shared_ptr<Pipeline> createComputePipeline(const ComputePipelineDesc& desc) = 0;
        virtual std::shared_ptr<Pipeline> createRasterPipeline(const RasterPipelineDesc& desc) = 0;

        inline const bool isRayQuerySupported() const { return _rayQuerySupported; }

    protected:
        bool _rayQuerySupported = false;
    };
}
