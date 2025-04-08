#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> dualKawaseBlurDownPipeline;
	std::shared_ptr<Pipeline> dualKawaseBlurUpPipeline;

	uint32_t backgroundWidth = 0;
	uint32_t backgroundHeight = 0;
	std::shared_ptr<BindSet> backgroundBindSet;
	std::shared_ptr<Texture> backgroundTexture;
	std::shared_ptr<TextureView> backgroundTextureView;
	
	int32_t imageX = 100;
	int32_t imageY = 100;
	uint32_t imageWidth = 800;
	uint32_t imageHeight = 600;
	struct BlurIterTexture
	{
		std::shared_ptr<Texture> texture;
		std::shared_ptr<TextureView> textureView;
		std::shared_ptr<BindSet> bindSet;
	};
	std::vector<BlurIterTexture> blurIterTextures;
	// 用来画最终模糊结果图
	std::shared_ptr<BindSet> bluredImageBindSet;

	struct Param
	{
		glm::vec2 iResolution;
		glm::vec2 texelSize;
		float offset = 0.0f;
	};

	int iteration = 2;
	float offset = 2.0f;
	int lastIteration = -1;

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
			LOAD_SHADER("DualKawaseBlurDown.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("DualKawaseBlurDown.ps", psShader, _psCode, _psFilePath);
			dualKawaseBlurDownPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(Param), 0, 0 },
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() }
			});
		}

		{
			LOAD_SHADER("DualKawaseBlurUp.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("DualKawaseBlurUp.ps", psShader, _psCode, _psFilePath);
			dualKawaseBlurUpPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(Param), 0, 0 },
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() }
			});
		}

		std::tie(backgroundTexture, backgroundTextureView) = createTextureFormImage("BlurDualKawase.png");
		assert(backgroundTexture);
		backgroundWidth = backgroundTexture->getWidth();
		backgroundHeight = backgroundTexture->getHeight();
		backgroundBindSet = _device->createBindSet(tex1BindSetLayout);
		backgroundBindSet->bindSampler(0, _linerRepeatSampler);
		backgroundBindSet->bindTexture(1, backgroundTextureView);

		bluredImageBindSet = _device->createBindSet(tex1BindSetLayout);
	}

	void onImGui() override
	{
		ImGui::DragInt("X", &imageX, 1.f, 0, backgroundWidth - imageWidth);
		ImGui::DragInt("Y", &imageY, 1.f, 0, backgroundHeight - imageHeight);
		ImGui::Text("Width: %d", imageWidth);
		ImGui::Text("Height: %d", imageHeight);
		ImGui::Separator();
		ImGui::InputInt("Iteration", &iteration);
		iteration = glm::clamp(iteration, 1, 10);
		ImGui::DragFloat("Offset", &offset, 0.1f, 0.f, 100.f);
	}

	void initBlurIterTextures()
	{
		blurIterTextures.clear();

		// 比迭代次数多一张。第一张原始大小图用来从复制被模糊原图，同时是最后模糊结果
		for (int i = 0; i < iteration + 1; i++)
		{
			auto texture = _device->createTexture
			({
				.usage = TextureUsage::ColorAttachment | TextureUsage::Sampled | 
					(i == 0 ? TextureUsage::CopyDst : TextureUsage::Undefined),
				.format = _swapchain->getFormat(), // 和后缓冲格式一致否则从后缓冲copy会有问题
				.width = imageWidth / (uint32_t)pow(2, i),
				.height = imageHeight / (uint32_t)pow(2, i),
				.name = fmt::format("blurTexture{}", i)
			});
			transitionTextureState(texture, TextureState::ShaderRead);
			auto textureView = texture->createView({});

			auto bindSet = _device->createBindSet(tex1BindSetLayout);
			bindSet->bindSampler(0, _linerClampSampler);
			bindSet->bindTexture(1, textureView);
			blurIterTextures.push_back({ texture, textureView, bindSet });
		}

		bluredImageBindSet->bindSampler(0, _linerRepeatSampler);
		bluredImageBindSet->bindTexture(1, blurIterTextures[0].textureView);
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		if (lastIteration != iteration)
		{
			initBlurIterTextures();
			lastIteration = iteration;
		}

		Param param;
		param.offset = offset;

		auto [backBuffer, backBufferView] = getBackBuffer(_frameIndex);
		
		// Draw background
		commandList->beginLabel("Draw background");
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, backgroundBindSet);
		commandList->setViewport(0, 0, backgroundWidth, backgroundHeight);
		commandList->setScissor(0, 0, backgroundWidth, backgroundHeight);
		commandList->draw(6);
		commandList->endRenderPass();
		commandList->endLabel();
		
		//Copy background subregion
		commandList->beginLabel("Copy background subregion");
		commandList->resourceBarrier({ backBuffer, TextureState::ColorAttachment, TextureState::CopySrc });
		commandList->resourceBarrier({ blurIterTextures[0].texture, TextureState::ShaderRead, TextureState::CopyDst});
		commandList->copyTexture(backBuffer, blurIterTextures[0].texture, { imageWidth, imageHeight }, { imageX, imageY }, { 0, 0 } );
		commandList->resourceBarrier({ blurIterTextures[0].texture, TextureState::CopyDst, TextureState::ShaderRead });
		commandList->resourceBarrier({ backBuffer, TextureState::CopySrc, TextureState::ColorAttachment });
		commandList->endLabel();

		//Downsample
		for (int i = 0; i < iteration; i++)
		{
			auto& renderTarget = blurIterTextures[i + 1];
			auto& drawTexture = blurIterTextures[i];
			uint32_t width = renderTarget.texture->getWidth();
			uint32_t height = renderTarget.texture->getHeight();

			param.iResolution = { width, height };
			param.texelSize = { 1.0f / param.iResolution.x, 1.0f / param.iResolution.y };
			
			commandList->beginLabel(fmt::format("Downsample {}", i));
			commandList->resourceBarrier({ renderTarget.texture, renderTarget.texture->getState(), TextureState::ColorAttachment});
			commandList->resourceBarrier({ drawTexture.texture, drawTexture.texture->getState(), TextureState::ShaderRead });
			commandList->beginRenderPass({{{ .textureView = renderTarget.textureView }}});
			commandList->setPipeline(dualKawaseBlurDownPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(6);
			commandList->endRenderPass();
			commandList->endLabel();
		}

		//Upsample
		for (int i = iteration; i > 0; i--)
		{
			auto& renderTarget = blurIterTextures[i - 1];
			auto& drawTexture = blurIterTextures[i];
			uint32_t width = renderTarget.texture->getWidth();
			uint32_t height = renderTarget.texture->getHeight();

			param.iResolution = { width, height };
			param.texelSize = { 1.0f / param.iResolution.x, 1.0f / param.iResolution.y };
			
			commandList->beginLabel(fmt::format("Upsample {}", i));
			commandList->resourceBarrier({ renderTarget.texture, renderTarget.texture->getState(), TextureState::ColorAttachment });
			commandList->resourceBarrier({ drawTexture.texture, drawTexture.texture->getState(), TextureState::ShaderRead });
			commandList->beginRenderPass({{{ .textureView = renderTarget.textureView }}});
			commandList->setPipeline(dualKawaseBlurUpPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(6);
			commandList->endRenderPass();
			commandList->endLabel();
		}

		//Draw blured image
		commandList->beginLabel("Draw blured image");
		commandList->resourceBarrier({ blurIterTextures[0].texture, blurIterTextures[0].texture->getState(), TextureState::ShaderRead });
		commandList->beginRenderPass({{{ .textureView = backBufferView }}});
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, bluredImageBindSet);
		commandList->setViewport(imageX, imageY, imageWidth, imageHeight);
		commandList->setScissor(imageX, imageY, imageWidth, imageHeight);
		commandList->draw(6);
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
