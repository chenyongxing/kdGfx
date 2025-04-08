#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

struct Rect
{
	float x;
	float y;
	float width;
	float height;
};

struct Vertex
{
	float x;
	float y;
	float u;
	float v;

	bool operator==(const Vertex& other) const
	{
		return fabsf(x - other.x) < 0.01f && fabsf(y - other.y) < 0.01f;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return std::hash<float>()(vertex.x) ^ std::hash<float>()(vertex.y);
		}
	};
}

struct Quad
{
	// 0 top-left. 1 top-right. 2 bottom-right. 3 bottom-left
	Vertex verts[4];

	inline Vertex& operator[](uint32_t index)
	{
		assert(index >= 0 && index < 4);
		return verts[index];
	}

	inline const Vertex& operator[](uint32_t index) const
	{
		assert(index >= 0 && index < 4);
		return verts[index];
	}
};

inline float lerp(float a, float b, float t)
{
	return (1.f - t) * a + t * b;
}

void subdivideQuad(const Quad& quad, uint32_t x, uint32_t y, std::vector<Quad>* quads)
{
	quads->resize(x * y);

	float stepX = 1.f / x;
	float stepY = 1.f / y;

	for (uint32_t i = 0; i < x; i++)
	{
		for (uint32_t j = 0; j < y; j++)
		{
			float u1 = i * stepX;
			float u2 = (i + 1) * stepX;
			float v1 = j * stepY;
			float v2 = (j + 1) * stepY;

			Quad& subQuad = (*quads)[y * i + j];
			subQuad[0].u = u1;
			subQuad[0].v = v1;
			subQuad[1].u = u2;
			subQuad[1].v = v1;
			subQuad[2].u = u2;
			subQuad[2].v = v2;
			subQuad[3].u = u1;
			subQuad[3].v = v2;

			// 对y轴进行插值
			float top1x = lerp(quad[0].x, quad[1].x, u1);
			float top1y = lerp(quad[0].y, quad[1].y, u1);
			float top2x = lerp(quad[0].x, quad[1].x, u2);
			float top2y = lerp(quad[0].y, quad[1].y, u2);
			float bottom1x = lerp(quad[3].x, quad[2].x, u1);
			float bottom1y = lerp(quad[3].y, quad[2].y, u1);
			float bottom2x = lerp(quad[3].x, quad[2].x, u2);
			float bottom2y = lerp(quad[3].y, quad[2].y, u2);

			// 对x轴进行插值得到四个顶点
			subQuad[0].x = lerp(top1x, bottom1x, v1);
			subQuad[0].y = lerp(top1y, bottom1y, v1);
			subQuad[1].x = lerp(top2x, bottom2x, v1);
			subQuad[1].y = lerp(top2y, bottom2y, v1);

			subQuad[3].x = lerp(top1x, bottom1x, v2);
			subQuad[3].y = lerp(top1y, bottom1y, v2);
			subQuad[2].x = lerp(top2x, bottom2x, v2);
			subQuad[2].y = lerp(top2y, bottom2y, v2);
		}
	}
}

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<BindSetLayout> tex1BindSetLayout;
	std::shared_ptr<BindSetLayout> cbuf1tex1BindSetLayout;
	std::shared_ptr<Pipeline> imagePipeline;
	std::shared_ptr<Pipeline> vertexImagePipeline;
	
	std::shared_ptr<Texture> wallpaperTexture;
	std::shared_ptr<TextureView> wallpaperTextureView;
	std::shared_ptr<BindSet> wallpaperBindSet;

	Rect spout{ 0.f };
	std::shared_ptr<Texture> spoutTexture;
	std::shared_ptr<TextureView> spoutTextureView;
	std::shared_ptr<BindSet> spoutBindSet;

	std::shared_ptr<Buffer> paramBuffer;
	Rect window{ 0.f };
	std::shared_ptr<Texture> windowTexture;
	std::shared_ptr<TextureView> windowTextureView;
	std::shared_ptr<BindSet> windowBindSet;

	uint32_t verticesSize = 0;
	uint32_t indicesSize = 0;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;

	int direction = 1;
	float spoutPos = 200.f;
	float progress = 0.f;
	EasingAnimation<float> progressAnimation;
	std::vector<Quad> windowSubQuads;

	struct alignas(16) Param
	{
		glm::vec2 screenSize{};
	};
	Param param;

	const uint32_t subdivCount = 32;
	const uint32_t subdivCount2 = 8;

	void onSetup() override
	{
		tex1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::Sampler },
			{ .binding = 1, .type = BindEntryType::SampledTexture }
		});

		cbuf1tex1BindSetLayout = _device->createBindSetLayout
		({
			{ .binding = 0, .type = BindEntryType::ConstantBuffer },
			{ .binding = 1, .type = BindEntryType::Sampler },
			{ .binding = 2, .type = BindEntryType::SampledTexture }
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
			LOAD_SHADER("VertexImage.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("VertexImage.ps", psShader, _psCode, _psFilePath);
			vertexImagePipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.bindSetLayouts = { cbuf1tex1BindSetLayout },
				.vertexAttributes =
				{
					{ .location = 0, .semantic = "POSITION", .offset = offsetof(Vertex, x), .format = Format::RG32Sfloat },
					{ .location = 1, .semantic = "TEXCOORD", .offset = offsetof(Vertex, u), .format = Format::RG32Sfloat }
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

		std::tie(spoutTexture, spoutTextureView) = createTextureFormImage("vscode.png");
		assert(spoutTexture);
		spoutBindSet = _device->createBindSet(tex1BindSetLayout);
		spoutBindSet->bindSampler(0, _linerRepeatSampler);
		spoutBindSet->bindTexture(1, spoutTextureView);
		spout.width = spoutTexture->getWidth();
		spout.height = spoutTexture->getHeight();

		paramBuffer = _device->createBuffer
		({
			.size = sizeof(Param),
			.usage = BufferUsage::Constant,
			.hostVisible = true,
			.name = "param"
		});
		std::tie(windowTexture, windowTextureView) = createTextureFormImage("window.png");
		assert(windowTexture);
		windowBindSet = _device->createBindSet(cbuf1tex1BindSetLayout);
		windowBindSet->bindBuffer(0, paramBuffer);
		windowBindSet->bindSampler(1, _linerRepeatSampler);
		windowBindSet->bindTexture(2, windowTextureView);
		window.x = 500;
		window.y = 100;
		window.width = windowTexture->getWidth();
		window.height = windowTexture->getHeight();

		verticesSize = (subdivCount - 1) * (subdivCount2 - 1) + subdivCount * 2 + subdivCount2 * 2;
		vertexBuffer = _device->createBuffer
		({
			.size = verticesSize * sizeof(Vertex),
			.usage = BufferUsage::Vertex,
			.hostVisible = true,
			.name = "vertex"
		});
		indicesSize = subdivCount * subdivCount2 * 6;
		indexBuffer = _device->createBuffer
		({
			.size = indicesSize * sizeof(uint16_t),
			.usage = BufferUsage::Index,
			.hostVisible = true,
			.name = "vertex"
		});

		// 动画
		progressAnimation.start = 0.0f;
		progressAnimation.target = 1.0f;
		progressAnimation.duration = 0.4f;
		progressAnimation.easeFunction = [](float x) -> float { return x; };
	}

	enum PointAxis
	{
		POINT_AXIS_X = 0,
		POINT_AXIS_Y = 1
	};
	typedef float Point[2];

	struct BezierCurve
	{
		Point p0;
		Point cp1;
		Point cp2;
		Point p3;
	};

	inline float bezierPoint(const BezierCurve& curve, float t, enum PointAxis axis)
	{
		float u = 1.f - t;
		float tt = t * t;
		float uu = u * u;
		float uuu = uu * u;
		float ttt = tt * t;

		return uuu * curve.p0[axis] + 3.f * uu * t * curve.cp1[axis] + 3.f * u * tt * curve.cp2[axis] + ttt * curve.p3[axis];
	}

	inline float bezierDerivative(const BezierCurve& curve, float t, enum PointAxis axis)
	{
		float u = 1.f - t;
		float tt = t * t;
		float uu = u * u;

		return 3.f * uu * (curve.cp1[axis] - curve.p0[axis]) + 6.f * u * t * (curve.cp2[axis] - curve.cp1[axis]) +
			3.f * tt * (curve.p3[axis] - curve.cp2[axis]);
	}

	float bezierAxisIntersection(const BezierCurve& curve, enum PointAxis axis, float axisPos)
	{
		// Initial guess value
		float t = (axisPos - curve.p0[axis]) / (curve.p3[axis] - curve.p0[axis]);

		for (uint32_t i = 0; i < 3; i++)
		{
			float current = bezierPoint(curve, t, axis);
			float df = bezierDerivative(curve, t, axis);

			if (fabs(df) < 0.001f) break;

			// Newton-Raphson iterations. (Derivative correction)
			t -= (current - axisPos) / df;
		}
		return t;
	}

	inline void calculateBezierControlPoints(BezierCurve* curve)
	{
		curve->cp1[POINT_AXIS_X] = curve->p0[POINT_AXIS_X] + (curve->p3[POINT_AXIS_X] - curve->p0[POINT_AXIS_X]) * 0.25f;
		curve->cp1[POINT_AXIS_Y] = curve->p0[POINT_AXIS_Y] + (curve->p3[POINT_AXIS_Y] - curve->p0[POINT_AXIS_Y]) * 0.8f;

		curve->cp2[POINT_AXIS_X] = curve->p3[POINT_AXIS_X] - (curve->p3[POINT_AXIS_X] - curve->p0[POINT_AXIS_X]) * 0.2f;
		curve->cp2[POINT_AXIS_Y] = curve->p3[POINT_AXIS_Y] - (curve->p3[POINT_AXIS_Y] - curve->p0[POINT_AXIS_Y]) * 0.9f;
	}

	void calculateVertexCurvePosition(float progress)
	{
		if (direction == 0) // top
		{
			BezierCurve curve0 =
			{
				.p0 = { window.x, window.y + window.height },
				.p3 = { spout.x, spout.y + spout.height }
			};
			calculateBezierControlPoints(&curve0);

			BezierCurve curve1 =
			{
				.p0 = { window.x + window.width, window.y + window.height },
				.p3 = { spout.x + spout.width, spout.y + spout.height }
			};
			calculateBezierControlPoints(&curve1);

			for (Quad& quad : windowSubQuads)
			{
				for (uint32_t j = 0; j < 4; j++)
				{
					Vertex& vtx = quad[j];

					float y = lerp(curve0.p0[POINT_AXIS_Y], curve0.p3[POINT_AXIS_Y], progress) - (1.f - vtx.v) * window.height;
					y = fmax(y, curve0.p3[POINT_AXIS_Y]);

					float t0 = bezierAxisIntersection(curve0, POINT_AXIS_Y, y);
					float x0 = bezierPoint(curve0, t0, POINT_AXIS_X);

					float t1 = bezierAxisIntersection(curve1, POINT_AXIS_Y, y);
					float x1 = bezierPoint(curve1, t1, POINT_AXIS_X);

					vtx.x = lerp(x0, x1, vtx.u);
					vtx.y = y;
				}
			}
		}
		else if (direction == 1) // bottom
		{
			BezierCurve curve0 =
			{
				.p0 = { window.x, window.y },
				.p3 = { spout.x, spout.y }
			};
			calculateBezierControlPoints(&curve0);

			BezierCurve curve1 =
			{
				.p0 = { window.x + window.width, window.y },
				.p3 = { spout.x + spout.width, spout.y }
			};
			calculateBezierControlPoints(&curve1);

			for (Quad& quad : windowSubQuads)
			{
				for (uint32_t j = 0; j < 4; j++)
				{
					Vertex& vtx = quad[j];

					float y = lerp(curve0.p0[POINT_AXIS_Y], curve0.p3[POINT_AXIS_Y], progress) + vtx.v * window.height;
					y = fmin(y, curve0.p3[POINT_AXIS_Y]);

					float t0 = bezierAxisIntersection(curve0, POINT_AXIS_Y, y);
					float x0 = bezierPoint(curve0, t0, POINT_AXIS_X);

					float t1 = bezierAxisIntersection(curve1, POINT_AXIS_Y, y);
					float x1 = bezierPoint(curve1, t1, POINT_AXIS_X);

					vtx.x = lerp(x0, x1, vtx.u);
					vtx.y = y;
				}
			}
		}
		else if (direction == 2) // left
		{
			BezierCurve curve0 =
			{
				.p0 = { window.x + window.width, window.y },
				.p3 = { spout.x + spout.width, spout.y }
			};
			calculateBezierControlPoints(&curve0);

			BezierCurve curve1 =
			{
				.p0 = { window.x + window.width, window.y + window.height },
				.p3 = { spout.x + spout.width, spout.y + spout.height }
			};
			calculateBezierControlPoints(&curve1);

			for (Quad& quad : windowSubQuads)
			{
				for (uint32_t j = 0; j < 4; j++)
				{
					Vertex& vtx = quad[j];

					float x = lerp(curve0.p0[POINT_AXIS_X], curve0.p3[POINT_AXIS_X], progress) - (1.f - vtx.u) * window.width;
					x = fmax(x, curve0.p3[POINT_AXIS_X]);

					float t0 = bezierAxisIntersection(curve0, POINT_AXIS_X, x);
					float y0 = bezierPoint(curve0, t0, POINT_AXIS_Y);

					float t1 = bezierAxisIntersection(curve1, POINT_AXIS_X, x);
					float y1 = bezierPoint(curve1, t1, POINT_AXIS_Y);

					vtx.x = x;
					vtx.y = lerp(y0, y1, vtx.v);
				}
			}
		}
		else if (direction == 3) // right
		{
			BezierCurve curve0 = { .p0 = { window.x, window.y },.p3 = { spout.x, spout.y } };
			calculateBezierControlPoints(&curve0);

			BezierCurve curve1 = { .p0 = { window.x, window.y + window.height }, .p3 = { spout.x, spout.y + spout.height } };
			calculateBezierControlPoints(&curve1);

			for (Quad& quad : windowSubQuads)
			{
				for (uint32_t j = 0; j < 4; j++)
				{
					Vertex& vtx = quad[j];

					float x = lerp(curve0.p0[POINT_AXIS_X], curve0.p3[POINT_AXIS_X], progress) + vtx.u * window.width;
					x = fmin(x, curve0.p3[POINT_AXIS_X]);

					float t0 = bezierAxisIntersection(curve0, POINT_AXIS_X, x);
					float y0 = bezierPoint(curve0, t0, POINT_AXIS_Y);

					float t1 = bezierAxisIntersection(curve1, POINT_AXIS_X, x);
					float y1 = bezierPoint(curve1, t1, POINT_AXIS_Y);

					vtx.x = x;
					vtx.y = lerp(y0, y1, vtx.v);
				}
			}
		}
	}

	void process()
	{
		const float first_anim_dura = 0.3f;
		if (progress <= first_anim_dura)
		{
			// 第一段动画。矩形形变到曲线轨迹动画第零帧
			float progress = this->progress / first_anim_dura;
			calculateVertexCurvePosition(0.f);

			if (direction == 0 || direction == 1)
			{
				for (Quad& quad : windowSubQuads)
				{
					for (uint32_t j = 0; j < 4; j++)
					{
						Vertex& vtx = quad[j];
						vtx.x = lerp(window.x + vtx.u * window.width, vtx.x, progress);
					}
				}
			}
			else if (direction == 2 || direction == 3)
			{
				for (Quad& quad : windowSubQuads)
				{
					for (uint32_t j = 0; j < 4; j++)
					{
						Vertex& vtx = quad[j];
						vtx.y = lerp(window.y + vtx.v * window.height, vtx.y, progress);
					}
				}
			}
		}
		else
		{
			float progress = (this->progress - first_anim_dura) / (1.f - first_anim_dura);
			calculateVertexCurvePosition(progress);
		}
	}

	void triangulate()
	{
		std::vector<Vertex> verticesArray;
		verticesArray.reserve(indicesSize);
		for (Quad& quad : windowSubQuads)
		{
			verticesArray.emplace_back(quad[0]);
			verticesArray.emplace_back(quad[2]);
			verticesArray.emplace_back(quad[1]);
			verticesArray.emplace_back(quad[0]);
			verticesArray.emplace_back(quad[3]);
			verticesArray.emplace_back(quad[2]);
		}

		vertices.clear();
		vertices.reserve(verticesSize);
		indices.clear();
		indices.reserve(indicesSize);
		std::unordered_map<Vertex, uint16_t> uniqueVertices{};
		for (Vertex& vertex : verticesArray)
		{
			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint16_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}

		//static uint32_t maxVerticesSize = 0;
		//maxVerticesSize = std::max(maxVerticesSize, (uint32_t)vertices.size());
	}

	void onImGui() override
	{
		ImGui::DragFloat("Spout Distance", &spoutPos);

		const char* directionItems[] = { "Up", "Bottom", "Left", "Right" };
		ImGui::Combo("Direction", &direction, directionItems, IM_ARRAYSIZE(directionItems));
		switch (direction)
		{
		case 0:
			spout.x = spoutPos;
			spout.y = 0;
			break;
		case 1:
			spout.x = spoutPos;
			spout.y = getHeight() - spout.height;
			break;
		case 2:
			spout.x = 0;
			spout.y = spoutPos;
			break;
		case 3:
			spout.x = getWidth() - spout.width;
			spout.y = spoutPos;
			break;
		}

		ImGui::DragFloat2("Window Pos", &window.x);
		// ImGui::Text("Spout Pos: %.1f, %.1f", spout.x, spout.y);
		ImGui::Separator();
		if (ImGui::Button("In"))
		{
			progressAnimation.run();
		}
		ImGui::SameLine();
		if (ImGui::Button("Out"))
		{
			progressAnimation.runReverse();
		}
		ImGui::DragFloat("Progress", &progress, 0.01f, 0.f, 1.f);
	}

	void onUpdate(float deltaTime) override
	{
		progressAnimation.frame();
		if (progressAnimation.isRunning)	progress = progressAnimation.value;

		glm::vec2 windowPos{ window.x, window.y };
		glm::vec2 windowSize{ window.width, window.height };

		// subdivide
		Quad windowQuad;
		windowQuad[0].x = window.x;
		windowQuad[0].y = window.y;
		windowQuad[1].x = window.x + window.width;
		windowQuad[1].y = window.y;
		windowQuad[2].x = window.x + window.width;
		windowQuad[2].y = window.y + window.height;
		windowQuad[3].x = window.x;
		windowQuad[3].y = window.y + window.height;
		switch (direction)
		{
		case 0:
		case 1:
			subdivideQuad(windowQuad, subdivCount2, subdivCount, &windowSubQuads);
			break;
		case 2:
		case 3:
			subdivideQuad(windowQuad, subdivCount, subdivCount2, &windowSubQuads);
			break;
		}

		// modify vertex
		process();

		triangulate();
		memcpy(vertexBuffer->map(), vertices.data(), sizeof(Vertex) * vertices.size());
		memcpy(indexBuffer->map(), indices.data(), sizeof(uint16_t) * indices.size());
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		float width = getWidth();
		float height = getHeight();

		param.screenSize = { width, height };
		memcpy(paramBuffer->map(), &param, sizeof(Param));

		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearColor = { 0.0f, 0.0f, 0.0f, 1.0f }
		} } });
		// Draw wallpaper
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, wallpaperBindSet);
		commandList->setViewport(0, 0, width, height);
		commandList->setScissor(0, 0, width, height);
		commandList->draw(6);
		// Draw icon
		commandList->setPipeline(imagePipeline);
		commandList->setBindSet(0, spoutBindSet);
		commandList->setViewport(spout.x, spout.y, spout.width, spout.height);
		commandList->setScissor(spout.x, spout.y, spout.width, spout.height);
		commandList->draw(6);
		// Draw window
		commandList->beginLabel("Draw window");
		commandList->setPipeline(vertexImagePipeline);
		commandList->setViewport(0, 0, width, height);
		commandList->setScissor(0, 0, width, height);
		commandList->setBindSet(0, windowBindSet);
		commandList->setVertexBuffer(0, vertexBuffer);
		commandList->setIndexBuffer(indexBuffer, Format::R16Uint);
		commandList->drawIndexed(indices.size());
		commandList->endLabel();
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
