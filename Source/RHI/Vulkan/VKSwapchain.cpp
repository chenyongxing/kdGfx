#include "VKSwapchain.h"
#include "VKDevice.h"
#include "VKFence.h"

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Windows.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xlib-xcb.h>
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <dlfcn.h>
#include <wayland-client.h>
typedef struct wl_display *(*wl_proxy_get_display_func)(struct wl_proxy *);
#endif

namespace kdGfx
{
	VKSwapchain::VKSwapchain(VKDevice& device, VKCommandQueue& queue, const SwapchainDesc& desc) :
		_device(device),
		_queue(queue)
	{
		_desc = desc;

		_getOrCreateSurface();
		_getSurfaceAttribute();

		VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
		if (!desc.oldSwapchain.expired())
		{
			auto vkSwapchain = std::dynamic_pointer_cast<VKSwapchain>(desc.oldSwapchain.lock());
			oldSwapchain = vkSwapchain->getSwapchain();
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = _surface,
			.minImageCount = std::max(_surfaceCap.minImageCount, desc.frameCount),
			.imageFormat = _surfaceFormat.format,
			.imageColorSpace = _surfaceFormat.colorSpace,
			.imageExtent = VkExtent2D{desc.width, desc.height},
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = _presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = oldSwapchain
		};
		if (vkCreateSwapchainKHR(_device.getDevice(), &swapchainCreateInfo, nullptr, &_swapchain) != VK_SUCCESS)
		{
			RuntimeError("failed to create vulkan swapchain");
		}

		// 构造颜色附加贴图
		vkGetSwapchainImagesKHR(_device.getDevice(), _swapchain, &_desc.frameCount, nullptr);
		std::vector<VkImage> images(_desc.frameCount);
		vkGetSwapchainImagesKHR(_device.getDevice(), _swapchain, &_desc.frameCount, images.data());
		_colorAttachments.resize(_desc.frameCount);
		for (uint32_t i = 0; i < _desc.frameCount; i++)
		{
			TextureDesc textureDesc =
			{
				.usage = TextureUsage::ColorAttachment | TextureUsage::CopySrc | TextureUsage::CopyDst,
				.format = _desc.format,
				.width = _desc.width,
				.height = _desc.height,
				.name = std::string("backBuffer").append(std::to_string(i))
			};
			_colorAttachments[i] = std::make_shared<VKTexture>(_device, images[i], textureDesc);
		}
		std::shared_ptr<CommandQueue> commandQueue = _device.getCommandQueue(CommandListType::General);
		std::shared_ptr<CommandList> commandList = _device.createCommandList(CommandListType::General);
		commandList->begin();
		for (uint32_t i = 0; i < _desc.frameCount; i++)
		{
			commandList->resourceBarrier({ _colorAttachments[i], TextureState::Undefined, TextureState::Present });
		}
		commandList->end();
		std::shared_ptr<Fence> fence = _device.createFence(0);
		commandQueue->submit({ commandList });
		commandQueue->signal(fence, 1);
		fence->wait(1);

		VkSemaphoreCreateInfo semaphoreCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};
		vkCreateSemaphore(_device.getDevice(), &semaphoreCreateInfo, nullptr, &_imageAvailableSemaphore);
		vkCreateSemaphore(_device.getDevice(), &semaphoreCreateInfo, nullptr, &_renderingFinishedSemaphore);
	}

	VKSwapchain::~VKSwapchain()
	{
		vkDestroySemaphore(_device.getDevice(), _imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(_device.getDevice(), _renderingFinishedSemaphore, nullptr);

		vkDestroySwapchainKHR(_device.getDevice(), _swapchain, nullptr);
	}

	std::shared_ptr<Texture> VKSwapchain::getBackBuffer(uint32_t index)
	{
		return _colorAttachments.at(index);
	}

	uint32_t VKSwapchain::nextTexture(const std::shared_ptr<Fence>& fence, uint64_t signalValue)
	{
		auto vkFence = std::dynamic_pointer_cast<VKFence>(fence);
		VkSemaphore timelineSemaphore = vkFence->getTimelineSemaphore();

        VkResult result = vkAcquireNextImageKHR(_device.getDevice(), _swapchain, 
			UINT64_MAX - 1, // 超时时间不允许设置为UINT64_MAX
			_imageAvailableSemaphore, VK_NULL_HANDLE, &_frameIndex);
		if (result != VK_SUCCESS)
		{
			VkSemaphoreSignalInfo signalInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.semaphore = timelineSemaphore,
				.value = signalValue
			};
			vkSignalSemaphore(_device.getDevice(), &signalInfo);
			return UINT32_MAX;
		}

        uint64_t waitValue = UINT64_MAX;
		VkTimelineSemaphoreSubmitInfo timelineInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount = 1,
			.pWaitSemaphoreValues = &waitValue,
			.signalSemaphoreValueCount = 1,
			.pSignalSemaphoreValues = &signalValue,
		};
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_imageAvailableSemaphore,
			.pWaitDstStageMask = &waitDstStageMask,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &timelineSemaphore
		};
		vkQueueSubmit(_queue.getQueue(), 1, &submitInfo, VK_NULL_HANDLE);

        return _frameIndex;
	}

	void VKSwapchain::present(const std::shared_ptr<Fence>& fence, uint64_t waitValue, uint64_t signalValue)
	{
		auto vkFence = std::dynamic_pointer_cast<VKFence>(fence);
		VkSemaphore timelineSemaphore = vkFence->getTimelineSemaphore();

		// 先等待队列完成
		uint64_t submitSignalValue = UINT64_MAX;
		VkTimelineSemaphoreSubmitInfo timelineInfo =
		{
			.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
			.waitSemaphoreValueCount = 1,
			.pWaitSemaphoreValues = &waitValue,
			.signalSemaphoreValueCount = 1,
			.pSignalSemaphoreValues = &submitSignalValue,
		};
		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo submitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = &timelineInfo,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &timelineSemaphore,
			.pWaitDstStageMask = &waitDstStageMask,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &_renderingFinishedSemaphore
		};
		vkQueueSubmit(_queue.getQueue(), 1, &submitInfo, VK_NULL_HANDLE);

		// 呈现并标记完成信号
		VkPresentInfoKHR presentInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &_renderingFinishedSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &_swapchain,
			.pImageIndices = &_frameIndex,
		};
		if (vkQueuePresentKHR(_queue.getQueue(), &presentInfo) != VK_SUCCESS)
		{
			VkSemaphoreSignalInfo signalInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
				.semaphore = timelineSemaphore,
				.value = signalValue
			};
			vkSignalSemaphore(_device.getDevice(), &signalInfo);
		}
		_queue.signal(fence, signalValue);
	}

	void VKSwapchain::_getOrCreateSurface()
	{
		auto& vkInstance = _device.getAdapter().getInstance();
		auto& windowSurfacesMap = vkInstance.getWindowSurfacesMap();
		if (windowSurfacesMap.count(_desc.window))
		{
			_surface = windowSurfacesMap[_desc.window];
			return;
		}

		VkInstance instance = vkInstance.getInstance();
		VkResult result = VK_SUCCESS;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VkWin32SurfaceCreateInfoKHR surfaceCreateDesc =
		{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = GetModuleHandle(nullptr),
			.hwnd = reinterpret_cast<HWND>(_desc.window)
		};
		result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateDesc, nullptr, &_surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		VkXcbSurfaceCreateInfoKHR surfaceCreateDesc =
		{
			.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			.connection = XGetXCBConnection(XOpenDisplay(nullptr)),
			.window = (xcb_window_t)reinterpret_cast<uintptr_t>(_desc.window)
		};
		result = vkCreateXcbSurfaceKHR(instance, &surfaceCreateDesc, nullptr, &_surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		wl_proxy_get_display_func wl_proxy_get_display = 
			(wl_proxy_get_display_func)dlsym(RTLD_DEFAULT, "wl_proxy_get_display");
		assert(wl_proxy_get_display);
		VkWaylandSurfaceCreateInfoKHR surfaceCreateDesc =
		{
			.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
			.display = wl_proxy_get_display(reinterpret_cast<wl_proxy*>(_desc.window)),
			.surface = reinterpret_cast<wl_surface*>(_desc.window)
		};
		result = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateDesc, nullptr, &_surface);
#endif
		if (result != VK_SUCCESS)
		{
			RuntimeError("failed to create vulkan surface");
		}
		windowSurfacesMap[_desc.window] = _surface;
	}

	void VKSwapchain::_getSurfaceAttribute()
	{
		VkPhysicalDevice physicalDevice = _device.getAdapter().getPhysicalDevice();

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &_surfaceCap);

		uint32_t formatsCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatsCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatsCount, formats.data());
		if (formatsCount == 0)
		{
			RuntimeError("failed to find vulkan surface available format");
		}

		uint32_t presentModesCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModesCount, nullptr);
		std::vector<VkPresentModeKHR> presentModes(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModesCount, presentModes.data());
		if (presentModesCount == 0)
		{
			RuntimeError("failed to find vulkan surface available present mode");
		}

		// 选择合适格式
		VkFormat requestFormat = _device.toVkFormat(_desc.format);
		std::vector<VkFormat> requestFormats =
		{
			requestFormat,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8A8_UNORM
		};
		bool findRequestFormat = false;
		for (VkFormat requestFormat : requestFormats)
		{
			auto it = std::find_if(formats.begin(), formats.end(), [requestFormat](const VkSurfaceFormatKHR& fmt)
				{
					return fmt.format == requestFormat;
				});
			if (it != formats.end())
			{
				_surfaceFormat = *it;
				findRequestFormat = true;
				break;
			}
		}
		if (!findRequestFormat)	_surfaceFormat = formats[0];
		_desc.format = _device.fromVkFormat(_surfaceFormat.format);

		// 选择合适呈现
		if (_desc.vsync)
		{
			if (std::count(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR))
				_presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			else
				_presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}
		else
		{
			if (std::count(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR))
				_presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			else
				_presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
}
