#pragma once

#include <vulkan/vulkan.h>

#include "../BindSetLayout.h"

namespace kdGfx
{
    class VKDevice;

    class VKBindSetLayout : public BindSetLayout
    {
    public:
        VKBindSetLayout(VKDevice& device, const std::vector<BindEntryLayout>& entryLayouts);
        virtual ~VKBindSetLayout();

        inline VkDescriptorSetLayout getDescriptorSetLayout() const { return _descriptorSetLayout; }

    private:
        VKDevice& _device;
        VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE;
    };
}
