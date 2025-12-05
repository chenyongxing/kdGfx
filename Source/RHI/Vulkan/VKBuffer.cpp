#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKAPI.h"

namespace kdGfx
{
	static VkBufferUsageFlags BufferUsageToVkBufferUsage(BufferUsage usage)
	{
		VkBufferUsageFlags vkUsage = 0;

		if (usage & BufferUsage::CopyDst)
		{
			vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		if (usage & BufferUsage::CopySrc)
		{
			vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}
		if (usage & BufferUsage::Constant)
		{
			vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		}
		if (usage & BufferUsage::Readed || usage & BufferUsage::Storage)
		{
			vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (usage & BufferUsage::Index)
		{
			vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (usage & BufferUsage::Vertex)
		{
			vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (usage & BufferUsage::Indirect)
		{
			vkUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}

		return vkUsage;
	}

	VKBuffer::VKBuffer(VKDevice& device, const BufferDesc& desc) :
		_device(device)
	{
		_desc = desc;

		if (desc.usage & BufferUsage::Constant)
		{
			_desc.hostVisible = HostVisible::Upload;
		}

		VkBufferCreateInfo bufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = desc.size,
			.usage = BufferUsageToVkBufferUsage(_desc.usage)
		};
		if (vkCreateBuffer(_device.getDevice(), &bufferCreateInfo, nullptr, &_buffer) != VK_SUCCESS)
		{
			spdlog::error("failed to create vulkan buffer: {}", _desc.name);
		}

		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		if (_desc.hostVisible != HostVisible::Invisible)
		{
			properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_device.getDevice(), _buffer, &memRequirements);
		VkMemoryAllocateInfo allocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = _device.findMemoryType(memRequirements.memoryTypeBits, properties)
		};
		if (vkAllocateMemory(_device.getDevice(), &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS)
		{
			RuntimeError("failed to allocate vulkan buffer memory");
		}
		vkBindBufferMemory(_device.getDevice(), _buffer, _bufferMemory, 0);

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo = 
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)_buffer,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}
	}

	VKBuffer::VKBuffer(VKDevice& device, const BufferDesc& desc, const std::shared_ptr<VKMemory>& memory, size_t offset) :
		_device(device)
	{
		_desc = desc;

		if (desc.usage & BufferUsage::Constant)
		{
			_desc.hostVisible = HostVisible::Upload;
		}

		VkBufferCreateInfo bufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = desc.size,
			.usage = BufferUsageToVkBufferUsage(_desc.usage)
		};
		if (vkCreateBuffer(_device.getDevice(), &bufferCreateInfo, nullptr, &_buffer) != VK_SUCCESS)
		{
			spdlog::error("failed to create vulkan buffer: {}", _desc.name);
		}

		vkBindBufferMemory(device.getDevice(), _buffer, memory->getMemory(), offset);

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_BUFFER,
				.objectHandle = (uint64_t)_buffer,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}

		_memory = memory;
		_memoryOffset = offset;
	}

	VKBuffer::~VKBuffer()
	{
		if (_bufferMemory)
			vkFreeMemory(_device.getDevice(), _bufferMemory, nullptr);
		vkDestroyBuffer(_device.getDevice(), _buffer, nullptr);
	}

	void* VKBuffer::map()
	{
		if (_mappedPtr == nullptr && (_desc.hostVisible != HostVisible::Invisible))
		{
			vkMapMemory(_device.getDevice(), _bufferMemory, 0, _desc.size, 0, &_mappedPtr);
		}
		return _mappedPtr;
	}

	void VKBuffer::unmap()
	{
		if (_mappedPtr && (_desc.hostVisible != HostVisible::Invisible))
		{
			vkUnmapMemory(_device.getDevice(), _bufferMemory);
			_mappedPtr = nullptr;
		}
	}
}
