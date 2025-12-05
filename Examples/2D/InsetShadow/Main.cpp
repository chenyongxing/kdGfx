#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

constexpr double M_PI = 3.14159265358979323846;

static inline std::vector<float> generateGaussianKernel(float sigma, int& radius)
{
	// 计算核的半径（通常取3σ）
	radius = static_cast<int>(std::ceil(3.0f * sigma));
	int size = 2 * radius + 1;

	std::vector<float> kernel(size);
	float sum = 0.0f;
	float sigma_sq = sigma * sigma;
	float scale = 1.0f / (std::sqrt(2 * M_PI) * sigma);

	// 计算高斯权重
	for (int i = 0; i < size; ++i)
	{
		int x = i - radius;
		kernel[i] = scale * std::exp(-(x * x) / (2 * sigma_sq));
		sum += kernel[i];
	}

	// 归一化
	for (float& weight : kernel) {
		weight /= sum;
	}

	return kernel;
}

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<BindSetLayout> tex2BindSetLayout;
	std::shared_ptr<BindSetLayout> buf1tex2BindSetLayout;

	// 提取图片透明区域并填充颜色
	std::shared_ptr<Pipeline> colorPipeline;
	std::shared_ptr<BindSet> colorBindSet;
	std::shared_ptr<Pipeline> gaussianBlurPipeline;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<BindSet> gaussianBindSet;
	std::shared_ptr<Pipeline> shadowPipeline;
	std::shared_ptr<BindSet> shadowBindSet;
	std::shared_ptr<BindSet> imageBindSet;
	std::shared_ptr<BindSet> imageShadowBindSet;

	std::shared_ptr<Texture> imageTexture;
	std::shared_ptr<TextureView> imageTextureView;
	std::shared_ptr<Texture> colorOutput;
	std::shared_ptr<TextureView> colorOutputView;
	std::shared_ptr<Texture> gaussianOutput;
	std::shared_ptr<TextureView> gaussianOutputView;
	std::shared_ptr<Texture> shadowOutput;
	std::shared_ptr<TextureView> shadowOutputView;

	int gaussianRadius = 0;
	std::vector<float> gaussianKernel;
	std::shared_ptr<Buffer> gaussianBuffer;
	
	glm::vec4 color{ 1.0f, 0.0f, 0.0f, 1.0f };
	float blurRadius = 30.f;

	void reCreateGaussian()
	{
		gaussianKernel = std::move(generateGaussianKernel(blurRadius * 0.5f, gaussianRadius));
		gaussianBuffer = _device->createBuffer
		({
			.size = gaussianKernel.size() * sizeof(float),
			.stride = sizeof(float),
			.usage = BufferUsage::Readed,
			.hostVisible = HostVisible::Upload,
			.name = "gaussianKernel"
		});
		memcpy(gaussianBuffer->map(), gaussianKernel.data(), gaussianKernel.size() * sizeof(float));
		gaussianBindSet->bindBuffer(0, gaussianBuffer);

		colorOutput = _device->createTexture
		({
			.usage = TextureUsage::Sampled | TextureUsage::ColorAttachment,
			.format = Format::RGBA8Unorm,
			.width = imageTexture->getWidth() + gaussianRadius * 2,
			.height = imageTexture->getHeight() + gaussianRadius * 2,
			.name = "colorOutput"
		});
		transitionTextureState(colorOutput, TextureState::ShaderRead);
		colorOutputView = colorOutput->createView({});
		colorBindSet->bindTexture(1, imageTextureView);

		gaussianOutput = _device->createTexture
		({
			.usage = TextureUsage::Sampled | TextureUsage::Storage,
			.format = Format::RGBA8Unorm,
			.width = colorOutput->getWidth(),
			.height = colorOutput->getHeight(),
			.name = "gaussianOutput"
		});
		transitionTextureState(gaussianOutput, TextureState::ShaderRead);
		gaussianOutputView = gaussianOutput->createView({});
		gaussianBindSet->bindTexture(1, colorOutputView);
		gaussianBindSet->bindTexture(2, gaussianOutputView);

		shadowBindSet->bindTexture(1, gaussianOutputView);
	}

	void onSetup() override
	{
		tex1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::Sampler },
			{ .binding = 1, .type = BindEntryType::SampledTexture }
		});

		tex2BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .shaderRegister = 0, .type = BindEntryType::Sampler },
			{ .binding = 1, .shaderRegister = 0, .type = BindEntryType::SampledTexture },
			{ .binding = 2, .shaderRegister = 1, .type = BindEntryType::SampledTexture }
		});

		buf1tex2BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .shaderRegister = 0, .type = BindEntryType::ReadedBuffer },
			{ .binding = 1, .shaderRegister = 1, .type = BindEntryType::SampledTexture },
			{ .binding = 2, .shaderRegister = 0, .type = BindEntryType::StorageTexture }
		});

		{
			LOAD_SHADER("Image.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Image.ps", psShader, _psCode, _psFilePath);
			imagePipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}

		{
			LOAD_SHADER("Color.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Color.ps", psShader, _psCode, _psFilePath);
			colorPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 4 },
				.bindSetLayouts = { tex1BindSetLayout },
				.colorFormats = { Format::RGBA8Unorm }
			});
		}

		{
			LOAD_SHADER("GaussianBlur.cs", shader, _code, _filePath);
			gaussianBlurPipeline = _device->createComputePipeline
			({
				.shader = shader,
				.pushConstantLayout = { sizeof(float) },
				.bindSetLayouts = { buf1tex2BindSetLayout },
			});
		}

		{
			LOAD_SHADER("Shadow.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Shadow.ps", psShader, _psCode, _psFilePath);
			shadowPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 4 },
				.bindSetLayouts = { tex2BindSetLayout },
				.colorFormats = { Format::RGBA8Unorm }
			});
		}

		std::tie(imageTexture, imageTextureView) = createTextureFormImage("star.png");
		assert(imageTexture);
		imageBindSet = _device->createBindSet(tex1BindSetLayout);
		imageBindSet->bindSampler(0, _nearestClampSampler);
		imageBindSet->bindTexture(1, imageTextureView);

		colorBindSet = _device->createBindSet(tex1BindSetLayout);
		colorBindSet->bindSampler(0, _nearestClampSampler);

		gaussianBindSet = _device->createBindSet(buf1tex2BindSetLayout);
		
		shadowOutput = _device->createTexture
		({
			.usage = TextureUsage::Sampled | TextureUsage::ColorAttachment,
			.format = Format::RGBA8Unorm,
			.width = imageTexture->getWidth(),
			.height = imageTexture->getHeight(),
			.name = "shadowOutput"
		});
		transitionTextureState(shadowOutput, TextureState::ShaderRead);
		shadowOutputView = shadowOutput->createView({});
		shadowBindSet = _device->createBindSet(tex2BindSetLayout);
		shadowBindSet->bindSampler(0, _nearestClampSampler);
		shadowBindSet->bindTexture(2, imageTextureView);

		imageShadowBindSet = _device->createBindSet(tex1BindSetLayout);
		imageShadowBindSet->bindSampler(0, _nearestClampSampler);
		imageShadowBindSet->bindTexture(1, shadowOutputView);

		reCreateGaussian();
	}

	void onImGui() override
	{
		ImGui::ColorEdit4("color", glm::value_ptr(color));
		if (ImGui::DragFloat("blurRadius", &blurRadius, 1.0f, 0.0f, 60.f))
		{
			reCreateGaussian();
		}
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		// 模糊目标，提取原图反向区域+大小扩大模糊半径
		commandList->resourceBarrier({ colorOutput, TextureState::ShaderRead, TextureState::ColorAttachment });
		commandList->beginRenderPass
		({ { {
				.textureView = colorOutputView,
				.loadOp = LoadOp::Clear,
				.clearValue = { color.r, color.g, color.b, color.a }
		} } });
		commandList->setViewport(gaussianRadius, gaussianRadius, imageTexture->getWidth(), imageTexture->getHeight());
		commandList->setScissor(gaussianRadius, gaussianRadius, imageTexture->getWidth(), imageTexture->getHeight());
		commandList->setPipeline(colorPipeline);
		commandList->setBindSet(0, colorBindSet);
		commandList->setPushConstant(&color);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->resourceBarrier({ colorOutput, TextureState::ColorAttachment, TextureState::ShaderRead });

		// 高斯模糊
		const uint32_t workgroupWidth = 16;
		const uint32_t workgroupHeight = 16;
		commandList->resourceBarrier({ gaussianOutput, TextureState::ShaderRead, TextureState::General });
		commandList->setPipeline(gaussianBlurPipeline);
		commandList->setBindSet(0, gaussianBindSet);
		commandList->setPushConstant(&gaussianRadius);
		uint32_t xGroups = (gaussianOutput->getWidth() + workgroupWidth - 1) / workgroupWidth;
		uint32_t yGroups = (gaussianOutput->getHeight() + workgroupHeight - 1) / workgroupHeight;
		commandList->dispatch(xGroups, yGroups, 1);
		commandList->resourceBarrier({ gaussianOutput, TextureState::General, TextureState::ShaderRead });

		// 裁剪模糊结果，只保留阴影
		commandList->resourceBarrier({ shadowOutput, TextureState::ShaderRead, TextureState::ColorAttachment });
		commandList->beginRenderPass
		({ { {
				.textureView = shadowOutputView,
				.loadOp = LoadOp::Clear
		} } });
		commandList->setViewport(0, 0, shadowOutput->getWidth(), shadowOutput->getHeight());
		commandList->setScissor(0, 0, shadowOutput->getWidth(), shadowOutput->getHeight());
		commandList->setPipeline(shadowPipeline);
		commandList->setBindSet(0, shadowBindSet);
		glm::vec4 shadowRegion = 
		{
			gaussianRadius / (float)gaussianOutput->getWidth(),
			gaussianRadius / (float)gaussianOutput->getHeight(),
			(gaussianOutput->getWidth() - gaussianRadius) / (float)gaussianOutput->getWidth(),
			(gaussianOutput->getHeight() - gaussianRadius) / (float)gaussianOutput->getHeight(),
		};
		commandList->setPushConstant(&shadowRegion);
		commandList->draw(3);
		commandList->endRenderPass();
		commandList->resourceBarrier({ shadowOutput, TextureState::ColorAttachment, TextureState::ShaderRead });

		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearValue = { 1.0f, 1.0f, 1.0f, 1.0f }
		} } });
		commandList->setViewport(100, 100, imageTexture->getWidth(), imageTexture->getHeight());
		commandList->setScissor(100, 100, imageTexture->getWidth(), imageTexture->getHeight());
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, imageBindSet);
		commandList->draw(3);
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, imageShadowBindSet);
		commandList->draw(3);
		commandList->endRenderPass();
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
		.width = 800,
		.height = 600
	};
	windowDesc.parseArgs(argc, argv);

	MainWindow mainWindow;
	mainWindow.init(windowDesc);
	mainWindow.mainLoop();

	return 0;
}
