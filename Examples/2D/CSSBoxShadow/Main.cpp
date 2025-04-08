#include "WindowApp.h"

#include <imgui/imgui.h>

using namespace kdGfx;

struct BoxShadow
{
	// cornerRadius
	glm::vec2 offset{ 0.f, 0.f };
	float blurRadius = 30.f;
	float spreadRadius = 0.f;
	glm::vec4 color{ 0.f, 0.f, 0.f, 1.f };
};

class MainWindow : public WindowApp
{
public:
	std::shared_ptr<Pipeline> colorRoundRectPipeline;
	std::shared_ptr<Pipeline> fastRoundRectBlurPipeline;

	bool drawBox = true;
	glm::vec4 box{ 100.f, 100.f, 200.f, 100.f };
	glm::vec4 color{ 1.f, 1.0f, 0.f, 0.1f };
	float cornerRadius = 20.f;
	bool drawShadows = true;
	std::vector<BoxShadow> boxShadows{1};

	void onSetup() override
	{
		{
			LOAD_SHADER("ColorRoundRect.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("ColorRoundRect.ps", psShader, _psCode, _psFilePath);
			colorRoundRectPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 10 },
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}

		{
			LOAD_SHADER("FastRoundRectBlur.vs", vsShader, _vsCode, _vsFilePath);
			LOAD_SHADER("FastRoundRectBlur.ps", psShader, _psCode, _psFilePath);
			fastRoundRectBlurPipeline = _device->createRasterPipeline
			({
				.vertex = vsShader,
				.pixel = psShader,
				.pushConstantLayout = { sizeof(float) * 18 },
				.colorFormats = { _swapchain->getFormat() },
				.colorBlends = {{ true }}
			});
		}
	}

	void onImGui() override
	{
		ImGui::Checkbox("drawBox", &drawBox);
		ImGui::DragFloat4("box", glm::value_ptr(box));
		ImGui::ColorEdit4("color", glm::value_ptr(color));
		ImGui::DragFloat("cornerRadius", &cornerRadius);
		cornerRadius = glm::max(0.00001f, cornerRadius);
		ImGui::Separator();
		ImGui::Checkbox("drawShadows", &drawShadows);
		if (ImGui::Button("Add Box Shadow"))
			boxShadows.emplace_back();
		ImGui::Separator();

		for (int i = 0; i < boxShadows.size(); i++)
		{
			auto& shadow = boxShadows[i];
			std::string name("Box Shadow");
			name.append(std::to_string(i));
			if (ImGui::TreeNode(name.c_str()))
			{
				ImGui::DragFloat2("offset", glm::value_ptr(shadow.offset));
				ImGui::DragFloat("blurRadius", &shadow.blurRadius);
				shadow.blurRadius = glm::max(0.f, shadow.blurRadius);
				ImGui::DragFloat("spreadRadius", &shadow.spreadRadius);
				shadow.spreadRadius = glm::max(0.f, shadow.spreadRadius);
				ImGui::ColorEdit4("color", glm::value_ptr(shadow.color));
				if (ImGui::Button("Delete"))
					boxShadows.erase(boxShadows.begin() + i);
				ImGui::TreePop();
			}
		}
	}

	void onRender(const std::shared_ptr<CommandList>& commandList) override
	{
		auto [_, backBufferView] = getBackBuffer(_frameIndex);
		commandList->beginRenderPass
		({ { {
				.textureView = backBufferView,
				.loadOp = LoadOp::Clear,
				.clearValue = { 1.0f, 1.0f, 1.0f, 1.0f }
		} } });

		// Draw shadow
		commandList->setViewport(0, 0, getWidth(), getHeight());
		commandList->setScissor(0, 0, getWidth(), getHeight());
		
		float distScale = 1.0f / (getHeight() / 2);
		struct
		{
			glm::vec4 color;
			glm::vec4 rect;
			float aspectRatio;
			float radius;
		} sdfPushConst =
		{
			.color = color,
			.rect = box * distScale,
			.aspectRatio = getWidth() / (float)getHeight(),
			.radius = cornerRadius * distScale
		};

		if (drawShadows)
		{
			commandList->setPipeline(fastRoundRectBlurPipeline);
			for (auto it = boxShadows.rbegin(); it != boxShadows.rend(); ++it)
			{
				const auto& boxShadow = *it;
				struct
				{
					glm::vec4 color;
					glm::vec2 topLeft;
					glm::vec2 bottomRight;
					float sigma;
					float radius;
					glm::vec2 _pad0;
					glm::vec4 sdfRect;
					float aspectRatio;
					float sdfRadius;
				} pushConst =
				{
					.color = boxShadow.color,
					.topLeft =
					{
						box.x + boxShadow.offset.x - boxShadow.spreadRadius,
						box.y + boxShadow.offset.y - boxShadow.spreadRadius
					},
					.bottomRight =
					{
						box.x + boxShadow.offset.x + box.z + boxShadow.spreadRadius,
						box.y + boxShadow.offset.y + box.w + boxShadow.spreadRadius
					},
					.sigma = boxShadow.blurRadius * 0.5f,
					.radius = cornerRadius,
					.sdfRect = sdfPushConst.rect,
					.aspectRatio = sdfPushConst.aspectRatio,
					.sdfRadius = sdfPushConst.radius
				};
				commandList->setPushConstant(&pushConst);
				commandList->draw(3);
			}
		}
		
		// Draw rect
		if (drawBox)
		{
			commandList->setPipeline(colorRoundRectPipeline);
			commandList->setPushConstant(&sdfPushConst);
			commandList->draw(3);
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
