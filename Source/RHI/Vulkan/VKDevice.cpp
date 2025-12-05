#include "VKDevice.h"
#include "VKAPI.h"
#include "VKCommandList.h"
#include "VKFence.h"
#include "VKMemory.h"
#include "VKBuffer.h"
#include "VKTexture.h"
#include "VKSwapchain.h"
#include "VKPipeline.h"
#include "VKBindSetLayout.h"
#include "VKBindSet.h"

extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddrKD;
PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameKD = NULL;
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelKD = NULL;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelKD = NULL;

static void LoadFunctions(VkDevice device)
{
	vkSetDebugUtilsObjectNameKD = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddrKD(device, "vkSetDebugUtilsObjectNameEXT");
	vkCmdBeginDebugUtilsLabelKD = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddrKD(device, "vkCmdBeginDebugUtilsLabelEXT");
	vkCmdEndDebugUtilsLabelKD = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddrKD(device, "vkCmdEndDebugUtilsLabelEXT");
}

namespace kdGfx
{
	VKDevice::VKDevice(VKAdapter& adapter) :
		_adapter(adapter)
	{
		VkPhysicalDevice physicalDevice = _adapter.getPhysicalDevice();

		// 检查支持特性
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
		for (const auto& extension : extensions)
		{
			if (strcmp(extension.extensionName, VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0)
			{
				_rayQuerySupported = true;
				break;
			}
		}

		// 队列信息
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			auto& queueFamily = queueFamilies[i];
			if (queueFamily.queueCount == 0)	continue;
			VkQueueFlags queueFlags = queueFamily.queueFlags;
			
			if (HasAllBits(queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))
			{
				_queuesInfo[CommandListType::General].queueFamilyIndex = i;
				_queuesInfo[CommandListType::General].queueCount = queueFamily.queueCount;
			}
			else if (HasAllBits(queueFlags, VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT) &&
				!HasAnyBits(queueFlags, VK_QUEUE_GRAPHICS_BIT))
			{
				_queuesInfo[CommandListType::Compute].queueFamilyIndex = i;
				_queuesInfo[CommandListType::Compute].queueCount = queueFamily.queueCount;
			}
			else if (HasAllBits(queueFlags, VK_QUEUE_TRANSFER_BIT) &&
				!HasAnyBits(queueFlags, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
			{
				_queuesInfo[CommandListType::Copy].queueFamilyIndex = i;
				_queuesInfo[CommandListType::Copy].queueCount = queueFamily.queueCount;
			}
		}
		const float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (const auto& queueInfo : _queuesInfo)
		{
			VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos.emplace_back();
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueInfo.second.queueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
		}
		
		// 硬件特性
		VkPhysicalDeviceVulkan11Features vulkan11Features =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.shaderDrawParameters = true,
		};
		VkPhysicalDeviceVulkan12Features vulkan12Features = 
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			.pNext = &vulkan11Features,
			.descriptorBindingPartiallyBound = true,
			.descriptorBindingVariableDescriptorCount = true,
			.runtimeDescriptorArray = true,
			.timelineSemaphore = true,
			.bufferDeviceAddress = true,
		};
		VkPhysicalDeviceVulkan13Features vulkan13Features =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			.pNext = &vulkan12Features,
			.dynamicRendering = true
		};

