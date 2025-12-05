#include "WindowApp.h"
#include "StagingBuffer.h"

#include <imgui/imgui.h>

using namespace kdGfx;

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> yuyvPipeline;

	uint32_t rgbaWidth = 0;
	uint32_t rgbaHeight = 0;
	std::shared_ptr<BindSet> rgbaBindSet;
	std::shared_ptr<Texture> rgbaTexture;
	std::shared_ptr<TextureView> rgbaTextureView;

	uint32_t yuyvWidth = 0;
	uint32_t yuyvHeight = 0;
	std::shared_ptr<BindSet> yuyvBindSet;
	std::shared_ptr<Texture> yuyvTexture;
	std::shared_ptr<TextureView> yuyvTextureView;

	void onSetup() override
	{
		tex1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::Sampler },
			{ .binding = 1, .type = BindEntryType::SampledTexture }
		});

		{
			LOAD_SHADER("Image.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Image.ps", psShader, _psCode, _psFilePath);
			imagePipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() }
			});
		}

		{
			LOAD_SHADER("YUYV.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("YUYV.ps", psShader, _psCode, _psFilePath);
			yuyvPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { Format::RGBA8Unorm }
			});
		}

		std::tie(rgbaTexture, rgbaTextureView) = createTextureFormImage("wallpaper.jpg");
		assert(rgbaTexture);
		rgbaWidth = rgbaTexture->getWidth();
		rgbaHeight = rgbaTexture->getHeight();
		rgbaBindSet = _device->createBindSet(tex1BindSetLayout);
		rgbaBindSet->bindSampler(0, _nearestClampSampler);
		rgbaBindSet->bindTexture(1, rgbaTextureView);

		yuyvWidth = rgbaWidth / 2;
		yuyvHeight = rgbaHeight;
		yuyvTexture = _device->createTexture
		({
			.usage = TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::CopySrc,
			.format = Format::RGBA8Unorm,
			.width = yuyvWidth,
			.height = yuyvHeight,
		});
		transitionTextureState(yuyvTexture, TextureState::ShaderRead);
		yuyvTextureView = yuyvTexture->createView({});

		yuyvBindSet = _device->createBindSet(tex1BindSetLayout);
		yuyvBindSet->bindSampler(0, _nearestClampSampler);
		yuyvBindSet->bindTexture(1, yuyvTextureView);
	}

	void onImGui() override
	{
		if (ImGui::Button("Save")){
			auto imageDataSize = rgbaWidth * rgbaHeight * 2;
			std::vector<uint8_t> imageData(imageDataSize);
			StagingBuffer::getReadbackGlobal().readbackTexture(yuyvTexture, imageData.data(), imageData.size());
			{
				std::string file_name = fmt::format("output_{}x{}.yuv", rgbaWidth, rgbaHeight);
				std::ofstream file(file_name, std::ios::binary);
				assert(file);
				file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
				assert(file);
				file.close();
			}
			{
				std::string file_name = fmt::format("output_{}x{}.rgb", yuyvWidth, yuyvHeight);
				std::ofstream file(file_name, std::ios::binary);
				assert(file);
				file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
				assert(file);
				file.close();
			}
		}
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		auto [backBuffer, backBufferView] = getBackBuffer(_frameIndex);
		
		// Convert RGBA to YUYV
		commandList->beginLabel("Convert YUYV");
		commandList->resourceBarrier({ yuyvTexture, TextureState::ShaderRead, TextureState::ColorAttachment });
		commandList->beginRenderPass
		({ { {
				.textureView = yuyvTextureView,
				.loadOp = LoadOp::DontCare,
		} } });
		commandList->setPipeline(yuyvPipeline);
		commandList->setBindSet(0, rgbaBindSet);
		commandList->setViewport(0, 0, yuyvWidth, yuyvHeight);
		commandList->setScissor(0, 0, yuyvWidth, yuyvHeight);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->resourceBarrier({ yuyvTexture, TextureState::ColorAttachment, TextureState::ShaderRead });
		commandList->endLabel();

		// Draw yuyv
		commandList->beginLabel("Draw YUYV");
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, yuyvBindSet);
		commandList->setViewport(0, 0, yuyvWidth, yuyvHeight);
		commandList->setScissor(0, 0, yuyvWidth, yuyvHeight);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->endLabel();
	}
};

int main(int argc, char* argv[])
{
	spdlog::set_level(spdlog::level::trace);

	WindowAppDesc windowDesc =
	{
#ifdef _WIN32
		//.backend = BackendType::DirectX12,
#endif
#ifdef _DEBUG
		.debug = true,
#endif
		.width = 1280,
		.height = 720,
		.format = Format::RGBA8Unorm,
		.usage = TextureUsage::ColorAttachment | TextureUsage::CopySrc,
	};
	windowDesc.parseArgs(argc, argv);

	MainWindow mainWindow;
	mainWindow.init(windowDesc);
	mainWindow.mainLoop();

	return 0;
}
