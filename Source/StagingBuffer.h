#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
	class StagingBuffer final
	{
	public:
		StagingBuffer() = default;
		StagingBuffer(BackendType backend, const std::shared_ptr<Device>& device);

		void uploadBuffer(std::shared_ptr<Buffer> buffer, void* data, size_t size);
		void uploadTexture(std::shared_ptr<Texture> texture, void* data, size_t size);
		bool resize(size_t size);

		static void initGlobal(BackendType backend, const std::shared_ptr<Device>& device);
		static void destroyGlobal();
		static StagingBuffer& getGlobal();

	private:
		BackendType _backend = BackendType::Vulkan;
		std::shared_ptr<Device> _device;
		std::shared_ptr<Buffer> _buffer;
	};
}
