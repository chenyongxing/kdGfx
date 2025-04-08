#include "VKFence.h"
#include "VKDevice.h"

namespace kdGfx
{
	VKFence::VKFence(VKDevice& device, uint64_t initialValue) :
		_device(device)
	{
		VkSemaphoreTypeCreateInfo timelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
			.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
			.initialValue = initialValue
		};
		VkSemaphoreCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = &timelineCreateInfo
		};
		vkCreateSemaphore(_device.getDevice(), &createInfo, nullptr, &_timelineSemaphore);
	}

	VKFence::~VKFence()
	{
		vkDestroySemaphore(_device.getDevice(), _timelineSemaphore, nullptr);
	}

	void VKFence::signal(uint64_t value)
	{
		VkSemaphoreSignalInfo signalInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
			.semaphore = _timelineSemaphore,
			.value = value
		};
		vkSignalSemaphore(_device.getDevice(), &signalInfo);
	}

	void VKFence::wait(uint64_t value)
	{
		VkSemaphoreWaitInfo waitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &_timelineSemaphore,
			.pValues = &value
		};
		vkWaitSemaphores(_device.getDevice(), &waitInfo, UINT64_MAX);
	}

	uint64_t VKFence::getCompletedValue()
	{
		uint64_t value;
		vkGetSemaphoreCounterValue(_device.getDevice(), _timelineSemaphore, &value);
		return value;
	}
}
