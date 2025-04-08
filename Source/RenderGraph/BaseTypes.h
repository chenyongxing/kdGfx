#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
    // DAG（有向无环图）的虚拟资源索引
    using RenderGraphResource = uint32_t;

    // 资源作为Pass的边，用于构建依赖
    struct RenderGraphPassNode;
    struct RenderGraphResourceEdge
    {
        // 只有一个写入Pass或外部导入
        RenderGraphPassNode* producer = nullptr;
        // 可多个Pass读取
        std::vector<RenderGraphPassNode*> consumers;
        // 内部管理资源生命周期
        // 外部导入资源。读的firstUseTask无法确定，写的lastUseTask无法确定
        struct
        {
            uint32_t firstUseTask = 0;
            uint32_t lastUseTask = 0;
        } lifecycle;

        std::string name;
        bool isUAV = false;
        RenderGraphResource handle = 0;
    };

    // Pass类型。带Frame表示和RenderGraph的大小同步（一般设定成后缓冲大小），会自动缩放大小
    enum struct RenderGraphPassType
    {
        Raster,
        Compute,
        Copy,
        FrameRaster,
        FrameCompute
    };

    class RenderGraphRegistry;
    class RenderGraphBuilder;
    // Pass执行函数
    using RenderGraphPassNodeEvaluation = std::function<void(RenderGraphRegistry&, CommandList&)>;
    // Pass构造函数
    using RenderGraphPassNodeConstruction = std::function<RenderGraphPassNodeEvaluation(RenderGraphBuilder&)>;
   
    // 构成DAG的节点，构建依赖，执行图形命令
    struct RenderGraphPassNode
    {
        std::vector<RenderGraphPassNode*> dependencies;
        std::vector<RenderGraphResourceEdge*> inputs;
        std::vector<RenderGraphResourceEdge*> outputs;

        std::string name;
        RenderGraphPassType type = RenderGraphPassType::Raster;
        RenderGraphPassNodeEvaluation evaluation;
        bool isCullBase = false;
    };
}
