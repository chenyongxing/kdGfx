#pragma once

#include <vulkan/vulkan.h>

#include "../CommandList.h"
#include "VKPipeline.h"

namespace kdGfx
{
    class VKDevice;

    class VKCommandList : public CommandList
    {
    public:
        VKCommandList(VKDevice& device, CommandListType type);
        virtual ~VKCommandList();

        void reset() override;
        void begin() override;
        void end() override;
        void beginLabel(const std::string& label, uint32_t color) override;
        void endLabel() override;
        void resourceBarrier(const TextureBarrierDesc& desc) override;
        void setPipeline(const std::shared_ptr<Pipeline>& pipeline) override;
        void setPushConstant(const void* data)  override;
        void setBindSet(uint32_t set, const std::shared_ptr<BindSet>& bindSet) override;
        void beginRenderPass(const RenderPassDesc& desc) override;
        void endRenderPass() override;
        void setViewport(float x, float y, float width, float height) override;
        void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
        void setStencilReference(uint32_t reference) override;
        void setVertexBuffer(uint32_t slot, const std::shared_ptr<Buffer>& buffer, size_t offset) override;
        void setIndexBuffer(const std::shared_ptr<Buffer>& buffer, size_t offset, Format indexForamt) override;
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
        void drawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, uint32_t drawCount) override;
        void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void copyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, size_t size) override;
        void copyBufferToTexture(const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<Texture>& texture, uint32_t mipLevel) override;
        void copyTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst,
            glm::uvec2 size, glm::ivec2 srcOffset, glm::ivec2 dstOffset) override;
        void resolveTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst) override;

        inline VkCommandBuffer getCommandBuffer() const { return _commandBuffer; }

    private:
        VKDevice& _device;
        VkCommandPool _commandPool = VK_NULL_HANDLE;
        VkCommandBuffer _commandBuffer = VK_NULL_HANDLE;
        VKRasterPipeline* _stateRasterPipeline = nullptr;
        VKComputePipeline* _stateComputePipeline = nullptr;
    };
}
