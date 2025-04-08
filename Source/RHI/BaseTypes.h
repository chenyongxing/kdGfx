#pragma once

#include "../PCH.h"

namespace kdGfx
{
    enum struct BackendType
    {
        Vulkan,
        DirectX12
    };

    enum struct Format
    {
        Undefined,
        R8Unorm,
        R16Uint,
        R32Uint,
        RGBA8Unorm,
        RGBA8Srgb,
        BGRA8Unorm,
        BGRA8Srgb,
        RGBA16Unorm,
        RGBA16Sfloat,
        R32Sfloat,
        RG32Sfloat,
        RGB32Sfloat,
        RGBA32Sfloat,
        D16Unorm,
        X8D24Unorm,
        D32Sfloat,
        D24UnormS8Uint,
        D32SfloatS8Uint
    };

    enum struct CommandListType
    {
        General,
        Compute,
        Copy
    };

    enum struct MemoryType
    {
        Device,
        Upload,
        Readback
    };

    struct MemoryDesc
    {
        size_t size = 0;
        MemoryType type = MemoryType::Device;
        std::string name;
    };

    enum struct BufferUsage
    {
        Undefined = 0,
        CopyDst = 1 << 0,
        CopySrc = 1 << 1,
        Constant = 1 << 2,
        Storage = 1 << 3,
        Index = 1 << 4,
        Vertex = 1 << 5,
        Indirect = 1 << 6
    };
    ENUM_BITWISE_OPERATOR(BufferUsage)

    struct BufferDesc
    {
        size_t size = 0;
        // indirectDraw commandBuffer stride + directX12 StructuredBuffer stride
        uint32_t stride = 0;
        BufferUsage usage = BufferUsage::Undefined;
        bool hostVisible = false;
        std::string name;
    };

    enum struct TextureType
    {
        e1D,
        e2D,
        e3D,
    };

    enum struct TextureUsage
    {
        Undefined = 0,
        CopyDst = 1 << 0,
        CopySrc = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencilAttachment = 1 << 5
    };
    ENUM_BITWISE_OPERATOR(TextureUsage)

    struct TextureDesc
    {
        TextureType type = TextureType::e2D;
        TextureUsage usage = TextureUsage::Undefined;
        Format format = Format::RGBA8Unorm;
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        uint32_t sampleCount = 1;
        std::string name;
    };

    enum struct TextureState
    {
        Undefined,
        General,
        CopyDst,
        CopySrc,
        ColorAttachment,
        DepthStencilRead,
        DepthStencilWrite,
        ShaderRead,
        Present,
    };

    class Texture;
    struct TextureBarrierDesc
    {
        std::shared_ptr<Texture> texture;
        TextureState oldState = TextureState::Undefined;
        TextureState newState = TextureState::Undefined;
        struct
        {
            uint32_t baseMipLevel = 0;
            uint32_t levelCount = 1;
            uint32_t baseArrayLayer = 0;
            uint32_t layerCount = 1;
        } subRange;
    };

    struct TextureViewDesc
    {
        uint32_t baseMipLevel = 0;
        uint32_t levelCount = 1;
        uint32_t baseArrayLayer = 0;
        uint32_t layerCount = 1;
    };

    enum struct Filter
    {
        Nearest,
        Linear,
        Anisotropic
    };

    enum struct AddressMode
    {
        Repeat,
        Mirror,
        Clamp
    };

    struct SamplerDesc
    {
        Filter filter = Filter::Nearest;
        AddressMode addressMode = AddressMode::Repeat;
        uint32_t maxAnisotropy = 16;
        float maxLOD = FLT_MAX;
    };

    class Swapchain;
    struct SwapchainDesc
    {
        void* window = nullptr;
        Format format = Format::Undefined;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t frameCount = 2;
        bool vsync = false;
        std::weak_ptr<Swapchain> oldSwapchain;
    };

    enum struct LoadOp
    {
        Load,
        Clear,
        DontCare
    };

    enum struct StoreOp
    {
        Store,
        DontCare
    };

    class TextureView;
    struct RenderPassAttachment
    {
        std::shared_ptr<TextureView> textureView;
        LoadOp loadOp = LoadOp::Load;
        StoreOp storeOp = StoreOp::Store;
        std::array<float, 4> clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
        float clearDepth = 1.0f;
    };

    enum struct ShaderType
    {
        Vertex = 1 << 0,
        Pixel = 1 << 1,
        Compute = 1 << 2,
        Raster = Vertex | Pixel,
        All = Vertex | Pixel | Compute
    };
    ENUM_BITWISE_OPERATOR(ShaderType)

    struct Shader
    {
        size_t codeSize = 0;
        const void* code = nullptr;
    };

