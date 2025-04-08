#include "VKBindSetLayout.h"
#include "VKDevice.h"

namespace kdGfx
{
	VKBindSetLayout::VKBindSetLayout(VKDevice& device, const std::vector<BindEntryLayout>& entryLayouts) :
		_device(device)
	{
		_entryLayouts = entryLayouts;

		bool hasBindless = false;
		std::vector<VkDescriptorSetLayoutBinding> descLayoutBindings(entryLayouts.size());
		std::vector<VkDescriptorBindingFlags> descLayoutBindingFlags(entryLayouts.size());
		for (size_t i = 0; i < entryLayouts.size(); ++i)
		{
			const BindEntryLayout& entryLayout = entryLayouts[i];
			const bool isBindless = (entryLayout.count == UINT32_MAX);
			const uint32_t maxDescriptorCount = _device.getMaxDescriptorCount(entryLayout.type);
			VkDescriptorSetLayoutBinding& descLayoutBinding = descLayoutBindings[i];
			descLayoutBinding.binding = entryLayout.binding;
			descLayoutBinding.descriptorCount = isBindless ? maxDescriptorCount : 1;
			descLayoutBinding.descriptorType = _device.toVkDescriptorType(entryLayout.type);
			descLayoutBinding.stageFlags = _device.toVkShaderStageFlags(entryLayout.visible);
			if (!hasBindless)	hasBindless = isBindless;
			VkDescriptorBindingFlags flags = 0;
			if (isBindless) flags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
			descLayoutBindingFlags[i] = flags;
		}
		VkDescriptorSetLayoutCreateInfo descLayoutInfoCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = (uint32_t)descLayoutBindings.size(),
			.pBindings = descLayoutBindings.data()
		};
		VkDescriptorSetLayoutBindingFlagsCreateInfo descSetLayoutBindingFlagsCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = (uint32_t)descLayoutBindingFlags.size(),
			.pBindingFlags = descLayoutBindingFlags.data()
		};
		if (hasBindless)	descLayoutInfoCreateInfo.pNext = &descSetLayoutBindingFlagsCreateInfo;
		vkCreateDescriptorSetLayout(_device.getDevice(), &descLayoutInfoCreateInfo, nullptr, &_descriptorSetLayout);
	}

	VKBindSetLayout::~VKBindSetLayout()
	{
		vkDestroyDescriptorSetLayout(_device.getDevice(), _descriptorSetLayout, nullptr);
	}
}
