#pragma once

#include <vulkan/vulkan.h>

#include "../Device.h"
#include "VKAdapter.h"
#include "VKCommandQueue.h"

namespace kdGfx
{
    class VKDevice : public Device
    {
    public:
        VKDevice(VKAdapter& adapter);
        virtual ~VKDevice();

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
        std::shared_ptr<BindSetLayout>createBindSetLayout(const std::vector<BindEntryLayout>& entryLayouts) override;
        std::shared_ptr<BindSet> createBindSet(const std::shared_ptr<BindSetLayout>& layout) override;
        std::shared_ptr<Pipeline> createComputePipeline(const ComputePipelineDesc& desc) override;
        std::shared_ptr<Pipeline> createRasterPipeline(const RasterPipelineDesc& desc) override;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        uint32_t getMaxDescriptorCount(BindEntryType type);
        Format fromVkFormat(VkFormat format) const;
        VkFormat toVkFormat(Format format) const;
        VkImageLayout toVkImageLayout(TextureState state) const;
        VkDescriptorType toVkDescriptorType(BindEntryType type) const;
        VkShaderStageFlags toVkShaderStageFlags(ShaderType type) const;
        VkImageAspectFlags getAspectFlagsFromFormat(VkFormat format) const;
        VkAccessFlags getAccessFlagsFromImageLayout(VkImageLayout layout) const;
        
        inline const VKAdapter& getAdapter() const { return _adapter; }
        inline VkDevice getDevice() const { return _device; }
        inline uint32_t getQueueFamilyIndex(CommandListType type) const
        {
            if (_queuesInfo.count(type))
                return _queuesInfo.at(type).queueFamilyIndex;
            return 0;
        }
        inline CommandListType getAvailableCommandListType(CommandListType type)
        {
            if (_queuesInfo.count(type))
                return type;
            return CommandListType::General;
        }

    private:
        VKAdapter& _adapter;
        VkDevice _device = VK_NULL_HANDLE;
        struct QueueInfo
        {
            uint32_t queueFamilyIndex = 0;
            uint32_t queueCount = 0;
            std::shared_ptr<VKCommandQueue> commandQueue;
        };
        std::unordered_map<CommandListType, QueueInfo> _queuesInfo;
    };
}
