#include "WindowApp.h"
#include "StagingBuffer.h"
#include "ImageProcessor.h"
#include "MipMapsGen.h"
#include "ImGuiRenderer.h"

#include <GLFW/glfw3.h>
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux)
#if defined(USE_XCB)
#define GLFW_EXPOSE_NATIVE_X11
#else
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#endif
#include <GLFW/glfw3native.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imnodes/imnodes.h>
#include <stb_image.h>

namespace kdGfx
{
	static GLFWwindow* gWindow = nullptr;

	WindowApp::~WindowApp()
	{
		_swapchain.reset();
		glfwDestroyWindow(gWindow);
	}
	
	bool WindowApp::init(const WindowAppDesc& desc)
	{
		_desc = desc;
		
		_instance = CreateInstance(_desc.backend, desc.debug);
		_adapter = std::move(_instance->enumerateAdapters()[0]);
		spdlog::info("use GPU: {}", _adapter->getName());
		_device = _adapter->createDevice();
		_desc.backend = _instance->getBackendType();

		if (!glfwInit())
		{
			RuntimeError("failed init glfw");
		}
		float yScale = 1.0f;
		glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &_dpiScale, &yScale);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		gWindow = glfwCreateWindow(_desc.width * _dpiScale, _desc.height * _dpiScale, _desc.title.data(), nullptr, nullptr);
		if (gWindow == nullptr)
		{
			RuntimeError("failed to create window");
		}
		int width, height;
		glfwGetFramebufferSize(gWindow, &width, &height);
		_desc.width = (uint32_t)width;
		_desc.height = (uint32_t)height;

		_commandQueue = _device->getCommandQueue(CommandListType::General);

		_nearestClampSampler = _device->createSampler({ Filter::Nearest, AddressMode::Clamp });
		_linerClampSampler = _device->createSampler({ Filter::Linear, AddressMode::Clamp });
		_linerRepeatSampler = _device->createSampler({ Filter::Linear, AddressMode::Repeat });
		_anisotropicRepeatSampler = _device->createSampler({ Filter::Anisotropic, AddressMode::Repeat });

		_fence = _device->createFence(_fenceValue);
		
		StagingBuffer::initUploadGlobal(_desc.backend, _device);
		StagingBuffer::initReadbackGlobal(_desc.backend, _device);
		ImageProcessor::initSingleton(_desc.backend, _device);
		MipMapsGen::initSingleton(_desc.backend, _device);

		_commandList = _device->createCommandList(CommandListType::General);

		_reCreateSwapchain();

		_initImGui();

