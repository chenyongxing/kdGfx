#pragma once

#include "RHI/Device.h"

namespace kdGfx
{
	class StagingBuffer final
	{
	public:
		StagingBuffer() = default;
		StagingBuffer(HostVisible hostVisible, BackendType backend, const std::shared_ptr<Device>& device);

		void uploadBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size);
		void uploadTexture(std::shared_ptr<Texture> texture, const void* data, size_t size);
		void readbackTexture(std::shared_ptr<Texture> texture, void* data, size_t size);
		bool resize(size_t size);

		static void initUploadGlobal(BackendType backend, const std::shared_ptr<Device>& device);
		static void destroyUploadGlobal();
		static StagingBuffer& getUploadGlobal();

		static void initReadbackGlobal(BackendType backend, const std::shared_ptr<Device>& device);
		static void destroyReadbackGlobal();
		static StagingBuffer& getReadbackGlobal();

	private:
		HostVisible _hostVisible = HostVisible::Upload;
		BackendType _backend = BackendType::Vulkan;
		std::shared_ptr<Device> _device;
		std::shared_ptr<Buffer> _buffer;
	};
}
