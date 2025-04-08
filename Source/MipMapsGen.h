#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
	class MipMapsGen final
	{
	public:
		static void initSingleton(BackendType backend, const std::shared_ptr<Device>& device);
		static void destroySingleton();
		static MipMapsGen& singleton();

		void generate(const std::shared_ptr<Texture>& texture);

	private:
		BackendType _backend = BackendType::Vulkan;
		std::shared_ptr<Device> _device;
		std::shared_ptr<BindSetLayout> _bindSetLayout;
		std::shared_ptr<Pipeline> _pipeline;
	};
}
