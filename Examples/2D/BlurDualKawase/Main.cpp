#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

struct Param
{
	glm::vec2 iResolution;
	glm::vec2 texelSize;
	glm::vec4 texUVClamp;
	float offset = 2.0f;
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

	int iteration = 4;
	float offset = 2.0f;

	// 从原图提取的子区域进行模糊
	glm::ivec4 subregion = { 201, 101, 401, 301 };

	// 模糊中间过程贴图
	struct BlurIterCacheItem
	{
		std::shared_ptr<Buffer> vertexBuffer;
		std::shared_ptr<Texture> texture;
		std::shared_ptr<TextureView> textureView;
		std::shared_ptr<BindSet> bindSet;
	};
	std::vector<BlurIterCacheItem> blurIterCache;
	bool blurIterCacheDirty = true;

	void recreateCacheTextures()
	{
		if (!blurIterCacheDirty)	return;
		blurIterCacheDirty = false;

		blurIterCache.clear();

		size_t vertexBufferSize = sizeof(float) * 2 * 6;

		// 渲染时候更新第0张贴图为后备缓冲区
		blurIterCache.push_back({
			.vertexBuffer = _device->createBuffer
			({
				.size = vertexBufferSize,
				.usage = BufferUsage::Vertex,
				.hostVisible = HostVisible::Upload,
				.name = "vertexBuffer0"
			}),
			.bindSet = _device->createBindSet(tex1BindSetLayout)
		});

		for (uint32_t i = 1; i < iteration + 1; i++)
		{
			auto vertexBuffer = _device->createBuffer
			({
				.size = vertexBufferSize,
				.usage = BufferUsage::Vertex,
				.hostVisible = HostVisible::Upload,
				.name = fmt::format("vertexBuffer{}", i)
			});

			auto texture = _device->createTexture
			({
				.usage = TextureUsage::ColorAttachment | TextureUsage::Sampled | TextureUsage::CopyDst,
				.format = _swapchain->getFormat(),
				.width = getWidth() / (uint32_t)pow(2, i),
				.height = getHeight() / (uint32_t)pow(2, i),
				.name = fmt::format("blurTexture{}", i)
			});
			transitionTextureState(texture, TextureState::ShaderRead);
			auto textureView = texture->createView({});

			auto bindSet = _device->createBindSet(tex1BindSetLayout);
			bindSet->bindSampler(0, _nearestClampSampler);
			bindSet->bindTexture(1, textureView);
			blurIterCache.push_back({ vertexBuffer, texture, textureView, bindSet });
		}
	}

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
				.vertexAttributes ={{ .location = 0, .semantic = "POSITION", .format = Format::RG32Sfloat }},
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
				.vertexAttributes = {{.location = 0, .semantic = "POSITION", .format = Format::RG32Sfloat }},
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
	}

	void onImGui() override
	{
		ImGui::DragInt4("Sub Region", glm::value_ptr(subregion));
		ImGui::Separator();
		if (ImGui::InputInt("Iteration", &iteration)) {
			blurIterCacheDirty = true;
		}
		iteration = glm::clamp(iteration, 1, 10);
		ImGui::DragFloat("Offset", &offset, 0.1f, 0.f, 100.f);

		recreateCacheTextures();
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		// vertices upload gpu
		for (int i = 0; i < iteration + 1; i++)
		{
			int32_t iterScale = pow(2, i);

			auto& renderTarget = blurIterCache[i];
			glm::vec2* pos = (glm::vec2*)renderTarget.vertexBuffer->map();

			glm::ivec4 subregionIter;
			subregionIter.x = subregion.x / iterScale;
			subregionIter.y = subregion.y / iterScale;
			subregionIter.z = subregion.z / iterScale;
			subregionIter.w = subregion.w / iterScale;

			pos[0] = glm::vec2(subregionIter.x, subregionIter.y);
			pos[1] = glm::vec2(subregionIter.x + subregionIter.z, subregionIter.y);
			pos[2] = glm::vec2(subregionIter.x + subregionIter.z, subregionIter.y + subregionIter.w);
			pos[3] = glm::vec2(subregionIter.x, subregionIter.y);
			pos[4] = glm::vec2(subregionIter.x + subregionIter.z, subregionIter.y + subregionIter.w);
			pos[5] = glm::vec2(subregionIter.x, subregionIter.y + subregionIter.w);
		}

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

		// Blur
		blurIterCache[0].texture = backBuffer;
		blurIterCache[0].textureView = backBufferView;
		blurIterCache[0].bindSet->bindSampler(0, _nearestClampSampler);
		blurIterCache[0].bindSet->bindTexture(1, backBufferView);

		// Blur Downsample
		for (int i = 0; i < iteration; i++)
		{
			auto& renderTarget = blurIterCache[i + 1];
			auto& drawTexture = blurIterCache[i];
			uint32_t width = renderTarget.texture->getWidth();
			uint32_t height = renderTarget.texture->getHeight();

			Param param;
			param.offset = offset;
			param.iResolution = { width, height };
			param.texelSize = { 0.5f / param.iResolution.x, 0.5f / param.iResolution.y };
			{
				float width = drawTexture.texture->getWidth();
				float height = drawTexture.texture->getHeight();

				float iterScale = pow(2, i);
				glm::ivec4 subregionIter;
				subregionIter.x = subregion.x / iterScale;
				subregionIter.y = subregion.y / iterScale;
				subregionIter.z = subregion.z / iterScale;
				subregionIter.w = subregion.w / iterScale;
				param.texUVClamp =
				{
					subregionIter.x / width,
					subregionIter.y / height,
					(subregionIter.x + subregionIter.z) / width,
					(subregionIter.y + subregionIter.w) / height
				};
			}

			commandList->beginLabel(fmt::format("Downsample {}", i));
			commandList->resourceBarrier({ renderTarget.texture, renderTarget.texture->getState(), TextureState::ColorAttachment });
			commandList->resourceBarrier({ drawTexture.texture, drawTexture.texture->getState(), TextureState::ShaderRead });
			commandList->beginRenderPass({ {{.textureView = renderTarget.textureView }} });
			commandList->setPipeline(dualKawaseBlurDownPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setVertexBuffer(0, renderTarget.vertexBuffer);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(6);
			commandList->endRenderPass();
			commandList->endLabel();
		}

		// Blur Upsample
		for (int i = iteration; i > 0; i--)
		{
			auto& renderTarget = blurIterCache[i - 1];
			auto& drawTexture = blurIterCache[i];
			uint32_t width = renderTarget.texture->getWidth();
			uint32_t height = renderTarget.texture->getHeight();

			Param param;
			param.offset = offset;
			param.iResolution = { width, height };
			param.texelSize = { 0.5f / param.iResolution.x, 0.5f / param.iResolution.y };
			{
				float width = drawTexture.texture->getWidth();
				float height = drawTexture.texture->getHeight();

				float iterScale = pow(2, i);
				glm::ivec4 subregionIter;
				subregionIter.x = subregion.x / iterScale;
				subregionIter.y = subregion.y / iterScale;
				subregionIter.z = subregion.z / iterScale;
				subregionIter.w = subregion.w / iterScale;
				param.texUVClamp =
				{
					subregionIter.x / width,
					subregionIter.y / height,
					(subregionIter.x + subregionIter.z) / width,
					(subregionIter.y + subregionIter.w) / height
				};
			}

			commandList->beginLabel(fmt::format("Upsample {}", i - 1));
			commandList->resourceBarrier({ renderTarget.texture, renderTarget.texture->getState(), TextureState::ColorAttachment });
			commandList->resourceBarrier({ drawTexture.texture, drawTexture.texture->getState(), TextureState::ShaderRead });
			commandList->beginRenderPass({ {{.textureView = renderTarget.textureView }} });
			commandList->setPipeline(dualKawaseBlurUpPipeline);
			commandList->setPushConstant(&param);
			commandList->setBindSet(0, drawTexture.bindSet);
			commandList->setVertexBuffer(0, renderTarget.vertexBuffer);
			commandList->setViewport(0, 0, width, height);
			commandList->setScissor(0, 0, width, height);
			commandList->draw(6);
			commandList->endRenderPass();
			commandList->endLabel();
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
		.height = 720,
		.usage = TextureUsage::ColorAttachment | TextureUsage::Sampled,
	};
	windowDesc.parseArgs(argc, argv);

	MainWindow mainWindow;
	mainWindow.init(windowDesc);
	mainWindow.mainLoop();

	return 0;
}
