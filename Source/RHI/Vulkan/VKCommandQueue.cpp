#include "VKCommandQueue.h"
#include "VKDevice.h"
#include "VKCommandList.h"
#include "VKFence.h"

namespace kdGfx
{
	VKCommandQueue::VKCommandQueue(VKDevice& device, uint32_t queueFamilyIndex) :
		_device(device),
		_queueFamilyIndex(queueFamilyIndex)
	{
		vkGetDeviceQueue(_device.getDevice(), _queueFamilyIndex, 0, &_queue);
	}

	void VKCommandQueue::signal(const std::shared_ptr<Fence>& fence, uint64_t value)
	{
		auto vkFence = std::dynamic_pointer_cast<VKFence>(fence);
		if (!vkFence) return;
		if (!vkFence->getTimelineSemaphore()) return;

		VkTimelineSemaphoreSubmitInfo timelineInfo =
		{
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.signalSemaphoreValueCount = 1,
			.pSignalSemaphoreValues = &value
		};
		VkSemaphore timelineSemaphore = vkFence->getTimelineSemaphore();
		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &timelineSemaphore
		};
		vkQueueSubmit(_queue, 1, &submitInfo, nullptr);
	}

	void VKCommandQueue::wait(const std::shared_ptr<Fence>& fence, uint64_t value)
	{
		auto vkFence = std::dynamic_pointer_cast<VKFence>(fence);
		if (!vkFence) return;
		if (!vkFence->getTimelineSemaphore()) return;

		VkTimelineSemaphoreSubmitInfo timelineInfo =
		{
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount = 1,
			.pWaitSemaphoreValues = &value,
		};
		VkSemaphore timelineSemaphore = vkFence->getTimelineSemaphore();
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &timelineSemaphore,
			.pWaitDstStageMask = &waitDstStageMask

		};
		vkQueueSubmit(_queue, 1, &submitInfo, nullptr);
	}

	void VKCommandQueue::submit(const std::vector<std::shared_ptr<CommandList>>& commandLists)
	{
		std::vector<VkCommandBuffer> commandBuffers;
		for (auto& commandList : commandLists)
		{
			auto vkCommandList = std::dynamic_pointer_cast<VKCommandList>(commandList);
			if (!vkCommandList) continue;
			if (!vkCommandList->getCommandBuffer()) continue;
			commandBuffers.emplace_back(vkCommandList->getCommandBuffer());
		}

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pWaitDstStageMask = &waitDstStageMask,
			.commandBufferCount = (uint32_t)commandBuffers.size(),
			.pCommandBuffers = commandBuffers.data()
		};
		vkQueueSubmit(_queue, 1, &submitInfo, nullptr);
	}

	void VKCommandQueue::waitIdle()
	{
		vkQueueWaitIdle(_queue);
	}
}
