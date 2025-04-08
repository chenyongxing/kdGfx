#include "VKAdapter.h"
#include "VKDevice.h"

namespace kdGfx
{
	VKAdapter::VKAdapter(VKInstance& instance, VkPhysicalDevice physicalDevice) :
		_instance(instance),
		_physicalDevice(physicalDevice)
	{
		vkGetPhysicalDeviceProperties(physicalDevice, &_properties);

		_name = _properties.deviceName;
		_discreteGPU = _properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	}

	std::shared_ptr<Device> VKAdapter::createDevice()
	{
		return std::make_shared<VKDevice>(*this);
	}
}
