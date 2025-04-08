#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> cbuf1BindSetLayout;
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> decoPipeline;

	std::shared_ptr<Texture> wallpaperTexture;
	std::shared_ptr<TextureView> wallpaperTextureView;
	std::shared_ptr<BindSet> wallpaperBindSet;

	std::shared_ptr<Buffer> paramBuffer;
	std::shared_ptr<BindSet> paramBindSet;

	struct alignas(16) Param
	{
		glm::vec4 shadowColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		glm::vec4 windowRoundRadius{ 0.0370370373f };
		glm::vec4 windowTitleColor{ 1.0f };
		glm::vec2 shadowTopLeft{};
		glm::vec2 shadowBottomRight{};
		glm::vec2 windowOffset{};
		glm::vec2 windowSize{};
		float aspect = 1.0f;
		float shadowRadius = 0.0f;
		float shadowSigma = 20.0f;
		float windowTitleHeight = 0.0370370373f;
		glm::vec3 windowBorderColor{ 1.0f };
		float windowBorderThickness = 0.00740740728f;
	};
	Param param;
	glm::vec4 windowRect{ 800.f, 200.f, 600.f, 400.f };
	glm::vec2 shadowOffset{ 0.0f, 0.0f };

	void onSetup() override
	{
		cbuf1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::ConstantBuffer}
		});

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
			LOAD_SHADER("Decoration.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Decoration.ps", psShader, _psCode, _psFilePath);
			decoPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { cbuf1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}

		std::tie(wallpaperTexture, wallpaperTextureView) = createTextureFormImage("wallpaper.jpg");
		assert(wallpaperTexture);
		wallpaperBindSet = _device->createBindSet(tex1BindSetLayout);
		wallpaperBindSet->bindSampler(0, _linerRepeatSampler);
		wallpaperBindSet->bindTexture(1, wallpaperTextureView);

		paramBuffer = _device->createBuffer
		({
			.size = sizeof(Param),
			.usage = BufferUsage::Constant,
			.hostVisible = true,
			.name = "param"
		});
		paramBindSet = _device->createBindSet(cbuf1BindSetLayout);
		paramBindSet->bindBuffer(0, paramBuffer);
	}

	void onImGui() override
	{
		float width = getWidth();
		float height = getHeight();

		param.aspect = float(width) / float(height);

		ImGui::DragFloat2("window pos", glm::value_ptr(windowRect));
		ImGui::DragFloat2("window size", &windowRect.z);
		ImGui::Separator();

		ImGui::DragFloat2("shadow offset", glm::value_ptr(shadowOffset));
		param.shadowTopLeft = glm::vec2(windowRect.x, windowRect.y) + shadowOffset;
		param.shadowBottomRight = param.shadowTopLeft + glm::vec2(windowRect.z, windowRect.w);
		ImGui::ColorEdit4("shadow color", glm::value_ptr(param.shadowColor));
		ImGui::DragFloat("sigma", &param.shadowSigma, 1.f, 0.01f, 1000.f);

		float coeff = 2.0 * sqrt(2.0 * log(2.0));
		ImGui::Text("shadow width: %f", coeff * param.shadowSigma);
		ImGui::Separator();

		glm::vec4 radiusPixel = param.windowRoundRadius * float(height) * 0.5f;
		ImGui::DragFloat4("radius", glm::value_ptr(radiusPixel), 1.f, 0.0f, 1000.f);
		// * 2
		param.windowRoundRadius = radiusPixel / float(height) * 2.0f;

		float titleHeightPixel = param.windowTitleHeight * float(height);
		ImGui::DragFloat("title height", &titleHeightPixel, 1.f, 0.0f, 1000.f);
		param.windowTitleHeight = titleHeightPixel / float(height);

		ImGui::ColorEdit4("title color", glm::value_ptr(param.windowTitleColor));

		float borderPixel = param.windowBorderThickness * float(height) * 0.5f;
		ImGui::DragFloat("border", &borderPixel, 0.1f, 0.0f, 100.f);
		// * 2
		param.windowBorderThickness = borderPixel / float(height) * 2.0f;

		ImGui::ColorEdit3("border color", glm::value_ptr(param.windowBorderColor));

		glm::vec2 sizeDist = glm::vec2(windowRect.z, windowRect.w) / float(height);
		param.windowSize = sizeDist;
		param.windowOffset = sizeDist * 0.5f + glm::vec2(windowRect.x, windowRect.y) / float(height);
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		memcpy(paramBuffer->map(), &param, sizeof(Param));

		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setViewport(0, 0, getWidth(), getHeight());
		commandList->setScissor(0, 0, getWidth(), getHeight());
		// Draw wallpaper
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, wallpaperBindSet);
		commandList->draw(6);
		// Draw decoration
		commandList->setPipeline(decoPipeline);
		commandList->setBindSet(0, paramBindSet);
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
