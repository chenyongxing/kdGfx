#pragma once

#include "RHI/Instance.h"
#include "RHI/Adapter.h"
#include "RHI/Device.h"
#include "Misc.h"

// 和createPipeline一起使用用{}包起来防止命名冲突
#define LOAD_SHADER(file, shader, codeData, filePath) \
	Shader shader; \
    std::vector<char> codeData; \
    std::filesystem::path filePath; \
    if (_desc.backend == BackendType::Vulkan) \
        filePath = std::string(file) + ".spv"; \
    else if (_desc.backend == BackendType::DirectX12) \
        filePath = std::string(file) + ".dxil"; \
    if (std::filesystem::exists(filePath)) \
    { \
        LoadBinaryFile(filePath.string(), codeData); \
        shader.code = codeData.data(); \
        shader.codeSize = codeData.size(); \
    } \

#define UPDATE_TEXTURE_BIND(bindSet, binding, textureView) \
    static TextureView* textureView##Cache = nullptr; \
    if (textureView##Cache != textureView.get()) \
    { \
        bindSet->bindTexture(binding, textureView); \
        textureView##Cache = textureView.get(); \
    } \

namespace kdGfx
{
    struct WindowAppDesc
    {
        BackendType backend = BackendType::Vulkan;
        bool debug = false;
        uint32_t width = 800;
        uint32_t height = 600;
        std::string title;
        Format format = Format::Undefined;
        bool vsync = false;

        void parseArgs(int argc, char* argv[])
        {
            for (int i = 1; i < argc; i++)
            {
                std::string arg(argv[i]);
                if (arg == "--vk")
                    backend = BackendType::Vulkan;
                else if (arg == "--dx12")
                    backend = BackendType::DirectX12;
                else if (arg == "--debug")
                    debug = true;
                else if (arg == "--width")
                    width = std::stoi(argv[++i]);
                else if (arg == "--height")
                    height = std::stoi(argv[++i]);
            }
        }
    };

    class ImGuiRenderer;
    class WindowApp
    {
    public:
        WindowApp() = default;
        virtual ~WindowApp();

        bool init(const WindowAppDesc& desc);
        void mainLoop();
        // 窗口函数
        glm::vec2 getCursorPos();
        bool mouseButtonPress(int button = 0);
        bool mouseButtonRelease(int button = 0);
        bool keyPress(int keycode);
        // 常用工具函数
        std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>>
            createTextureFormImage(const std::string& file);
        void transitionTextureState(const std::shared_ptr<Texture>& texture, TextureState state);

        virtual void onSetup() = 0;
        virtual void onResize();
        virtual void onImGui();
        virtual void onUpdate(float deltaTime);
        virtual void onRender(const std::shared_ptr<CommandList>& commandList) = 0;
        virtual void onDestroy();
        
        inline uint32_t getWidth() { return _desc.width; }
        inline uint32_t getHeight() { return _desc.height; }
        inline float getDpiScale() { return _dpiScale; }
        std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>> getBackBuffer(uint32_t index)
        {
            return std::make_tuple(_swapchain->getBackBuffer(index), _backBufferViews.at(index));
        }
        
    private:
        void _reCreateSwapchain();
        void _initImGui();
		void _renderImGui();
        void _destroyImGui();

    protected:
        WindowAppDesc _desc;
        float _dpiScale = 1.0f;
        std::shared_ptr<Instance> _instance;
        std::shared_ptr<Adapter> _adapter;
        std::shared_ptr<Device> _device;
        std::shared_ptr<CommandQueue> _commandQueue;
        std::shared_ptr<Sampler> _nearestClampSampler;
        std::shared_ptr<Sampler> _linerClampSampler;
        std::shared_ptr<Sampler> _linerRepeatSampler;
        std::shared_ptr<Sampler> _anisotropicRepeatSampler;

        uint32_t _frameTotalCount = 0;
        uint32_t _frameCount = 2;
        uint32_t _frameIndex = 0;
        std::shared_ptr<Swapchain> _swapchain;
        std::vector<std::shared_ptr<TextureView>> _backBufferViews;
        
    private:
        uint64_t _fenceValue = 0;
        std::shared_ptr<Fence> _fence;
        std::shared_ptr<CommandList> _commandList;
        uint32_t _lastWidth = 0;
        uint32_t _lastHeight = 0;
        ImGuiRenderer* _imGuiRenderer = nullptr;
        bool _imGuiResourceValid = false;
    };
}
