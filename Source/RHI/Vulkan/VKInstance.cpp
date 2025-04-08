#include "VKInstance.h"
#include "VKAdapter.h"

PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerKD = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerKD = NULL;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddrKD = NULL;

static void LoadFunctions(VkInstance instance)
{
	vkCreateDebugUtilsMessengerKD = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessengerKD = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	vkGetDeviceProcAddrKD = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr");
}

namespace kdGfx
{
	VKInstance::VKInstance(bool debug)
	{
		_backend = BackendType::Vulkan;

		VkApplicationInfo applicationInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3 };

		std::vector<const char*> layers;
		if (debug)
		{
			layers.push_back("VK_LAYER_KHRONOS_validation");
		}
		
		std::vector<const char*> extensions = 
		{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
			VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
			VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
		};

		VkInstanceCreateInfo instancecreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &applicationInfo,
			.enabledLayerCount = (uint32_t)layers.size(),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = (uint32_t)extensions.size(),
			.ppEnabledExtensionNames = extensions.data(),
		};
		if (vkCreateInstance(&instancecreateInfo, nullptr, &_instance) != VK_SUCCESS)
		{
			RuntimeError("failed to create vulkan instance");
		}
		LoadFunctions(_instance);
	}

	VKInstance::~VKInstance()
	{
		for (auto& windowSurface : _windowSurfacesMap)
		{
			vkDestroySurfaceKHR(_instance, windowSurface.second, nullptr);
		}
		
		vkDestroyInstance(_instance, nullptr);
	}

	std::vector<std::shared_ptr<Adapter>> VKInstance::enumerateAdapters()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data());

		std::vector<std::shared_ptr<Adapter>> adapters;
		adapters.reserve(deviceCount);
		for (const auto& physicalDevice : physicalDevices)
		{
			adapters.emplace_back(std::make_shared<VKAdapter>(*this, physicalDevice));
		}

		return adapters;
	}
}