		// 可用扩展
		std::unordered_set<std::string> reqExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
			VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
			VK_KHR_RAY_QUERY_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		};
		std::vector<const char*> foundEextensions;
		for (const auto& extension : extensions)
		{
			if (reqExtensions.count(extension.extensionName)) {
				foundEextensions.push_back(extension.extensionName);
			}
		}

		VkPhysicalDeviceFeatures deviceFeatures = 
		{
			.multiDrawIndirect = true,
			.fillModeNonSolid = true,
			.samplerAnisotropy = true,
			.vertexPipelineStoresAndAtomics = true,
			.fragmentStoresAndAtomics = true
		};
		VkDeviceCreateInfo deviceCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &vulkan13Features,
			.queueCreateInfoCount = (uint32_t)queueCreateInfos.size(),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledExtensionCount = (uint32_t)foundEextensions.size(),
			.ppEnabledExtensionNames = foundEextensions.data(),
			.pEnabledFeatures = &deviceFeatures
		};
		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &_device);
		if (result != VK_SUCCESS)
		{
			RuntimeError("failed to create vulkan device");
		}
		LoadFunctions(_device);

		for (auto& queueInfo : _queuesInfo)
		{
			queueInfo.second.commandQueue = std::make_shared<VKCommandQueue>(*this, queueInfo.second.queueFamilyIndex);
		}
	}

	VKDevice::~VKDevice()
	{
		vkDestroyDevice(_device, nullptr);
	}

	std::shared_ptr<CommandQueue> VKDevice::getCommandQueue(CommandListType type)
	{
		return _queuesInfo[getAvailableCommandListType(type)].commandQueue;
	}

	size_t VKDevice::getTextureAllocateSize(const TextureDesc& desc)
	{
		VkImageCreateInfo imageCreateInfo = std::move(VKTexture::getCreateInfo(*this, desc));
		VkImage _image = VK_NULL_HANDLE;
		vkCreateImage(_device, &imageCreateInfo, nullptr, &_image);

		VkMemoryRequirements memRequirements = {};
		vkGetImageMemoryRequirements(_device, _image, &memRequirements);

		vkDestroyImage(_device, _image, nullptr);

		return memRequirements.size;
	}

	std::shared_ptr<CommandList> VKDevice::createCommandList(CommandListType type)
	{
		return std::make_shared<VKCommandList>(*this, getAvailableCommandListType(type));
	}

	std::shared_ptr<Fence> VKDevice::createFence(uint64_t initialValue)
	{
		return std::make_shared<VKFence>(*this, initialValue);
	}

	std::shared_ptr<Memory> VKDevice::createMemory(const MemoryDesc& desc)
	{
		return std::make_shared<VKMemory>(*this, desc);
	}

	std::shared_ptr<Buffer> VKDevice::createBuffer(const BufferDesc& desc)
	{
		return std::make_shared<VKBuffer>(*this, desc);
	}

	std::shared_ptr<Buffer> VKDevice::createBuffer(const BufferDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset)
	{
		auto vkMemory = std::dynamic_pointer_cast<VKMemory>(memory);
		return std::make_shared<VKBuffer>(*this, desc, vkMemory, offset);
	}

	std::shared_ptr<Texture> VKDevice::createTexture(const TextureDesc& desc)
	{
		return std::make_shared<VKTexture>(*this, desc);
	}

	std::shared_ptr<Texture> VKDevice::createTexture(const TextureDesc& desc, const std::shared_ptr<Memory>& memory, size_t offset)
	{
		auto vkMemory = std::dynamic_pointer_cast<VKMemory>(memory);
		return std::make_shared<VKTexture>(*this, desc, vkMemory, offset);
	}

	std::shared_ptr<Sampler> VKDevice::createSampler(const SamplerDesc& desc)
	{
		return std::make_shared<VKSampler>(*this, desc);
	}

	std::shared_ptr<Swapchain> VKDevice::createSwapchain(const SwapchainDesc& desc)
	{
		return std::make_shared<VKSwapchain>(*this, *_queuesInfo[CommandListType::General].commandQueue, desc);
	}

	std::shared_ptr<BindSetLayout> VKDevice::createBindSetLayout(const std::vector<BindEntryLayout>& entryLayouts)
	{
		return std::make_shared<VKBindSetLayout>(*this, entryLayouts);
	}

	std::shared_ptr<BindSet> VKDevice::createBindSet(const std::shared_ptr<BindSetLayout>& layout)
	{
		return std::make_shared<VKBindSet>(*this, layout);
	}

	std::shared_ptr<Pipeline> VKDevice::createComputePipeline(const ComputePipelineDesc& desc)
	{
		return std::make_shared<VKComputePipeline>(*this, desc);
	}

	std::shared_ptr<Pipeline> VKDevice::createRasterPipeline(const RasterPipelineDesc& desc)
	{
		return std::make_shared<VKRasterPipeline>(*this, desc);
	}

	uint32_t VKDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_adapter.getPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		spdlog::error("failed to find vulkan memory type");
		return 0;
	}

	uint32_t VKDevice::getMaxDescriptorCount(BindEntryType type)
	{
		switch (type)
		{
		case BindEntryType::ConstantBuffer:
			return _adapter.getProperties().limits.maxPerStageDescriptorUniformBuffers;
		case BindEntryType::ReadedBuffer:
		case BindEntryType::StorageBuffer:
			return _adapter.getProperties().limits.maxPerStageDescriptorStorageBuffers;
		case BindEntryType::SampledTexture:
			return _adapter.getProperties().limits.maxPerStageDescriptorSampledImages;
		case BindEntryType::StorageTexture:
			return _adapter.getProperties().limits.maxPerStageDescriptorStorageImages;
		case BindEntryType::Sampler:
			return _adapter.getProperties().limits.maxPerStageDescriptorSamplers;
		default:
			return UINT32_MAX;
		}
	}

	Format VKDevice::fromVkFormat(VkFormat format) const
	{
		static std::unordered_map<VkFormat, Format> mapping =
		{
			{ VK_FORMAT_UNDEFINED, Format::Undefined },
			{ VK_FORMAT_R8_UNORM, Format::R8Unorm },
			{ VK_FORMAT_R8G8B8A8_UNORM, Format::RGBA8Unorm },
			{ VK_FORMAT_R8G8B8A8_SRGB, Format::RGBA8Srgb },
			{ VK_FORMAT_B8G8R8A8_UNORM, Format::BGRA8Unorm },
			{ VK_FORMAT_B8G8R8A8_SRGB, Format::BGRA8Srgb },
			{ VK_FORMAT_R16G16B16A16_UNORM, Format::RGBA16Unorm },
			{ VK_FORMAT_R16G16B16A16_SFLOAT, Format::RGBA16Sfloat },
			{ VK_FORMAT_R32_SFLOAT, Format::R32Sfloat },
			{ VK_FORMAT_R32G32_SFLOAT, Format::RG32Sfloat },
			{ VK_FORMAT_R32G32B32_SFLOAT, Format::RGB32Sfloat },
			{ VK_FORMAT_R32G32B32A32_SFLOAT, Format::RGBA32Sfloat },
			{ VK_FORMAT_D16_UNORM, Format::D16Unorm },
			{ VK_FORMAT_D24_UNORM_S8_UINT, Format::D24UnormS8Uint },
			{ VK_FORMAT_D32_SFLOAT, Format::D32Sfloat },
			{ VK_FORMAT_S8_UINT, Format::S8Uint }
		};
		assert(mapping.count(format));
		return mapping[format];
	}

	VkFormat VKDevice::toVkFormat(Format format) const
	{
		static std::unordered_map<Format, VkFormat> mapping =
		{
			{ Format::Undefined, VK_FORMAT_UNDEFINED },
			{ Format::R8Unorm, VK_FORMAT_R8_UNORM },
            { Format::R16Uint, VK_FORMAT_R16_UINT },
            { Format::R32Uint, VK_FORMAT_R32_UINT },
			{ Format::RGBA8Unorm, VK_FORMAT_R8G8B8A8_UNORM },
			{ Format::RGBA8Srgb, VK_FORMAT_R8G8B8A8_SRGB },
			{ Format::BGRA8Unorm, VK_FORMAT_B8G8R8A8_UNORM },
			{ Format::BGRA8Srgb, VK_FORMAT_B8G8R8A8_SRGB },
			{ Format::RGBA16Unorm, VK_FORMAT_R16G16B16A16_UNORM },
			{ Format::RGBA16Sfloat, VK_FORMAT_R16G16B16A16_SFLOAT },
			{ Format::R32Sfloat, VK_FORMAT_R32_SFLOAT },
			{ Format::RG32Sfloat, VK_FORMAT_R32G32_SFLOAT },
			{ Format::RGB32Sfloat, VK_FORMAT_R32G32B32_SFLOAT },
			{ Format::RGBA32Sfloat, VK_FORMAT_R32G32B32A32_SFLOAT },
			{ Format::D16Unorm, VK_FORMAT_D16_UNORM },
			{ Format::D24UnormS8Uint, VK_FORMAT_D24_UNORM_S8_UINT },
			{ Format::D32Sfloat, VK_FORMAT_D32_SFLOAT },
			{ Format::S8Uint, VK_FORMAT_S8_UINT }
		};
		assert(mapping.count(format));
		return mapping[format];
	}

	VkImageUsageFlags VKDevice::toVkImageUsage(TextureUsage usage) const
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

	VkImageLayout VKDevice::toVkImageLayout(TextureState state) const
	{
		static std::unordered_map<TextureState, VkImageLayout> mapping =
		{
			{ TextureState::Undefined, VK_IMAGE_LAYOUT_UNDEFINED },
			{ TextureState::General, VK_IMAGE_LAYOUT_GENERAL },
			{ TextureState::CopyDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			{ TextureState::CopySrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			{ TextureState::ColorAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
			{ TextureState::DepthStencilRead, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
			{ TextureState::DepthStencilWrite, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
			{ TextureState::ShaderRead, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
			{ TextureState::ResolveDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
			{ TextureState::ResolveSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
			{ TextureState::Present, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR }
		};
		assert(mapping.count(state));
		return mapping[state];
	}

	VkDescriptorType VKDevice::toVkDescriptorType(BindEntryType type) const
	{
		switch (type)
		{
		case BindEntryType::ConstantBuffer:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case BindEntryType::ReadedBuffer:
		case BindEntryType::StorageBuffer:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case BindEntryType::SampledTexture:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BindEntryType::StorageTexture:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case BindEntryType::Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		}
		return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}

	VkShaderStageFlags VKDevice::toVkShaderStageFlags(ShaderType type) const
	{
		switch (type)
		{
		case ShaderType::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderType::Pixel:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderType::Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case ShaderType::Raster:
			return VK_SHADER_STAGE_ALL_GRAPHICS;
		case ShaderType::All:
			return VK_SHADER_STAGE_ALL;
		}
		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

	VkImageAspectFlags VKDevice::getAspectFlagsFromFormat(VkFormat format) const
	{
		switch (format)
		{
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case VK_FORMAT_S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}

	VkAccessFlags VKDevice::getAccessFlagsFromImageLayout(VkImageLayout layout) const
	{
		switch (layout)
		{
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			return VK_ACCESS_TRANSFER_READ_BIT;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			return VK_ACCESS_TRANSFER_WRITE_BIT;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			return VK_ACCESS_SHADER_READ_BIT;
		case VK_IMAGE_LAYOUT_GENERAL:
			return VK_ACCESS_SHADER_WRITE_BIT;
		default:
			return VK_ACCESS_NONE;
		}
	}
}