		return true;
	}
	
	void WindowApp::mainLoop()
	{
		onSetup();

		auto lastTime = std::chrono::steady_clock::now();

		while (!glfwWindowShouldClose(gWindow))
		{
			glfwPollEvents();

			int width, height;
			glfwGetFramebufferSize(gWindow, &width, &height);
			_desc.width = (uint32_t)width;
			_desc.height = (uint32_t)height;
			if (_desc.width == 0 || _desc.height == 0)	continue;

			auto currentTime = std::chrono::steady_clock::now();
			std::chrono::duration<float> deltaTimeDura = currentTime - lastTime;
			lastTime = currentTime;
			float deltaTime = deltaTimeDura.count();
			onUpdate(deltaTime);

			// 等待上一帧命令和呈现完成
			_fence->wait(_fenceValue);

			// ImGui计算和gpu不异步，更方便修改gpu资源
			_renderImGui();

			// 窗口大小变化重建交换链
			if (_desc.width != _lastWidth || _desc.height != _lastHeight)
			{
				_reCreateSwapchain();
				continue;
			}

			// 获取下个后缓冲进行渲染
			_frameIndex = _swapchain->nextTexture(_fence, ++_fenceValue);
			// 拿交换链错误，尝试重建交换链
			if (_frameIndex == UINT32_MAX){
				_reCreateSwapchain();
				continue;
			}

			_commandList->reset();
			_commandList->begin();
			auto backBuffer = _swapchain->getBackBuffer(_frameIndex);
			_commandList->resourceBarrier({ backBuffer, TextureState::Present, TextureState::ColorAttachment });
			onRender(_commandList);
			_commandList->beginLabel("ImGui");
			_commandList->beginRenderPass({{{ .textureView = _backBufferViews.at(_frameIndex) }}});
			_imGuiRenderer->renderDrawData(ImGui::GetDrawData(), _commandList);
			_commandList->endRenderPass();
			_commandList->endLabel();
			_commandList->resourceBarrier({ backBuffer, TextureState::ColorAttachment, TextureState::Present });
			_commandList->end();
			_commandQueue->submit({ _commandList });
			_commandQueue->signal(_fence, ++_fenceValue);
			
			_swapchain->present(_fence, _fenceValue, _fenceValue + 1);
			_fenceValue += 1;

			_frameTotalCount++;
		}
		_commandQueue->waitIdle();

		onDestroy();
		_destroyImGui();
		ImageProcessor::destroySingleton();
		MipMapsGen::destroySingleton();
		StagingBuffer::destroyReadbackGlobal();
		StagingBuffer::destroyUploadGlobal();
	}

	bool WindowApp::mouseButtonPress(int button)
	{
		return glfwGetMouseButton(gWindow, button) == GLFW_PRESS;
	}

	bool WindowApp::mouseButtonRelease(int button)
	{
		return glfwGetMouseButton(gWindow, button) == GLFW_RELEASE;
	}

	bool WindowApp::keyPress(int keycode)
	{
		return glfwGetKey(gWindow, keycode) == GLFW_PRESS;
	}

	std::tuple<std::shared_ptr<Texture>, std::shared_ptr<TextureView>>
		WindowApp::createTextureFormImage(const std::string& file)
	{
		int imageWidth, imageHeight, imageChannels;
		unsigned char* imageData = stbi_load(file.c_str(), &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
		if (!imageData) return std::make_tuple(std::shared_ptr<Texture>(), std::shared_ptr<TextureView>());

		auto texture = _device->createTexture
		({
			.usage = TextureUsage::CopyDst | TextureUsage::Sampled,
			.format = Format::RGBA8Unorm,
			.width = (uint32_t)imageWidth,
			.height = (uint32_t)imageHeight,
			.name = file.c_str()
		});
		StagingBuffer::getUploadGlobal().uploadTexture(texture, imageData, imageWidth * imageHeight * 4);

		return std::make_tuple(texture, texture->createView({}));
	}

	void WindowApp::transitionTextureState(const std::shared_ptr<Texture>& texture, TextureState state)
	{
		auto commandList = _device->createCommandList(CommandListType::General);
		commandList->begin();
		commandList->resourceBarrier({ texture, texture->getState(), state});
		commandList->end();
		auto queue = _device->getCommandQueue(CommandListType::General);
		queue->submit({ commandList });
		queue->waitIdle();
	}

	void WindowApp::onResize()
	{
	}

	void WindowApp::onImGui()
	{
	}

	void WindowApp::onUpdate(float deltaTime)
	{
	}

	void WindowApp::onDestroy()
	{
	}

	void WindowApp::setWindowSize(uint32_t width, uint32_t height)
	{
		glfwSetWindowSize(gWindow, width, height);
	}

	glm::vec2 WindowApp::getCursorPos()
	{
		double x, y;
		glfwGetCursorPos(gWindow, &x, &y);
		return glm::vec2(x, y);
	}

	void WindowApp::_reCreateSwapchain()
	{
		if (_desc.width == 0 || _desc.height == 0)	return;
		
		spdlog::debug("window recreate swapchain");

		SwapchainDesc swapchainDesc =
		{
#if defined(_WIN32)
			.window = glfwGetWin32Window(gWindow),
#elif defined(__linux)
#if defined(USE_XCB)
			.window = (void*)glfwGetX11Window(gWindow),
#else
			.window = glfwGetWaylandWindow(gWindow),
#endif
#endif
			.format = _desc.format,
			.usage = _desc.usage,
			.width = _desc.width,
			.height = _desc.height,
			.frameCount = _frameCount,
			.vsync = _desc.vsync,
			.oldSwapchain = _swapchain
		};
		_swapchain = _device->createSwapchain(swapchainDesc);
		_frameCount = _swapchain->getFrameCount();
		_desc.format = _swapchain->getFormat();

		_backBufferViews.resize(_frameCount);
		for (uint32_t i = 0; i < _frameCount; i++)
		{
			auto texture = _swapchain->getBackBuffer(i);
			_backBufferViews[i] = texture->createView({});
		}

		_lastWidth = _desc.width;
		_lastHeight = _desc.height;
		_imGuiResourceValid = false;

		onResize();
	}

	void WindowApp::_initImGui()
	{
		IMGUI_CHECKVERSION();
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImNodes::CreateContext();
		ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(_dpiScale);
#ifdef _WIN32
		io.Fonts->AddFontFromFileTTF("c:/Windows/Fonts/segoeui.ttf", 16.0f * _dpiScale);
#endif // _WIN32

		ImGui_ImplGlfw_InitForOther(gWindow, true);

		_imGuiRenderer = new ImGuiRenderer();
		_imGuiRenderer->init({ _desc.backend, _device, _desc.format, _frameCount });
	}

	void WindowApp::_renderImGui()
	{
		if (!_imGuiResourceValid)
		{
			_imGuiRenderer->invalidateDeviceObjects();
			_imGuiRenderer->createDeviceObjects();
			_imGuiResourceValid = true;
		}
		
		_imGuiRenderer->newFrame();

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		onImGui();
		ImGui::Render();
	}

	void WindowApp::_destroyImGui()
	{
		_imGuiRenderer->shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImNodes::DestroyContext();
		ImGui::DestroyContext();

		delete _imGuiRenderer;
		_imGuiRenderer = nullptr;
	}
}
