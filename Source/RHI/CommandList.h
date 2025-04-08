#pragma once

#include "BaseTypes.h"
#include "Pipeline.h"
#include "BindSet.h"
#include "Buffer.h"
#include "Texture.h"

namespace kdGfx
{
    // 记录命令
    class CommandList
    {
    public:
        virtual ~CommandList() = default;

        virtual void reset() = 0;
        virtual void begin() = 0;
        virtual void end() = 0;
        virtual void beginLabel(const std::string& label, uint32_t color = 0x00000000) = 0;
        virtual void endLabel() = 0;
        // DX12切换状态只允许在主队列
        virtual void resourceBarrier(const TextureBarrierDesc& desc) = 0;
        virtual void setPipeline(const std::shared_ptr<Pipeline>& pipeline) = 0;
        virtual void setPushConstant(const void* data) = 0;
        // 必须要setPipeline之后调用
        virtual void setBindSet(uint32_t set, const std::shared_ptr<BindSet>& bindSet) = 0;
        // raster
        virtual void beginRenderPass(const RenderPassDesc& desc) = 0;
        virtual void endRenderPass() = 0;
        virtual void setViewport(float x, float y, float width, float height) = 0;
        virtual void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
        virtual void setStencilReference(uint32_t reference) = 0;
        virtual void setVertexBuffer(uint32_t slot,
                                     const std::shared_ptr<Buffer>& buffer,
                                     size_t offset = 0) = 0;
        virtual void setIndexBuffer(const std::shared_ptr<Buffer>& buffer,
                                    size_t offset = 0,
                                    Format indexForamt = Format::R32Uint) = 0;
        virtual void draw(uint32_t vertexCount,
                          uint32_t instanceCount = 1,
                          uint32_t firstVertex = 0,
                          uint32_t firstInstance = 0) = 0;
        virtual void drawIndexed(uint32_t indexCount,
                                 uint32_t instanceCount = 1,
                                 uint32_t firstIndex = 0,
                                 int32_t vertexOffset = 0,
                                 uint32_t firstInstance = 0) = 0;
        virtual void drawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, uint32_t drawCount) = 0;
        
            // compute
        virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
        // copy
        virtual void copyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, size_t size) = 0;
        virtual void copyBufferToTexture(const std::shared_ptr<Buffer>& buffer,
                                         const std::shared_ptr<Texture>& texture,
                                         uint32_t mipLevel = 0) = 0;
        virtual void copyTexture(const std::shared_ptr<Texture>& src,
                                 const std::shared_ptr<Texture>& dst,
                                 glm::uvec2 size,
                                 glm::ivec2 srcOffset = { 0, 0 },
                                 glm::ivec2 dstOffset = { 0, 0 }) = 0;
        virtual void resolveTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst) = 0;
    };
}
