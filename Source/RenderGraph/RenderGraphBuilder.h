#pragma once

#include "BaseTypes.h"
#include "RenderGraph.h"

namespace kdGfx
{
    // 设置节点读写依赖，构建DAG边
    class RenderGraphBuilder
    {
    public:
        RenderGraphBuilder(RenderGraph& graph, RenderGraphPassNode* passNode)
            : _graph(graph), _passNode(passNode) {};

        inline RenderGraphResource read(RenderGraphResource resource)
        {
            auto resourceEdge = _graph._getResourceEdge(resource);
            resourceEdge->consumers.emplace_back(_passNode);
            _passNode->inputs.emplace_back(resourceEdge);
            return resource;
        }
        inline RenderGraphResource write(RenderGraphResource resource)
        {
            auto resourceEdge = _graph._getResourceEdge(resource);
            resourceEdge->producer = _passNode;
            _passNode->outputs.emplace_back(resourceEdge);
            return resource;
        }
        
        // 导入外部资源，渲染图不管理生命周期
        inline RenderGraphResource importBuffer(const std::shared_ptr<Buffer>& buffer, const std::string_view name = "")
        {
            return _graph.getRegistry().importBuffer(buffer, name);
        }
        
        inline  RenderGraphResource importTexture(const std::shared_ptr<Texture>& texture, const std::string_view name = "")
        {
            return _graph.getRegistry().importTexture(texture, name);
        }
        
        // 注意：导入后缓冲区持有引用会导致交换链无法释放
        // 不要导入后缓冲区贴图，使用空资源节点替代，录制渲染命令时候实时获取当前后缓冲区
        // 或重建交换链之前setImportedTexture(null)，重建交换链之后setImportedTexture(new)
        inline  RenderGraphResource importTexture(const std::shared_ptr<Texture>& texture, const std::shared_ptr<TextureView>& textureView, const std::string_view name = "")
        {
            return _graph.getRegistry().importTexture(texture, textureView, name);
        }
        
        inline  RenderGraphResource createBuffer(const BufferDesc& desc)
        {
            return _graph.getRegistry().createBuffer(desc);
        }
        
        // FramePass不需要宽高，渲染图自动缩放
        // Usage渲染图会自动推导
        inline RenderGraphResource createTexture(const TextureDesc& desc)
        {
            return _graph.getRegistry().createTexture(desc);
        }

    private:
        RenderGraph& _graph;
        RenderGraphPassNode* _passNode = nullptr;
    };
}
