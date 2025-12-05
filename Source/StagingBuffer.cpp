#include "StagingBuffer.h"

namespace kdGfx
{
	StagingBuffer::StagingBuffer(HostVisible hostVisible, BackendType backend, const std::shared_ptr<Device>& device) :
		_hostVisible(hostVisible),
		_backend(backend),
		_device(device)
	{
	}

	void StagingBuffer::uploadBuffer(std::shared_ptr<Buffer> buffer, const void* data, size_t size)
	{
		resize(size);
		memcpy(_buffer->map(), data, size);

		auto commandList = _device->createCommandList(CommandListType::Copy);
		commandList->begin();
		commandList->copyBuffer(_buffer, buffer, size);
		commandList->end();
		auto copyQueue = _device->getCommandQueue(CommandListType::Copy);
		copyQueue->submit({ commandList });
		copyQueue->waitIdle();
	}

	void StagingBuffer::uploadTexture(std::shared_ptr<Texture> texture, const void* data, size_t size)
	{
		resize(size);
		memcpy(_buffer->map(), data, size);

		if (_backend == BackendType::DirectX12)
		{
			// dx12复制队列复制需要General状态
			auto commandList = _device->createCommandList(CommandListType::Copy);
			commandList->begin();
			commandList->copyBufferToTexture(_buffer, texture);
			commandList->end();
			auto copyQueue = _device->getCommandQueue(CommandListType::Copy);
			copyQueue->submit({ commandList });
			copyQueue->waitIdle();
		}
		else if (_backend == BackendType::Vulkan)
		{
			// vulkan需要CopyDst状态，最后顺便切换到ShaderRead
			auto commandList = _device->createCommandList(CommandListType::General);
			commandList->begin();
			commandList->resourceBarrier
			({ 
				.texture = texture, 
				.oldState = TextureState::Undefined, 
				.newState = TextureState::CopyDst,
				.subRange = { 0, texture->getDesc().mipLevels, 0, 1}
			});
			commandList->copyBufferToTexture(_buffer, texture);
			commandList->resourceBarrier
			({ 
				.texture = texture, 
				.oldState = TextureState::CopyDst,
				.newState = TextureState::ShaderRead,
				.subRange = { 0, texture->getDesc().mipLevels, 0, 1}
			});
			commandList->end();
			auto copyQueue = _device->getCommandQueue(CommandListType::General);
			copyQueue->submit({ commandList });
			copyQueue->waitIdle();
		}
	}

	void StagingBuffer::readbackTexture(std::shared_ptr<Texture> texture, void* data, size_t size)
	{
		assert(_device && _hostVisible == HostVisible::Readback);

		resize(size);

		if (_backend == BackendType::DirectX12)
		{
			// dx12复制队列复制需要General状态
			auto commandList = _device->createCommandList(CommandListType::Copy);
			commandList->begin();
			commandList->copyTextureToBuffer(texture, _buffer);
			commandList->end();
			auto copyQueue = _device->getCommandQueue(CommandListType::Copy);
			copyQueue->submit({ commandList });
			copyQueue->waitIdle();
		}
		else if (_backend == BackendType::Vulkan)
		{
			auto textureState = texture->getState();
			auto commandList = _device->createCommandList(CommandListType::General);
			commandList->begin();
			commandList->resourceBarrier
			({
				.texture = texture,
				.oldState = textureState,
				.newState = TextureState::CopySrc,
				.subRange = { 0, texture->getDesc().mipLevels, 0, 1}
				});
			commandList->copyTextureToBuffer(texture, _buffer);
			commandList->resourceBarrier
			({
				.texture = texture,
				.oldState = TextureState::CopySrc,
				.newState = textureState,
				.subRange = { 0, texture->getDesc().mipLevels, 0, 1}
				});
			commandList->end();
			auto copyQueue = _device->getCommandQueue(CommandListType::General);
			copyQueue->submit({ commandList });
			copyQueue->waitIdle();
		}

		memcpy(data, _buffer->map(), size);
	}

	bool StagingBuffer::resize(size_t size)
	{
		if (_buffer == nullptr || _buffer->getSize() < size)
		{
			_buffer = _device->createBuffer
			({
				.size = size,
				.usage = (_hostVisible == HostVisible::Readback) ? BufferUsage::CopyDst : BufferUsage::CopySrc,
				.hostVisible = _hostVisible,
				.name = "Staging Buffer"
			});
			return true;
		}
		return false;
	}

	static StagingBuffer gUploadStagingBuffer;

	void StagingBuffer::initUploadGlobal(BackendType backend, const std::shared_ptr<Device>& device)
	{
		gUploadStagingBuffer._hostVisible = HostVisible::Upload;
		gUploadStagingBuffer._backend = backend;
		gUploadStagingBuffer._device = device;
	}

	void StagingBuffer::destroyUploadGlobal()
	{
		gUploadStagingBuffer._buffer.reset();
		gUploadStagingBuffer._device.reset();
	}

	StagingBuffer& StagingBuffer::getUploadGlobal()
	{
		return gUploadStagingBuffer;
	}

	static StagingBuffer gReadbackStagingBuffer;

	void StagingBuffer::initReadbackGlobal(BackendType backend, const std::shared_ptr<Device>& device)
	{
		gReadbackStagingBuffer._hostVisible = HostVisible::Readback;
		gReadbackStagingBuffer._backend = backend;
		gReadbackStagingBuffer._device = device;
	}

	void StagingBuffer::destroyReadbackGlobal()
	{
		gReadbackStagingBuffer._buffer.reset();
		gReadbackStagingBuffer._device.reset();
	}

	StagingBuffer& StagingBuffer::getReadbackGlobal()
	{
		return gReadbackStagingBuffer;
	}
}
