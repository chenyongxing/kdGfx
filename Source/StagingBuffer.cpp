#include "StagingBuffer.h"

namespace kdGfx
{
	StagingBuffer::StagingBuffer(BackendType backend, const std::shared_ptr<Device>& device) :
		_backend(backend),
		_device(device)
	{
	}

	void StagingBuffer::uploadBuffer(std::shared_ptr<Buffer> buffer, void* data, size_t size)
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

	void StagingBuffer::uploadTexture(std::shared_ptr<Texture> texture, void* data, size_t size)
	{
		resize(size);
		memcpy(_buffer->map(), data, size);

		if (_backend == BackendType::DirectX12)
		{
			// dx12复制队列复制需要状态General
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

	bool StagingBuffer::resize(size_t size)
	{
		if (_buffer == nullptr || _buffer->getSize() < size)
		{
			_buffer = _device->createBuffer
			({
				.size = size,
				.usage = BufferUsage::CopySrc,
				.hostVisible = true,
				.name = "Staging Buffer"
			});
			return true;
		}
		return false;
	}

	static StagingBuffer gStagingBuffer;

	void StagingBuffer::initGlobal(BackendType backend, const std::shared_ptr<Device>& device)
	{
		gStagingBuffer._backend = backend;
		gStagingBuffer._device = device;
	}

	void StagingBuffer::destroyGlobal()
	{
		gStagingBuffer._buffer.reset();
		gStagingBuffer._device.reset();
	}

	StagingBuffer& StagingBuffer::getGlobal()
	{
		return gStagingBuffer;
	}
}
