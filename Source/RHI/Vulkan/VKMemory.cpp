#include "VKMemory.h"
#include "VKAPI.h"

namespace kdGfx
{
	VKMemory::VKMemory(VKDevice& device, const MemoryDesc& desc) :
		_device(device)
	{
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		switch (desc.type)
		{
		case MemoryType::Upload:
			properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
		case MemoryType::Readback:
			properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;
		}

		uint32_t memoryTypeIndex = 0;
		VkPhysicalDevice physicalDevice = _device.getAdapter().getPhysicalDevice();
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				memoryTypeIndex |= (1 << i);
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = desc.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;
		if (vkAllocateMemory(_device.getDevice(), &allocInfo, nullptr, &_memory) != VK_SUCCESS)
		{
			RuntimeError("failed to allocate vulkan memory");
		}

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY,
				.objectHandle = (uint64_t)_memory,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}
	}

	VKMemory::~VKMemory()
	{
		vkFreeMemory(_device.getDevice(), _memory, nullptr);
	}
}
