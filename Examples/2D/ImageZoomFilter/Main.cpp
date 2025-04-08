#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

enum FilterType
{
	Filter_Nearest,
	Filter_Liner,
	Filter_FSR,
	Filter_Lanczos
};

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> bindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> FSRPipeline;
	std::shared_ptr<Pipeline> lanczosPipeline;

	uint32_t imageWidth = 0;
	uint32_t imageHeight = 0;
	std::shared_ptr<Texture> imageTexture;
	std::shared_ptr<TextureView> imageTextureView;
	std::shared_ptr<BindSet> imageBindSet;
	std::shared_ptr<BindSet> imageLinerBindSet;
	
	struct Param
	{
		glm::vec2 iResolution;
		glm::vec2 renderSize;
	};
	Param param;
	FilterType filter = Filter_Nearest;
	float scale = 3.0f;

	void onSetup() override
	{
		bindSetLayout = _device->createBindSetLayout
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
				.bindSetLayouts = { bindSetLayout },
				.colorFormats = { _desc.format }
			});
		}
		{
			LOAD_SHADER("FSR.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("FSR.ps", psShader, _psCode, _psFilePath);
			FSRPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 4, 0, 0 },
				.bindSetLayouts = { bindSetLayout },
				.colorFormats = { _desc.format }
			});
		}
		{
			LOAD_SHADER("Lanczos.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Lanczos.ps", psShader, _psCode, _psFilePath);
			lanczosPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 4, 0, 0 },
				.bindSetLayouts = { bindSetLayout },
				.colorFormats = { _desc.format }
			});
		}

		std::tie(imageTexture, imageTextureView) = createTextureFormImage("Image.png");
		assert(imageTexture);
		imageWidth = imageTexture->getWidth();
		imageHeight = imageTexture->getHeight();
		imageBindSet = _device->createBindSet(bindSetLayout);
		imageBindSet->bindSampler(0, _nearestClampSampler);
		imageBindSet->bindTexture(1, imageTextureView);
		imageLinerBindSet = _device->createBindSet(bindSetLayout);
		imageLinerBindSet->bindSampler(0, _linerClampSampler);
		imageLinerBindSet->bindTexture(1, imageTextureView);
	}

	void onImGui() override
	{
		param.iResolution = { imageWidth, imageHeight };
		param.renderSize = { imageWidth * scale, imageHeight * scale };

		const char* filterItems[] = { "Nearest", "Liner", "FSR", "Lanczos" };
		ImGui::Combo("Filter", (int*)&filter, filterItems, IM_ARRAYSIZE(filterItems));

		ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.f);
		ImGui::SameLine();
		if (ImGui::Button("Reset"))
		{
			scale = 1.f;
		}
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setViewport(0, 0, param.renderSize.x, param.renderSize.y);
		commandList->setScissor(0, 0, param.renderSize.x, param.renderSize.y);
		switch (filter)
		{
		case Filter_Liner:
			commandList->setPipeline(imagePipeline);
			commandList->setBindSet(0, imageLinerBindSet);
			break;
		case Filter_FSR:
			commandList->setPipeline(FSRPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, imageBindSet);
			break;
		case Filter_Lanczos:
			commandList->setPipeline(lanczosPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, imageBindSet);
			break;
		default:
			commandList->setPipeline(imagePipeline);
			commandList->setBindSet(0, imageBindSet);
			break;
		}
		commandList->draw(6);
		commandList->endRenderPass();
	}
};

int main(int argc, char* argv[])
{
	spdlog::set_level(spdlog::level::trace);

	WindowAppDesc windowDesc =
	{
#ifdef _WIN32
		.backend = BackendType::DirectX12,
#endif
#ifdef _DEBUG
		.debug = true,
#endif
		.width = 1280,
		.height = 720
	};
	windowDesc.parseArgs(argc, argv);

	MainWindow mainWindow;
	mainWindow.init(windowDesc);
	mainWindow.mainLoop();

	return 0;
}
