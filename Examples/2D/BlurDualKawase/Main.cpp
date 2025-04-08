#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

struct Param
{
	glm::vec2 iResolution;
	glm::vec2 texelSize;
	float offset = 2.0f;
};

class BlurSubregionPass final
{
public:
	WindowApp* windowApp = nullptr;
	std::shared_ptr<Device> device;
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> dualKawaseBlurDownPipeline;
	std::shared_ptr<Pipeline> dualKawaseBlurUpPipeline;
	std::shared_ptr<Sampler> linerClampSampler;
	std::shared_ptr<Sampler> linerRepeatSampler;

	std::shared_ptr<Texture> image;
	std::shared_ptr<TextureView> imageView;
	Format imageFormat = Format::Undefined;
	glm::ivec4 imageRegion{ 0 };

	int iteration = 2;
	float offset = 2.0f;

	bool dirty = true;

private:
	// 模糊中间过程贴图
	struct BlurIterTexture
	{
		std::shared_ptr<Texture> texture;
		std::shared_ptr<TextureView> textureView;
		std::shared_ptr<BindSet> bindSet;
	};
	std::vector<BlurIterTexture> blurIterTextures;

	// 用来画最终模糊结果图
	std::shared_ptr<BindSet> bluredImageBindSet;

	void reset()
	{
		blurIterTextures.clear();

		// 比迭代次数多一张。第一张原始大小图用来从复制被模糊原图，同时是最后模糊结果
		for (int i = 0; i < iteration + 1; i++)
		{
			auto texture = device->createTexture
			({
				.usage = TextureUsage::ColorAttachment | TextureUsage::Sampled |
					(i == 0 ? TextureUsage::CopyDst : TextureUsage::Undefined),
				.format = imageFormat,
				.width = imageRegion.z / (uint32_t)pow(2, i),
				.height = imageRegion.w / (uint32_t)pow(2, i),
				.name = fmt::format("blurTexture{}", i)
				});
			windowApp->transitionTextureState(texture, TextureState::ShaderRead);
			auto textureView = texture->createView({});

			auto bindSet = device->createBindSet(tex1BindSetLayout);
			bindSet->bindSampler(0, linerClampSampler);
			bindSet->bindTexture(1, textureView);
			blurIterTextures.push_back({ texture, textureView, bindSet });
		}

		bluredImageBindSet->bindSampler(0, linerRepeatSampler);
		bluredImageBindSet->bindTexture(1, blurIterTextures[0].textureView);
	}

public:
	void onSetup()
	{
		bluredImageBindSet = device->createBindSet(tex1BindSetLayout);
	}

	void setRegion(glm::ivec4 region)
	{
		if (this->imageRegion == region)	return;

		this->imageRegion = region;
		dirty = true;
	}

	void setIteration(int iteration)
	{
		if (this->iteration == iteration)	return;

		this->iteration = iteration;
		dirty = true;
	}

