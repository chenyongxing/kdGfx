#pragma once

#include "RenderGraphRegistry.h"

namespace kdGfx
{
    class RenderGraph final
    {
        friend class RenderGraphRegistry;
        friend class RenderGraphBuilder;
    public:
        std::string name;

        RenderGraph(const std::shared_ptr<Device>& device);
        ~RenderGraph();
        // isCullbase裁剪图的依据，一般是最后渲染到后缓冲区Pass
        void addPass(const std::string_view name, RenderGraphPassType type,
            RenderGraphPassNodeConstruction construction, bool isCullBase = false);
        void compile();
        void execute();
        void execute(std::shared_ptr<Fence> fence, uint64_t waitValue, uint64_t signalValue);
        // 自动缩放所有帧纹理大小
        void resize(uint32_t width, uint32_t height);
        // 生成可视化dot，可以用Graphviz查看
        void genDotFile(const std::string& filename, bool showImportRes = false);
        // imgui渲染节点图，可以动态调节节点连接
        void drawImNodes(float dpiScale);

        inline RenderGraphRegistry& getRegistry() { return _registry; }
        inline void markDirty()
        {
            _compiled = false;
            _dirty = true;
            _imNodesDirty = true;
        }
        inline void setCommandList(const std::shared_ptr<CommandList>& commandList)
        {
            _commandList = commandList;
        }

    private:
        std::shared_ptr<Device> _device;
        std::shared_ptr<CommandList> _commandList;
        std::shared_ptr<Memory> _memory;
        RenderGraphRegistry _registry;
        
        std::vector<RenderGraphPassNode*> _passes;
        std::unordered_map<RenderGraphResource, RenderGraphResourceEdge*> _resourceEdgesMap;
        std::vector<std::vector<RenderGraphPassNode*>> _passesTasks;
        
        uint32_t _width = 0;
        uint32_t _height = 0;
        bool _compiled = false;
        bool _dirty = true;
        bool _imNodesDirty = true;

    private:
        void _cullPasses();
        void _genParallelTasks();
        void _allocResources();
        void _allocMemory();
        void _createResources();
        void _autoSetTextureUsage();
        void _autoTransitionTextureState(RenderGraphPassNode* passNode, std::shared_ptr<CommandList> commandList);
        inline RenderGraphResourceEdge* _createResourceEdge(const std::string_view name, RenderGraphResource handle)
        {
            auto resourceEdge = new RenderGraphResourceEdge();
            resourceEdge->name = name;
            resourceEdge->handle = handle;
            assert(!_resourceEdgesMap.contains(handle));
            _resourceEdgesMap[handle] = resourceEdge;
            return resourceEdge;
        }
        inline RenderGraphResourceEdge* _getResourceEdge(RenderGraphResource handle)
        {
            if (_resourceEdgesMap.contains(handle))
            {
                return _resourceEdgesMap[handle];
            }
            assert(true);
            return nullptr;
        }
    };
}
