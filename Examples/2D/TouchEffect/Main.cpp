#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
struct TouchPoint
{
	glm::vec2 point;
	TimePoint time;
};

float distSq(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return dx * dx + dy * dy;
}

// https://psimpl.sourceforge.net/radial-distance.html
void pathSimplify(std::vector<TouchPoint>& path, float tolerance)
{
	std::vector<TouchPoint> out;
	if (path.size() < 2)    return;

	out.push_back(path.front());

	const TouchPoint* point = &path.front();
	const TouchPoint* prevPoint = &path.front();
	for (int i = 1; i < path.size(); i++)
	{
		point = &path[i];
		if (distSq(point->point.x, point->point.y, prevPoint->point.x, prevPoint->point.y) > tolerance * tolerance) {
			out.push_back(*point);
			prevPoint = point;
		}
	}
	if (fabsf(point->point.x - prevPoint->point.x) > 0.01f && fabsf(point->point.y - prevPoint->point.y) > 0.01f)
	{
		out.push_back(*point);
	}

	path = out;
}

// https://www.educative.io/answers/what-is-chaikins-algorithm
void pathSmooth(const std::vector<TouchPoint>& path, std::vector<TouchPoint>& out)
{
	if (path.size() < 3)    return;

	out.clear();
	out.reserve(path.size() * 2);

	out.push_back(path.front());

	for (int i = 0; i < path.size() - 1; i++)
	{
		const TouchPoint& p0 = path[i];
		const TouchPoint& p1 = path[i + 1];

		glm::vec2 q = 0.75f * p0.point + 0.25f * p1.point;
		glm::vec2 r = 0.25f * p0.point + 0.75f * p1.point;

		// 时间点插值
		auto duration = p1.time - p0.time;
		auto qTime = p0.time + std::chrono::duration_cast<std::chrono::nanoseconds>(0.25f * duration);
		auto rTime = p0.time + std::chrono::duration_cast<std::chrono::nanoseconds>(0.75f * duration);

		out.push_back({ q, qTime });
		out.push_back({ r, rTime });
	}

	out.push_back(path.back());
}

