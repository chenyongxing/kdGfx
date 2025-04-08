#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
	class ImageProcessor final
	{
	public:
		struct Param
		{
			glm::mat4 colorMatrix{ 1.0f };
			float gamma = 1.0f;
		};

		static void initSingleton(BackendType backend, const std::shared_ptr<Device>& device);
		static void destroySingleton();
		static ImageProcessor& singleton();

		void process(const Param& param, const std::shared_ptr<Texture>& texture, uint32_t mipLevel = 0);

	private:
		BackendType _backend = BackendType::Vulkan;
		std::shared_ptr<Device> _device;
		std::shared_ptr<BindSetLayout> _bindSetLayout;
		std::shared_ptr<Pipeline> _pipeline;
	};
}
