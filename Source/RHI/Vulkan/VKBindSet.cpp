#include "VKBindSet.h"
#include "VKDevice.h"
#include "VKBuffer.h"
#include "VKTexture.h"

namespace kdGfx
{
	VKBindSet::VKBindSet(VKDevice& device, const std::shared_ptr<BindSetLayout>& layout) :
		_device(device)
	{
		_layout = layout;
		const auto& entryLayouts = _layout->getEntryLayouts();

		assert(entryLayouts.size() > 0);

		bool hasBindless = false;
		uint32_t maxVarDescriptorCount = 0;
		// 绑定组分配池
		std::vector<VkDescriptorPoolSize> poolSizes(entryLayouts.size());
		for (uint32_t i = 0; i < entryLayouts.size(); ++i)
		{
			const BindEntryLayout& entryLayout = entryLayouts[i];
			VkDescriptorType type = _device.toVkDescriptorType(entryLayout.type);
			_entriesType[entryLayout.binding] = type;
			const bool isBindless = (entryLayout.count == UINT32_MAX);
			if (!hasBindless)	hasBindless = isBindless;
			const uint32_t maxDescriptorCount = _device.getMaxDescriptorCount(entryLayout.type);
			maxVarDescriptorCount = std::max(maxVarDescriptorCount, maxDescriptorCount);
			VkDescriptorPoolSize& poolSize = poolSizes[i];
			poolSize.descriptorCount = isBindless ? maxDescriptorCount : 1;
			poolSize.type = type;
		}
		VkDescriptorPoolCreateInfo poolInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1,
			.poolSizeCount = (uint32_t)poolSizes.size(),
			.pPoolSizes = poolSizes.data()
		};
		VkResult vkResult = vkCreateDescriptorPool(_device.getDevice(), &poolInfo, nullptr, &_descriptorPool);
		if (vkResult != VK_SUCCESS)
		{
			spdlog::error("failed to create descriptor pool");
			return;
		}

		// 绑定组
		auto vkLayout = std::dynamic_pointer_cast<VKBindSetLayout>(_layout);
		VkDescriptorSetLayout setLayout = vkLayout->getDescriptorSetLayout();
		VkDescriptorSetAllocateInfo allocInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = _descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &setLayout
		};
		VkDescriptorSetVariableDescriptorCountAllocateInfo bindlessAllocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
			.descriptorSetCount = 1,
			.pDescriptorCounts = &maxVarDescriptorCount
		};
		if (hasBindless)	allocInfo.pNext = &bindlessAllocInfo;
		vkResult = vkAllocateDescriptorSets(_device.getDevice(), &allocInfo, &_descriptorSet);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyDescriptorPool(_device.getDevice(), _descriptorPool, nullptr);
			spdlog::error("failed to allocate descriptor set");
			return;
		}
	}

	VKBindSet::~VKBindSet()
	{
		if (_descriptorPool) vkDestroyDescriptorPool(_device.getDevice(), _descriptorPool, nullptr);
	}

	void VKBindSet::bindBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer)
	{
		auto& entryLayouts = _layout->getEntryLayouts();
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);

		VkDescriptorBufferInfo bufferInfo = 
		{
			.buffer = vkBuffer->getBuffer(),
			.range = VK_WHOLE_SIZE
		};
		VkWriteDescriptorSet writeDescSet = 
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _descriptorSet,
			.dstBinding = binding,
			.descriptorCount = 1,
			.descriptorType = _entriesType[binding],
			.pBufferInfo = &bufferInfo
		};
		vkUpdateDescriptorSets(_device.getDevice(), 1, &writeDescSet, 0, nullptr);
	}

	void VKBindSet::bindTexture(uint32_t binding, const std::shared_ptr<TextureView>& textureView)
	{
		auto& entryLayouts = _layout->getEntryLayouts();
		auto vkTextureView = std::dynamic_pointer_cast<VKTextureView>(textureView);

		bindTexture(binding, vkTextureView->getImageView());
	}

	void VKBindSet::bindSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler)
	{
		auto& entryLayouts = _layout->getEntryLayouts();
		auto vkSampler = std::dynamic_pointer_cast<VKSampler>(sampler);

		VkDescriptorImageInfo imageInfo =
		{
			.sampler = vkSampler->getSampler()
		};
		VkWriteDescriptorSet writeDescSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _descriptorSet,
			.dstBinding = binding,
			.descriptorCount = 1,
			.descriptorType = _entriesType[binding],
			.pImageInfo = &imageInfo
		};
		vkUpdateDescriptorSets(_device.getDevice(), 1, &writeDescSet, 0, nullptr);
	}

	void VKBindSet::bindTexture(uint32_t binding, TextureView* textureView)
	{
		auto& entryLayouts = _layout->getEntryLayouts();
		auto vkTextureView = dynamic_cast<VKTextureView*>(textureView);

		bindTexture(binding, vkTextureView->getImageView());
	}

	void VKBindSet::bindTextures(uint32_t binding, const std::vector<std::shared_ptr<TextureView>>& textureViews)
	{
		std::vector<VkDescriptorImageInfo> imageInfos(textureViews.size());
		for (uint32_t i = 0; i < imageInfos.size(); i++)
		{
			auto& imageInfo = imageInfos[i];
			const auto& vkTextureView = std::dynamic_pointer_cast<VKTextureView>(textureViews[i]);
			imageInfo.imageView = vkTextureView->getImageView();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		
		VkWriteDescriptorSet writeDescSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _descriptorSet,
			.dstBinding = binding,
			.descriptorCount = (uint32_t)imageInfos.size(),
			.descriptorType = _entriesType[binding],
			.pImageInfo = imageInfos.data()
		};
		vkUpdateDescriptorSets(_device.getDevice(), 1, &writeDescSet, 0, nullptr);
	}

	void VKBindSet::bindTexture(uint32_t binding, VkImageView imageView)
	{
		VkDescriptorType type = _entriesType[binding];
		VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		}
		VkDescriptorImageInfo imageInfo =
		{
			.imageView = imageView,
			.imageLayout = imageLayout
		};
		VkWriteDescriptorSet writeDescSet =
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = _descriptorSet,
			.dstBinding = binding,
			.descriptorCount = 1,
			.descriptorType = type,
			.pImageInfo = &imageInfo
		};
		vkUpdateDescriptorSets(_device.getDevice(), 1, &writeDescSet, 0, nullptr);
	}
}