    struct PushConstantLayout
    {
        uint32_t size = 0;
        // directX12
        uint32_t shaderRegister = 0;
        // directX12
        uint32_t space = 0;
    };

    enum struct BindEntryType
    {
        ConstantBuffer,
        ReadedBuffer,
        StorageBuffer, // uav
        SampledTexture,
        StorageTexture, // uav
        Sampler
    };

    struct BindEntryLayout
    {
        // vulkan，Bindless的binding需要最后
        uint32_t binding = 0;
        // directX12
        uint32_t shaderRegister = 0;
        // directX12
        uint32_t space = 0;
        BindEntryType type = BindEntryType::ConstantBuffer;
        // 如果是Bindless设置成UINT32_MAX
        uint32_t count = 1;
        ShaderType visible = ShaderType::All;
    };

    class BindSetLayout;
    struct ComputePipelineDesc
    {
        Shader shader;
        PushConstantLayout pushConstantLayout;
        std::vector<std::shared_ptr<BindSetLayout>> bindSetLayouts;
    };

    enum struct Topology
    {
        PointList,
        LineList,
        TriangleList,
        TriangleStrip,
        TriangleFan
    };

    enum struct PolygonMode
    {
        Fill,
        Point
    };

    enum struct CullMode
    {
        None,
        Front,
        Back
    };

    enum struct FrontFace
    {
        Clockwise,
        CounterClockwise
    };

    struct VertexAttributeDesc
    {
        // vulkan
        uint32_t location = 0;
        // directX12
        std::string semantic;
        uint32_t offset = 0;
        Format format = Format::Undefined;
    };

    enum struct BlendOp
    {
        Add,
        Sub,
        RevSub
    };

    enum struct BlendFactor
    {
        Zero,
        One,
        SrcColor,
        InvSrcColor,
        SrcAlpha,
        InvSrcAlpha,
        DstColor,
        InvDstColor,
        DstAlpha,
        InvDstAlpha,
    };

    struct BlendState
    {
        bool enable = false;
        BlendOp colorOp = BlendOp::Add;
        BlendFactor colorSrcFactor = BlendFactor::SrcAlpha;
        BlendFactor colorDstFactor = BlendFactor::InvSrcAlpha;
        BlendOp alphaOp = BlendOp::Add;
        BlendFactor alphaSrcFactor = BlendFactor::One;
        BlendFactor alphaDstFactor = BlendFactor::Zero;
    };

    enum struct CompareOp
    {
        Never,
        Less,
        Greater
    };

    struct RasterPipelineDesc
    {
        Shader vertex;
        Shader pixel;
        PushConstantLayout pushConstantLayout;
        std::vector<std::shared_ptr<BindSetLayout>> bindSetLayouts;

        // primitive state
        Topology topology = Topology::TriangleList;
        PolygonMode polygonMode = PolygonMode::Fill;
        CullMode cullMode = CullMode::None;
        FrontFace frontFace = FrontFace::Clockwise;
        float lineWidth = 1.0f;

        uint32_t sampleCount = 1;
        std::vector<VertexAttributeDesc> vertexAttributes;
        std::vector<Format> colorFormats;
        Format depthStencilFormat = Format::Undefined;
        std::vector<BlendState> colorBlends;
        CompareOp depthTest = CompareOp::Never;

        inline uint32_t getVertexStride() const
        {
            uint32_t vertexStride = 0;
            for (const auto& va : vertexAttributes)
            {
                switch (va.format)
                {
                case Format::RG32Sfloat:
                    vertexStride += 2 * sizeof(float);
                    break;
                case Format::RGB32Sfloat:
                    vertexStride += 3 * sizeof(float);
                    break;
                case Format::RGBA32Sfloat:
                    vertexStride += 4 * sizeof(float);
                    break;
                case Format::RGBA8Unorm:
                case Format::RGBA8Srgb:
                case Format::BGRA8Unorm:
                case Format::BGRA8Srgb:
                    vertexStride += 4 * sizeof(uint8_t);
                    break;
                case Format::RGBA16Unorm:
                case Format::RGBA16Sfloat:
                    vertexStride += 4 * sizeof(uint16_t);
                    break;
                case Format::R8Unorm:
                    vertexStride += sizeof(uint8_t);
                    break;
                case Format::R16Uint:
                    vertexStride += sizeof(uint16_t);
                    break;
                case Format::R32Uint:
                    vertexStride += sizeof(uint32_t);
                    break;
                case Format::R32Sfloat:
                    vertexStride += sizeof(float);
                    break;
                }
            }
            return vertexStride;
        }
    };

    struct RenderPassDesc
    {
        std::vector<RenderPassAttachment> colorAttachments;
        RenderPassAttachment depthAttachment;
    };

    struct DrawIndexedIndirectCommand
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t firstInstance;
    };
}
