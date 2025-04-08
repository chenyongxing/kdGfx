#include "VKTexture.h"
#include "VKDevice.h"
#include "VKAPI.h"

namespace kdGfx
{
	static VkImageUsageFlags TextureUsageToVkImageUsage(TextureUsage usage)
	{
		VkImageUsageFlags vkUsage = 0;

		if (usage & TextureUsage::CopyDst || usage & TextureUsage::ResolveDst)
		{
			vkUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		if (usage & TextureUsage::CopySrc || usage & TextureUsage::ResolveSrc)
		{
			vkUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		if (usage & TextureUsage::Sampled)
		{
			vkUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (usage & TextureUsage::Storage)
		{
			vkUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		if (usage & TextureUsage::ColorAttachment)
		{
			vkUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (usage & TextureUsage::DepthStencilAttachment)
		{
			vkUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		return vkUsage;
	}

	VKTexture::VKTexture(VKDevice& device, const TextureDesc& desc) :
		_device(device)
	{
		_desc = desc;
		
		VkImageCreateInfo imageCreateInfo = std::move(getCreateInfo(_device, _desc));
		_vkFormat = imageCreateInfo.format;
		if (vkCreateImage(device.getDevice(), &imageCreateInfo, nullptr, &_image) != VK_SUCCESS)
		{
			spdlog::error("failed to create vulkan image: {}", desc.name);
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device.getDevice(), _image, &memRequirements);
		VkMemoryAllocateInfo allocInfo =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		};
		if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS)
		{
			RuntimeError("failed to allocate vulkan image memory");
		}
		vkBindImageMemory(device.getDevice(), _image, _imageMemory, 0);

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}
	}

	VKTexture::VKTexture(VKDevice& device, const TextureDesc& desc, const std::shared_ptr<VKMemory>& memory, size_t offset) :
		_device(device)
	{
		_desc = desc;
		
		VkImageCreateInfo imageCreateInfo = std::move(getCreateInfo(_device, _desc));
		_vkFormat = imageCreateInfo.format;
		if (vkCreateImage(device.getDevice(), &imageCreateInfo, nullptr, &_image) != VK_SUCCESS)
		{
			spdlog::error("failed to create vulkan image: {}", _desc.name);
		}

		vkBindImageMemory(device.getDevice(), _image, memory->getMemory(), offset);

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}

		_memory = memory;
		_memoryOffset = offset;
	}

	VKTexture::VKTexture(VKDevice& device, VkImage image, const TextureDesc& desc) :
		_device(device),
		_image(image)
	{
		_outerImage = true;
		_desc = desc;
		_vkFormat = device.toVkFormat(_desc.format);

		if (!_desc.name.empty())
		{
			VkDebugUtilsObjectNameInfoEXT debugNameInfo =
			{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
				.objectType = VK_OBJECT_TYPE_IMAGE,
				.objectHandle = (uint64_t)_image,
				.pObjectName = _desc.name.c_str()
			};
			vkSetDebugUtilsObjectNameKD(_device.getDevice(), &debugNameInfo);
		}
	}

	VKTexture::~VKTexture()
	{
		if (_imageMemory)
			vkFreeMemory(_device.getDevice(), _imageMemory, nullptr);
		if (!_outerImage)
			vkDestroyImage(_device.getDevice(), _image, nullptr);
	}

	std::shared_ptr<TextureView> VKTexture::createView(const TextureViewDesc& desc)
	{
		return std::make_shared<VKTextureView>(_device, *this, desc);
	}

	VkImageCreateInfo VKTexture::getCreateInfo(VKDevice& device, const TextureDesc& desc)
	{
		VkImageCreateInfo imageCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.mipLevels = desc.mipLevels,
			.arrayLayers = desc.arrayLayers,
			.samples = static_cast<VkSampleCountFlagBits>(desc.sampleCount),
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		imageCreateInfo.format = device.toVkFormat(desc.format);
		imageCreateInfo.extent.width = desc.width;
		imageCreateInfo.extent.height = desc.height;
		imageCreateInfo.extent.depth = desc.depth;
		imageCreateInfo.usage = TextureUsageToVkImageUsage(desc.usage);

		switch (desc.type)
		{
		case TextureType::e1D:
			imageCreateInfo.imageType = VK_IMAGE_TYPE_1D;
			break;
		case TextureType::e2D:
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case TextureType::e3D:
			imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
			break;
		}

		if (imageCreateInfo.arrayLayers % 6 == 0)
		{
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		return imageCreateInfo;
	}

	VKTextureView::VKTextureView(VKDevice& device, VKTexture& texture, const TextureViewDesc& desc) :
		_device(device),
		_texture(texture)
	{
		_desc = desc;

		VkImageViewCreateInfo viewCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = texture.getImage(),
			.format = texture.getVkFormat()
		};
		switch (texture.getDesc().type)
		{
		case TextureType::e1D:
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
			break;
		case TextureType::e2D:
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			break;
		case TextureType::e3D:
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
			break;
		}
		viewCreateInfo.subresourceRange.aspectMask = _device.getAspectFlagsFromFormat(viewCreateInfo.format);
		viewCreateInfo.subresourceRange.baseMipLevel = desc.baseMipLevel;
		viewCreateInfo.subresourceRange.levelCount = desc.levelCount;
		viewCreateInfo.subresourceRange.baseArrayLayer = desc.baseArrayLayer;
		viewCreateInfo.subresourceRange.layerCount = desc.layerCount;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		vkCreateImageView(_device.getDevice(), &viewCreateInfo, nullptr, &_imageView);
	}

	VKTextureView::~VKTextureView()
	{
		vkDestroyImageView(_device.getDevice(), _imageView, nullptr);
	}

	VKSampler::VKSampler(VKDevice& device, const SamplerDesc& desc) :
		_device(device)
	{
		_desc = desc;

		VkSamplerCreateInfo samplerCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.maxAnisotropy = (float)_desc.maxAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.maxLod = _desc.maxLOD,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK
		};
		if (_desc.filter == Filter::Linear)
		{
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
		else if (_desc.filter == Filter::Anisotropic)
		{
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.anisotropyEnable = true;
		}
		
		if (_desc.addressMode == AddressMode::Mirror)
		{
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		}
		else if (_desc.addressMode == AddressMode::Clamp)
		{
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
		vkCreateSampler(_device.getDevice(), &samplerCreateInfo, nullptr, &_sampler);
	}

	VKSampler::~VKSampler()
	{
		vkDestroySampler(_device.getDevice(), _sampler, nullptr);
	}
}
