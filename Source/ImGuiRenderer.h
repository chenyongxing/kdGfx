#pragma once

#include <imgui/imgui.h>
#include "RHI/Device.h"
#include "StagingBuffer.h"

namespace kdGfx
{
	class ImGuiRenderer final
	{
	public:
        struct InitInfo
        {
			BackendType backend;
            std::shared_ptr<Device> device;
			Format renderTargetFormat = Format::Undefined;
            uint32_t frameCount = 2;
        };

		struct RenderState
		{
			std::shared_ptr<Device> device;
			std::shared_ptr<CommandList> commandList;
		};

		void init(const InitInfo& initInfo);
		void shutdown();
		void newFrame();
		void renderDrawData(ImDrawData* drawData, const std::shared_ptr<CommandList>& commandList);
		void createDeviceObjects();
		void invalidateDeviceObjects();

	private:
		void _createFontsTexture();
		BackendType _backend;
	};
}
