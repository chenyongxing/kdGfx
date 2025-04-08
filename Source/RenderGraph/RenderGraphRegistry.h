#pragma once

#include "BaseTypes.h"

namespace kdGfx
{
    class RenderGraph;

    // 管理实际GPU资源
    class RenderGraphRegistry
    {
    public:
        friend class RenderGraph;
        friend class RenderGraphBuilder;
        RenderGraphRegistry(RenderGraph& graph);

        RenderGraphResource createBuffer(const BufferDesc& desc);
        RenderGraphResource createTexture(const TextureDesc& desc);
        RenderGraphResource importBuffer(const std::shared_ptr<Buffer>& buffer, const std::string_view name);
        RenderGraphResource importTexture(const std::shared_ptr<Texture>& texture, const std::string_view name);
        RenderGraphResource importTexture(const std::shared_ptr<Texture>& texture, const std::shared_ptr<TextureView>& textureView, const std::string_view name);
        void setImportedTexture(RenderGraphResource handle, const std::shared_ptr<Texture>& texture, const std::shared_ptr<TextureView>& textureView);

        std::shared_ptr<Buffer> getBuffer(RenderGraphResource handle);
        std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>> getTexture(RenderGraphResource handle);
        void destroy();
        void resize(uint32_t width, uint32_t height);

    private:
        RenderGraph& _graph;
        RenderGraphResource _resourcesCount = 0u;
        std::unordered_map<RenderGraphResource, std::tuple<BufferDesc, std::shared_ptr<Buffer>>> _buffersMap;
        std::unordered_map<RenderGraphResource, std::tuple<TextureDesc, size_t, std::shared_ptr<Texture>, std::shared_ptr<TextureView>>> _texturesMap;
        std::unordered_map<RenderGraphResource, std::shared_ptr<Buffer>> _importBuffersMap;
        std::unordered_map<RenderGraphResource, std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>>> _importTexturesMap;
    };
}
