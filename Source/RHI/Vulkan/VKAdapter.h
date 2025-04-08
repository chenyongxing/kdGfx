#pragma once

#include <vulkan/vulkan.h>

#include "../Adapter.h"
#include "VKInstance.h"

namespace kdGfx
{
    class VKAdapter : public Adapter
    {
    public:
        VKAdapter(VKInstance& instance, VkPhysicalDevice physicalDevice);

        std::shared_ptr<Device> createDevice() override;

        inline const VKInstance& getInstance() const { return _instance; }
        inline VkPhysicalDevice getPhysicalDevice() const { return _physicalDevice; }
        inline const VkPhysicalDeviceProperties& getProperties() const { return _properties; }

    private:
        VKInstance& _instance;
        VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties _properties;
    };
}
