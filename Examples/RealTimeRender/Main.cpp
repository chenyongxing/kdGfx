#include <MathLib.h>
#include <RenderGraph/RenderGraph.h>
#include <RenderGraph/RenderGraphBuilder.h>
#include <RenderGraph/RenderGraphScope.h>
#include <Scene/Project.h>
#include <Scene/CameraControl.h>
#include <WindowApp.h>

#include <imgui/imgui.h>

using namespace kdGfx;

// https://github.com/DGriffin91/bevy_bistro_scene/blob/main/assets/environment_maps/info.txt
class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> cbuff1tex1BindSetLayout;
	std::shared_ptr<BindSetLayout> gBufferBindSetLayout;
	std::shared_ptr<Pipeline> gBufferPipeline;
	std::shared_ptr<BindSet> gBufferBindSet;
	std::shared_ptr<BindSetLayout> lightingBindSetLayout;
	std::shared_ptr<Pipeline> lightingPipeline;
	std::shared_ptr<BindSet> lightingBindSet;
	std::shared_ptr<BindSetLayout> taaBindSetLayout;
	std::shared_ptr<Pipeline> taaPipeline;
	std::shared_ptr<BindSet> taaBindSet;
	std::shared_ptr<Pipeline> toneMappingPipeline;
	std::shared_ptr<BindSet> toneMappingBindSet;

	struct alignas(16) Param
	{
		// GBuffer
		glm::mat4 projection0{ 1.0f };
		glm::mat4 view{ 1.0f };
		glm::mat4 projection{ 1.0f };
		glm::mat4 preView{ 1.0f };
		glm::mat4 preProjection{ 1.0f };
		glm::vec2 jitter{ 0.0f };
		glm::vec2 preJitter{ 0.0f };
		// Lighting
		int haveDirLight = 0;
		glm::vec3 dirLightDir{ 1.0f };
		glm::vec3 viewPos{ 0.0f };
		// ToneMapping
		float gamma = 2.2f;
	};
	Param param;
	std::shared_ptr<Buffer> paramBuffer;

	std::unique_ptr<RenderGraph> renderGraph;
	RenderGraphScope scope;
	RenderGraphResource lastTaa = 0;
	RenderGraphResource nextTaa = 0;

	Camera camera;
	CameraControl cameraControl;

	void defineRenderGraph()
	{
		auto& registry = renderGraph->getRegistry();
		RenderGraphResource paramBuffer = registry.importBuffer(this->paramBuffer, "Param");
		// 指向同一份资源，防止图出现循环
		lastTaa = registry.importTexture(nullptr, nullptr, "LastTAA");
		nextTaa = registry.importTexture(nullptr, nullptr, "NextTAA");

		struct GBufferOut
		{
			RenderGraphResource position;
			RenderGraphResource normal;
			RenderGraphResource baseColor;
			RenderGraphResource velocity;
			RenderGraphResource depth;
		};
		renderGraph->addPass("GBuffer", RenderGraphPassType::FrameRaster, [&](RenderGraphBuilder& builder)
			{
				builder.read(paramBuffer);

				RenderGraphResource outPosition = builder.createTexture({ .format = Format::RGBA16Sfloat, .name = "Position" });
				builder.write(outPosition);
				RenderGraphResource outNormal = builder.createTexture({ .format = Format::RGBA16Sfloat, .name = "Normal" });
				builder.write(outNormal);
				RenderGraphResource outBaseColor = builder.createTexture({ .format = Format::RGBA8Unorm, .name = "BaseColor" });
				builder.write(outBaseColor);
				RenderGraphResource outVelocity = builder.createTexture({ .format = Format::RG32Sfloat, .name = "Velocity" });
				builder.write(outVelocity);
				RenderGraphResource outDepth = builder.createTexture({ .format = Format::D32Sfloat, .name = "Depth" });
				builder.write(outDepth);
				
				scope.add<GBufferOut>({ outPosition, outNormal, outBaseColor, outVelocity, outDepth });

				return [=, this](RenderGraphRegistry& registry, CommandList& commandList)
					{
						auto scene = Project::singleton()->getScene();
						commandList.beginRenderPass
						({
							.colorAttachments =
							{
								{
									.textureView = std::get<1>(registry.getTexture(outPosition)),
									.loadOp = LoadOp::Clear,
									.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }
								},
								{
									.textureView = std::get<1>(registry.getTexture(outNormal)),
									.loadOp = LoadOp::Clear,
									.clearColor = { 0.0f, 0.0f, 0.0f, 0.0f }
								},
								{
									.textureView = std::get<1>(registry.getTexture(outBaseColor)),
									.loadOp = LoadOp::Clear
								},
								{
									.textureView = std::get<1>(registry.getTexture(outVelocity)),
									.loadOp = LoadOp::Clear
								}
							},
							.depthAttachment =
							{
								.textureView = std::get<1>(registry.getTexture(outDepth)),
								.loadOp = LoadOp::Clear,
								.clearDepth = 1.0f
							}
						});
						commandList.setPipeline(gBufferPipeline);
						commandList.setViewport(0, 0, getWidth(), getHeight());
						commandList.setScissor(0, 0, getWidth(), getHeight());
						commandList.setBindSet(0, gBufferBindSet);
						commandList.setVertexBuffer(0, scene->verticesBuffer);
						commandList.setIndexBuffer(scene->indicesBuffer);
						commandList.drawIndexedIndirect(scene->drawCommandsBuffer, scene->drawCommandCount);
						commandList.endRenderPass();
					};
			});

		RenderGraphResource lightResult;
		renderGraph->addPass("Lighting", RenderGraphPassType::FrameRaster, [&](RenderGraphBuilder& builder)
			{
				builder.read(paramBuffer);

				RenderGraphResource inPosition = scope.get<GBufferOut>().position;
				builder.read(inPosition);
				RenderGraphResource inNormal = scope.get<GBufferOut>().normal;
				builder.read(inNormal);
				RenderGraphResource inBaseColor = scope.get<GBufferOut>().baseColor;
				builder.read(inBaseColor);

				lightResult = builder.createTexture({ .format = Format::RGBA16Sfloat, .name = "LightResult" });
				builder.write(lightResult);

				return [=, this](RenderGraphRegistry& registry, CommandList& commandList)
					{
						const auto& inPositionTV = std::get<1>(registry.getTexture(inPosition));
						const auto& inNormalTV = std::get<1>(registry.getTexture(inNormal));
						const auto& inBaseColorTV = std::get<1>(registry.getTexture(inBaseColor));
						UPDATE_TEXTURE_BIND(lightingBindSet, 2, inPositionTV);
						UPDATE_TEXTURE_BIND(lightingBindSet, 3, inNormalTV);
						UPDATE_TEXTURE_BIND(lightingBindSet, 4, inBaseColorTV);
						
						commandList.beginRenderPass({{{ .textureView = std::get<1>(registry.getTexture(lightResult)) }}});
						commandList.setPipeline(lightingPipeline);
						commandList.setBindSet(0, lightingBindSet);
						commandList.setViewport(0, 0, getWidth(), getHeight());
						commandList.setScissor(0, 0, getWidth(), getHeight());
						commandList.draw(6);
						commandList.endRenderPass();
					};
			});

		RenderGraphResource taaResult;
		renderGraph->addPass("TAA", RenderGraphPassType::FrameRaster, [&](RenderGraphBuilder& builder)
			{
				builder.read(lightResult);
				builder.read(lastTaa);
				RenderGraphResource inVelocity = scope.get<GBufferOut>().velocity;
				builder.read(inVelocity);
				RenderGraphResource inDepth = scope.get<GBufferOut>().depth;
				builder.read(inDepth);

				taaResult = builder.createTexture({ .usage = TextureUsage::Sampled, .format = Format::RGBA16Sfloat, .name = "TaaResult" });
				builder.write(taaResult);

				return [=](RenderGraphRegistry& registry, CommandList& commandList)
					{
						const auto& [lightTexture, lightTextureView] = registry.getTexture(lightResult);
						const auto& [temporalTexture, temporalTextureView] = registry.getTexture(lastTaa);
						const auto& inVelocityTV = std::get<1>(registry.getTexture(inVelocity));
						const auto& inDepthTV = std::get<1>(registry.getTexture(inDepth));
						UPDATE_TEXTURE_BIND(taaBindSet, 1, lightTextureView);
						UPDATE_TEXTURE_BIND(taaBindSet, 2, temporalTextureView);
						UPDATE_TEXTURE_BIND(taaBindSet, 3, inVelocityTV);
						UPDATE_TEXTURE_BIND(taaBindSet, 4, inDepthTV);

						const auto& [taaTexture, taaTextureView] = registry.getTexture(taaResult);
						commandList.beginRenderPass({{{ .textureView = taaTextureView }}});
						commandList.setPipeline(taaPipeline);
						struct PushConstant
						{
							int32_t ignoreTemporaly;
							int32_t simpleMode;
							glm::vec2 screenSize;
						} param;
						param.ignoreTemporaly = (_frameTotalCount == 0);
						param.simpleMode = true;
						param.screenSize = { getWidth(), getHeight() };
						commandList.setPushConstant(&param);
						commandList.setBindSet(0, taaBindSet);
						commandList.setViewport(0, 0, getWidth(), getHeight());
						commandList.setScissor(0, 0, getWidth(), getHeight());
						commandList.draw(6);
						commandList.endRenderPass();
					};
			});

		renderGraph->addPass("StoreTAA", RenderGraphPassType::Copy, [&](RenderGraphBuilder& builder)
			{
				builder.read(taaResult);
				builder.write(nextTaa);

				return [=](RenderGraphRegistry& registry, CommandList& commandList)
					{
						const auto& taaTexture = std::get<0>(registry.getTexture(taaResult));
						const auto& temporalTexture = std::get<0>(registry.getTexture(nextTaa));
						commandList.copyTexture(taaTexture, temporalTexture, { getWidth(), getHeight() });
					};
			}, true);

		renderGraph->addPass("ToneMapping", RenderGraphPassType::FrameRaster, [&](RenderGraphBuilder& builder)
			{
				builder.read(paramBuffer);
				builder.read(taaResult);

				RenderGraphResource backBuffer = builder.importTexture(nullptr, nullptr, "BackBuffer");
				builder.write(backBuffer);

				return [=](RenderGraphRegistry& registry, CommandList& commandList)
					{
						const auto& taaResultTV = std::get<1>(registry.getTexture(taaResult));
						UPDATE_TEXTURE_BIND(toneMappingBindSet, 2, taaResultTV);
						
						const auto& [__, backBufferView] = getBackBuffer(_frameIndex);
						commandList.beginRenderPass({{{ .textureView = backBufferView }}});
						commandList.setPipeline(toneMappingPipeline);
						commandList.setBindSet(0, toneMappingBindSet);
						commandList.setViewport(0, 0, getWidth(), getHeight());
						commandList.setScissor(0, 0, getWidth(), getHeight());
						commandList.draw(6);
						commandList.endRenderPass();
					};
			}, true);
	}

	void setupScene(const std::string& path)
	{
		Project::singleton()->init(_device);
		Project::singleton()->open(path);

		auto scene = Project::singleton()->getScene();
		Project::singleton()->eventTower.addEventListener(EventAssetsUpload,
			[this, scene](std::shared_ptr<EventData> data)
			{
				gBufferBindSet->bindBuffer(3, scene->materialsBuffer);

				std::vector<std::shared_ptr<TextureView>> textureViews(scene->images.size());
				for (uint32_t i = 0; i < scene->images.size(); i++)
				{
					textureViews[i] = scene->images[i]->textureView;
				}
				if (!textureViews.empty())	gBufferBindSet->bindTextures(4, textureViews);
			});
		Project::singleton()->eventTower.addEventListener(EventNodesChanged,
			[this, scene](std::shared_ptr<EventData> data)
			{
				gBufferBindSet->bindBuffer(2, scene->instancesBuffer);

				if (!scene->cameras.empty())
				{
					camera = *scene->cameras[0];
				}
			});
	}

	void onSetup() override
	{
		cbuff1tex1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::ConstantBuffer },
			{ .binding = 1, .type = BindEntryType::Sampler },
			{ .binding = 2, .type = BindEntryType::SampledTexture }
		});
		gBufferBindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::ConstantBuffer },
			{ .binding = 1, .type = BindEntryType::Sampler },
			{ .binding = 2, .shaderRegister = 0, .type = BindEntryType::ReadedBuffer },
			{ .binding = 3, .shaderRegister = 1, .type = BindEntryType::ReadedBuffer },
			{ .binding = 4, .shaderRegister = 2, .type = BindEntryType::SampledTexture, .count = UINT32_MAX },
		});
		lightingBindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::ConstantBuffer },
			{ .binding = 1, .type = BindEntryType::Sampler },
			{ .binding = 2, .shaderRegister = 0, .type = BindEntryType::SampledTexture },
			{ .binding = 3, .shaderRegister = 1, .type = BindEntryType::SampledTexture },
			{ .binding = 4, .shaderRegister = 2, .type = BindEntryType::SampledTexture }
		});
		taaBindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::Sampler },
			{ .binding = 1, .shaderRegister = 0, .type = BindEntryType::SampledTexture },
			{ .binding = 2, .shaderRegister = 1, .type = BindEntryType::SampledTexture },
			{ .binding = 3, .shaderRegister = 2, .type = BindEntryType::SampledTexture },
			{ .binding = 4, .shaderRegister = 3, .type = BindEntryType::SampledTexture },
		});

		{
			LOAD_SHADER("GBuffer.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("GBuffer.ps", psShader, _psCode, _psFilePath);
			gBufferPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { gBufferBindSetLayout },
				.vertexAttributes =
				{
					{ .location = 0, .semantic = "POSITION", .offset = offsetof(SubMesh::Vertex, position), .format = Format::RGB32Sfloat },
					{ .location = 1, .semantic = "NORMAL", .offset = offsetof(SubMesh::Vertex, normal), .format = Format::RGB32Sfloat },
					{ .location = 2, .semantic = "TEXCOORD", .offset = offsetof(SubMesh::Vertex, texCoord), .format = Format::RG32Sfloat }
				},
				.colorFormats = { Format::RGBA16Sfloat, Format::RGBA16Sfloat, Format::RGBA8Unorm, Format::RG32Sfloat },
				.depthStencilFormat = Format::D32Sfloat,
				.depthTest = CompareOp::Less
			});
		}
		{
			LOAD_SHADER("Lighting.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Lighting.ps", psShader, _psCode, _psFilePath);
			lightingPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { lightingBindSetLayout },
				.colorFormats = { Format::RGBA16Sfloat }
			});
		}
		{
			LOAD_SHADER("TAA.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("TAA.ps", psShader, _psCode, _psFilePath);
			taaPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { .size = sizeof(float) * 4 },
				.bindSetLayouts = { taaBindSetLayout },
				.colorFormats = { Format::RGBA16Sfloat }
			});
		}
		{
			LOAD_SHADER("ToneMapping.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("ToneMapping.ps", psShader, _psCode, _psFilePath);
			toneMappingPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { cbuff1tex1BindSetLayout },
				.colorFormats = { _swapchain->getFormat() }
			});
		}

		paramBuffer = _device->createBuffer
		({
			.size = sizeof(Param),
			.usage = BufferUsage::Constant,
			.hostVisible = true,
			.name = "GBufferParam"
		});

		gBufferBindSet = _device->createBindSet(gBufferBindSetLayout);
		gBufferBindSet->bindBuffer(0, paramBuffer);
		gBufferBindSet->bindSampler(1, _anisotropicRepeatSampler);

		lightingBindSet = _device->createBindSet(lightingBindSetLayout);
		lightingBindSet->bindBuffer(0, paramBuffer);
		lightingBindSet->bindSampler(1, _nearestClampSampler);

		taaBindSet = _device->createBindSet(taaBindSetLayout);
		taaBindSet->bindSampler(0, _linerClampSampler);

		toneMappingBindSet = _device->createBindSet(cbuff1tex1BindSetLayout);
		toneMappingBindSet->bindBuffer(0, paramBuffer);
		toneMappingBindSet->bindSampler(1, _nearestClampSampler);

		renderGraph = std::make_unique<RenderGraph>(_device);
		renderGraph->name = "RealTimeRender";
		defineRenderGraph();
		onResize();
		renderGraph->genDotFile("RenderGraph.dot", false);
		renderGraph->compile();

		camera.lookAt(glm::vec3(0.f, 0.f, 5.f), glm::vec3(0.f));
		cameraControl.setCamera(&camera);

		setupScene("Toy_Rocketship");
		//setupScene("Bistro");
	}

	void onResize() override
	{
		if (renderGraph)
		{
			renderGraph->resize(getWidth(), getHeight());

			{
				auto texture = _device->createTexture
				({
					.usage = TextureUsage::CopyDst | TextureUsage::Sampled,
					.format = Format::RGBA16Sfloat,
					.width = getWidth(),
					.height = getHeight(),
					.name = "TemporaTaaResult"
				});
				transitionTextureState(texture, TextureState::ShaderRead);
				auto textureView = texture->createView({});
				renderGraph->getRegistry().setImportedTexture(lastTaa, texture, textureView);
				renderGraph->getRegistry().setImportedTexture(nextTaa, texture, textureView);
			}
		}
	}

	void onImGui() override
	{
		float dpiScale = getDpiScale();
		ImGuiIO& io = ImGui::GetIO();
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
		ImGui::SetNextWindowPos(ImVec2(15.f * dpiScale, 15.f * dpiScale));
		ImGui::SetNextWindowBgAlpha(0.2f);
		static bool open = true;
		if (ImGui::Begin("Status", &open, windowFlags))
		{
			ImGui::Text("%.0f FPS (%.2f ms)", ImGui::GetIO().Framerate, ImGui::GetIO().DeltaTime * 1000.f);
		}
		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(15.f * dpiScale, 60.f * dpiScale));
		ImGui::Begin("Params");
		auto scene = Project::singleton()->getScene();
		ImGui::SeparatorText("Camera");
		{
			static int cameraIndex = 0;
			std::vector<std::string> cameraNames(scene->cameras.size());
			std::vector<const char*> cameraNamesCStr(cameraNames.size());
			for (uint32_t i = 0; i < cameraNames.size(); i++)
			{
				std::string name = scene->cameras[i]->name;
				cameraNames[i] = name.empty() ? std::to_string(i) : name;
				cameraNamesCStr[i] = cameraNames[i].c_str();
			}
			if (!cameraNames.empty())
			{
				if (ImGui::Combo("Filter", &cameraIndex, cameraNamesCStr.data(), cameraNamesCStr.size()))
				{
					camera = *scene->cameras[cameraIndex];
				}
			}
		}
		ImGui::Text("Position: %.2f, %.2f, %.2f", param.viewPos.x, param.viewPos.y, param.viewPos.z);
		ImGui::DragFloat("Fov", &camera.fovY, 1.f, 1.f, 179.f);
		ImGui::DragFloat("NearZ", &camera.nearZ, 0.01f, 0.01f, 9.99f);
		ImGui::DragFloat("FarZ", &camera.farZ, 1.f, 1.f, 9999.f);
		ImGui::SeparatorText("PostImage");
		ImGui::DragFloat("Gamma", &param.gamma, 0.01f, 0.f, 10.f);
		ImGui::End();

		renderGraph->drawImNodes(dpiScale);
	}

	void onUpdate(float deltaTime) override
	{
		Project::singleton()->getScene()->update(deltaTime);

		if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) &&
			!ImGui::IsAnyItemHovered())
		{
			cameraControl.ResetState();
			auto cursorPos = getCursorPos();
			cameraControl.MousePositonUpdate(cursorPos.x, cursorPos.y);
			if (keyPress('E'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveUp);
			if (keyPress('Q'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveDown);
			if (keyPress('A'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveLeft);
			if (keyPress('D'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveRight);
			if (keyPress('W'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveForward);
			if (keyPress('S'))
				cameraControl.KeyboardControlUpdate(CameraControl::MoveBackward);

			if (mouseButtonPress(0))
				cameraControl.MouseButtonUpdate(CameraControl::MouseButtonLeft);
			if (mouseButtonPress(1))
				cameraControl.MouseButtonUpdate(CameraControl::MouseButtonRight);
			if (mouseButtonPress(2))
				cameraControl.MouseButtonUpdate(CameraControl::MouseButtonMiddle);
			cameraControl.update(deltaTime);
		}
		
		camera.aspect = float(getWidth()) / float(getHeight());
		// 摄像机亚像素内随机抖动
		glm::vec2 samplePoint = Halton23Array8()[_frameTotalCount % 8];
		auto jitter = (samplePoint * 2.0f - 1.0f) / glm::vec2(getWidth(), getHeight());

		param.projection0 = camera.getProjectionMatrix();
		param.projection0[2][0] += jitter.x;
		param.projection0[2][1] += jitter.y;
		if (_frameTotalCount == 0)
		{
			param.view = camera.getViewMatrix();
			param.projection = camera.getProjectionMatrix();
			param.jitter = camera.jitter;
			param.preView = param.view;
			param.preProjection = param.projection;
			param.preJitter = param.jitter;
		}
		else
		{
			param.preView = param.view;
			param.preProjection = param.projection;
			param.preJitter = param.jitter;
			param.view = camera.getViewMatrix();
			param.projection = camera.getProjectionMatrix();
			param.jitter = camera.jitter;
		}
		param.viewPos = camera.getWorldPosition();
		param.haveDirLight = true;
		param.dirLightDir = glm::normalize(param.dirLightDir);
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		memcpy(paramBuffer->map(), &param, sizeof(Param));
		renderGraph->setCommandList(commandList);
		renderGraph->execute();
	}
	void onDestroy() override
	{
		Project::singleton()->close();
		Project::singleton()->deinit();
		renderGraph.reset();
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