void swipePathTriangulate(const std::vector<TouchPoint>& path, float minThickness, float maxThickness, std::vector<glm::vec2>& vertices)
{
	vertices.clear();
	if (path.size() < 2)    return;

	vertices.push_back(path.front().point);

	for (int i = 0; i < path.size() - 1; i++)
	{
		// 细->粗
		float thick = std::max(minThickness, maxThickness * i / (float)path.size());

		const glm::vec2& p0 = path[i].point;
		const glm::vec2& p1 = path[i + 1].point;
		auto dir = glm::normalize(p1 - p0);
		glm::vec2 perp(-dir.y, dir.x);
		glm::vec2 v = perp * (thick * 0.5f);
		glm::vec2 p2 = p1 - v;
		glm::vec2 p3 = p1 + v;

		vertices.push_back(p2);
		vertices.push_back(p3);
	}

	// 头端向外扩展
	const glm::vec2& p0 = path[path.size() - 1].point;
	const glm::vec2& p1 = path[path.size() - 2].point;
	auto dir = glm::normalize(p0 - p1);
	vertices.push_back(p0 + dir * maxThickness);
}

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> swipePipeline;
	std::shared_ptr<Pipeline> circlePipeline;
	std::shared_ptr<Pipeline> arcPipeline;

	std::shared_ptr<Texture> wallpaperTexture;
	std::shared_ptr<TextureView> wallpaperTextureView;
	std::shared_ptr<BindSet> wallpaperBindSet;

	// Swipe
	std::vector<TouchPoint> path;
	std::vector<glm::vec2> vertices;
	float simpleThreshold = 35.f; // 轨迹线简化的阈值
	int smoothIterations = 2; // 平滑迭代次数
	float pointLiftTime = 0.2f; // 轨迹点停留时间
	std::shared_ptr<Buffer> swipeVerticesBuffer;
	struct SwipeParam
	{
		glm::vec2 screenSize{ 0.f };
		glm::vec2 _pad0{ 0.f };
		glm::vec4 color{ 1.f, 1.f, 1.f, 0.6f };
	};
	SwipeParam swipeParam;

	// Circle
	std::shared_ptr<Buffer> circleVerticesBuffer;
	EasingAnimation<float> outScaleAnimation;
	EasingAnimation<float> outOpacityAnimation;
	struct CircleParam
	{
		glm::vec2 screenSize{ 0.f };
		glm::vec2 leftTopPos{ 0.f };
		glm::vec2 rectSize{ 0.f };
		float radius = 0.5f;
		float attenuation = 0.4f;
	};
	CircleParam circleParam;

	// Arc
	bool isPress = false;
	TimePoint pressTime;
	EasingAnimation<float> pressAnimation;
	struct ArcParam
	{
		glm::vec2 screenSize{ 0.f };
		glm::vec2 leftTopPos{ 0.f };
		glm::vec2 rectSize{ 0.f };
		float endAngle = 0.0f;
	};
	ArcParam arcParam;

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
			LOAD_SHADER("Swipe.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Swipe.ps", psShader, _psCode, _psFilePath);
			swipePipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(SwipeParam), 0, 0 },
				.topology = Topology::TriangleStrip,
				.vertexAttributes =
				{
					{.location = 0, .semantic = "POSITION", .format = Format::RG32Sfloat }
				},
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}
		{
			LOAD_SHADER("Circle.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Circle.ps", psShader, _psCode, _psFilePath);
			circlePipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(CircleParam), 0, 0 },
				.vertexAttributes =
				{
					{.location = 0, .semantic = "POSITION", .format = Format::RG32Sfloat }
				},
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}
		{
			LOAD_SHADER("Arc.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("Arc.ps", psShader, _psCode, _psFilePath);
			arcPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(ArcParam), 0, 0 },
				.vertexAttributes =
				{
					{.location = 0, .semantic = "POSITION", .format = Format::RG32Sfloat }
				},
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}

		std::tie(wallpaperTexture, wallpaperTextureView) = createTextureFormImage("wallpaper.jpg");
		assert(wallpaperTexture);
		wallpaperBindSet = _device->createBindSet(tex1BindSetLayout);
		wallpaperBindSet->bindSampler(0, _linerRepeatSampler);
		wallpaperBindSet->bindTexture(1, wallpaperTextureView);

		// big buffer for swipe
		swipeVerticesBuffer = _device->createBuffer
		({
			.size = 1024 * 1024,
			.usage = BufferUsage::Vertex,
			.hostVisible = true
		});
		circleVerticesBuffer = _device->createBuffer
		({
			.size = sizeof(float) * 2 * 6,
			.usage = BufferUsage::Vertex,
			.hostVisible = true
		});

		// animate
		outScaleAnimation.start = 0.5f;
		outScaleAnimation.target = 0.3f;
		outScaleAnimation.duration = 0.5f;
		outScaleAnimation.easeFunction = [](float x) -> float
			{
				return 1.f - powf(1.f - x, 4.f);
			};
		outScaleAnimation.endCallback = [this]()
			{
				this->circleParam.radius = 0.f;
			};

		outOpacityAnimation.start = 0.4f;
		outOpacityAnimation.target = 0.8f;
		outOpacityAnimation.duration = outScaleAnimation.duration;
		outOpacityAnimation.easeFunction = [](float x) -> float
			{
				return 1.f - powf(1.f - x, 4.f);
			};

		pressAnimation.start = 0.0f;
		pressAnimation.target = glm::radians(360.f);
		pressAnimation.duration = 1.0f;
		pressAnimation.easeFunction = [](float x) -> float
			{
				return x;
			};
	}

	void onImGui() override
	{
		ImGui::Text("mouse click -> circle effect");
		ImGui::Text("ctrl + mouse click -> ripple effect");
		ImGui::Text("mouse long press -> ring progress");
		ImGui::Text("mouse press drag -> swipe path");
		ImGui::Separator();
		ImGui::DragFloat("Simple Threshold", &simpleThreshold);
		ImGui::DragInt("Smooth Iterations", &smoothIterations, 1, 0, 10);
		ImGui::Separator();
		ImGui::DragFloat("radius", &circleParam.radius, 0.01f);
		ImGui::DragFloat("attenuation", &circleParam.attenuation, 0.01f);
		ImGui::DragFloat("endAngle", &arcParam.endAngle, 0.01f);
	}

	void onUpdate(float deltaTime) override
	{
		float width = getWidth();
		float height = getHeight();
		swipeParam.screenSize = { width, height };
		circleParam.screenSize = { width, height };
		arcParam.screenSize = { width, height };

		static bool isPress = false;
		if (mouseButtonPress())
		{
			if (!isPress)
			{
				onTouchPress();
				isPress = true;
			}
			else
			{
				onTouchUpdate(deltaTime);
			}
		}
		if (mouseButtonRelease())
		{
			if (isPress)
			{
				onTouchRelease();
				isPress = false;
			}
		}

		// animate
		outScaleAnimation.frame();
		if (outScaleAnimation.isRunning)	circleParam.radius = outScaleAnimation.value;
		outOpacityAnimation.frame();
		if (outOpacityAnimation.isRunning)	circleParam.attenuation = outOpacityAnimation.value;
		pressAnimation.frame();
		if (pressAnimation.isRunning)	arcParam.endAngle = pressAnimation.value;
	}

	void onTouchPress()
	{
		auto cursorPos = getCursorPos();
		float mousePosX = cursorPos.x;
		float mousePosY = cursorPos.y;

		const float halfSize = 50.f;
		std::vector<glm::vec2> vtx =
		{
			{ mousePosX - halfSize, mousePosY - halfSize },
			{ mousePosX + halfSize, mousePosY - halfSize },
			{ mousePosX + halfSize, mousePosY + halfSize },
			{ mousePosX - halfSize, mousePosY - halfSize },
			{ mousePosX + halfSize, mousePosY + halfSize },
			{ mousePosX - halfSize, mousePosY + halfSize },
		};
		memcpy(circleVerticesBuffer->map(), vtx.data(), sizeof(glm::vec2) * vtx.size());

		circleParam.leftTopPos = { mousePosX - halfSize, mousePosY - halfSize };
		circleParam.rectSize = { halfSize * 2.f, halfSize * 2.f };
		arcParam.leftTopPos = circleParam.leftTopPos;
		arcParam.rectSize = circleParam.rectSize;

		isPress = true;
		pressTime = std::chrono::steady_clock::now();
		arcParam.endAngle = 0.0f;
	}

	void onTouchUpdate(float deltaTime)
	{
		// 点生存时间
		for (auto it = path.begin(); it != path.end();)
		{
			auto currentTime = std::chrono::steady_clock::now();
			float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - it->time).count();
			if (elapsedTime > pointLiftTime * 1000.f)
			{
				it = path.erase(it);
			}
			else
			{
				++it;
			}
		}

		path.push_back({ getCursorPos(), std::chrono::steady_clock::now() });

		pathSimplify(path, simpleThreshold);

		std::vector<TouchPoint> smoothedPath = path;
		for (int i = 0; i < smoothIterations; i++)
		{
			std::vector<TouchPoint> tmpPath;
			pathSmooth(smoothedPath, tmpPath);
			smoothedPath = tmpPath;
		}

		swipePathTriangulate(smoothedPath, 4.f, 12.f, vertices);

		memcpy(swipeVerticesBuffer->map(), vertices.data(), sizeof(glm::vec2) * vertices.size());

		// Arc progress bar
		if (isPress)
		{
			auto currentTime = std::chrono::steady_clock::now();
			float elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - pressTime).count();
			if (elapsedTime > 500.0f && !pressAnimation.isRunning)
			{
				pressAnimation.run();
			}
		}
	}

	void onTouchRelease()
	{
		path.clear();
		vertices.clear();

		outScaleAnimation.run();
		outOpacityAnimation.run();

		isPress = false;
		pressAnimation.stop();
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearValue = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		commandList->setViewport(0, 0, getWidth(), getHeight());
		commandList->setScissor(0, 0, getWidth(), getHeight());

		// Draw wallpaper
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, wallpaperBindSet);
		commandList->draw(3);

		// Draw Swipe
		if (vertices.size() > 0)
		{
			commandList->setPipeline(swipePipeline);
			commandList->setPushConstant(&swipeParam);
			commandList->setVertexBuffer(0, swipeVerticesBuffer);
			commandList->draw(vertices.size());
		}

		if (!pressAnimation.isRunning)
		{
			// Draw Circle
			commandList->setPipeline(circlePipeline);
			commandList->setPushConstant(&circleParam);
			commandList->setVertexBuffer(0, circleVerticesBuffer);
			commandList->draw(6);
		}
		else
		{
			// Draw Arc progress bar
			commandList->setPipeline(arcPipeline);
			commandList->setPushConstant(&arcParam);
			commandList->setVertexBuffer(0, circleVerticesBuffer);
			commandList->draw(6);
		}

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