	void onRender(const std::shared_ptr<CommandList>& commandList)
	{
		if (dirty)
		{
			reset();
			dirty = false;
		}

		Param param;
		param.offset = offset;

		//Copy background subregion
		commandList->beginLabel("Copy background subregion");
		commandList->resourceBarrier({ image, TextureState::ColorAttachment, TextureState::CopySrc });
		commandList->resourceBarrier({ blurIterTextures[0].texture, TextureState::ShaderRead, TextureState::CopyDst });
		commandList->copyTexture(image, blurIterTextures[0].texture, 
			{ imageRegion.z, imageRegion.w }, { imageRegion.x, imageRegion.y }, { 0, 0 });
		commandList->resourceBarrier({ blurIterTextures[0].texture, TextureState::CopyDst, TextureState::ShaderRead });
		commandList->resourceBarrier({ image, TextureState::CopySrc, TextureState::ColorAttachment });
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
			commandList->resourceBarrier({ renderTarget.texture, renderTarget.texture->getState(), TextureState::ColorAttachment });
			commandList->resourceBarrier({ drawTexture.texture, drawTexture.texture->getState(), TextureState::ShaderRead });
			commandList->beginRenderPass({ {{.textureView = renderTarget.textureView }} });
			commandList->setPipeline(dualKawaseBlurDownPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(3);
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
			commandList->beginRenderPass({ {{.textureView = renderTarget.textureView }} });
			commandList->setPipeline(dualKawaseBlurUpPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(3);
			commandList->endRenderPass();
			commandList->endLabel();
		}

		//Draw blured image
		commandList->beginLabel("Draw blured image");
		commandList->resourceBarrier({ blurIterTextures[0].texture, blurIterTextures[0].texture->getState(), TextureState::ShaderRead });
		commandList->beginRenderPass({ {{.textureView = imageView }} });
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, bluredImageBindSet);
		commandList->setViewport(imageRegion.x, imageRegion.y, imageRegion.z, imageRegion.w);
		commandList->setScissor(imageRegion.x, imageRegion.y, imageRegion.z, imageRegion.w);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->endLabel();
	}
};

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

	int iteration = 2;
	float offset = 2.0f;

	glm::ivec4 subregion0{ 100, 100, 800, 600 };
	std::shared_ptr<BlurSubregionPass> blurSubregionPass0;

	glm::ivec4 subregion1{ 200, 200, 200, 150 };
	std::shared_ptr<BlurSubregionPass> blurSubregionPass1;

	bool drawSubregion1 = true;

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


		blurSubregionPass0 = std::make_shared<BlurSubregionPass>();
		blurSubregionPass0->windowApp = this;
		blurSubregionPass0->device = _device;
		blurSubregionPass0->tex1BindSetLayout = tex1BindSetLayout;
		blurSubregionPass0->imagePipeline = imagePipeline;
		blurSubregionPass0->dualKawaseBlurDownPipeline = dualKawaseBlurDownPipeline;
		blurSubregionPass0->dualKawaseBlurUpPipeline = dualKawaseBlurUpPipeline;
		blurSubregionPass0->linerClampSampler = _linerClampSampler;
		blurSubregionPass0->linerRepeatSampler = _linerRepeatSampler;
		blurSubregionPass0->onSetup();

		blurSubregionPass1 = std::make_shared<BlurSubregionPass>();
		blurSubregionPass1->windowApp = this;
		blurSubregionPass1->device = _device;
		blurSubregionPass1->tex1BindSetLayout = tex1BindSetLayout;
		blurSubregionPass1->imagePipeline = imagePipeline;
		blurSubregionPass1->dualKawaseBlurDownPipeline = dualKawaseBlurDownPipeline;
		blurSubregionPass1->dualKawaseBlurUpPipeline = dualKawaseBlurUpPipeline;
		blurSubregionPass1->linerClampSampler = _linerClampSampler;
		blurSubregionPass1->linerRepeatSampler = _linerRepeatSampler;
		blurSubregionPass1->onSetup();
	}

	void onImGui() override
	{
		ImGui::DragInt4("subRegion0", glm::value_ptr(subregion0));
		ImGui::DragInt4("subRegion1", glm::value_ptr(subregion1));
		ImGui::Checkbox("subRegion1 blur", &drawSubregion1);
		ImGui::Separator();
		ImGui::InputInt("Iteration", &iteration);
		iteration = glm::clamp(iteration, 1, 10);
		ImGui::DragFloat("Offset", &offset, 0.1f, 0.f, 100.f);
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		auto [backBuffer, backBufferView] = getBackBuffer(_frameIndex);
		
		// Draw background
		commandList->beginLabel("Draw background");
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, backgroundBindSet);
		commandList->setViewport(0, 0, backgroundWidth, backgroundHeight);
		commandList->setScissor(0, 0, backgroundWidth, backgroundHeight);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->endLabel();

		// draw blured image
		blurSubregionPass0->image = backBuffer;
		blurSubregionPass0->imageView = backBufferView;
		blurSubregionPass0->imageFormat = _swapchain->getFormat();
		blurSubregionPass0->offset = offset;
		blurSubregionPass0->setRegion(subregion0);
		blurSubregionPass0->setIteration(iteration);
		blurSubregionPass0->onRender(commandList);

		// clear subregion1 to image
		commandList->beginRenderPass({ {{.textureView = backBufferView }} });
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, backgroundBindSet);
		commandList->setViewport(0, 0, backgroundWidth, backgroundHeight);
		commandList->setScissor(subregion1.x, subregion1.y, subregion1.z, subregion1.w);
		commandList->draw(3);
		commandList->endRenderPass();

		// draw subregion1 blured image
		if (drawSubregion1)
		{
			blurSubregionPass1->image = backBuffer;
			blurSubregionPass1->imageView = backBufferView;
			blurSubregionPass1->imageFormat = _swapchain->getFormat();
			blurSubregionPass1->offset = offset;
			blurSubregionPass1->setRegion(subregion1);
			blurSubregionPass1->setIteration(iteration);
			blurSubregionPass1->onRender(commandList);
		}
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
		.height = 720
	};
	windowDesc.parseArgs(argc, argv);

	MainWindow mainWindow;
	mainWindow.init(windowDesc);
	mainWindow.mainLoop();

	return 0;
}
