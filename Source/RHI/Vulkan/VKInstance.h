#pragma once

#include <vulkan/vulkan.h>

#include "../Instance.h"

namespace kdGfx
{
    class VKInstance : public Instance
    {
    public:
        VKInstance(bool debug);
        virtual ~VKInstance();

        std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override;
        
        inline VkInstance getInstance() const { return _instance; }
        inline std::unordered_map<void*, VkSurfaceKHR>& getWindowSurfacesMap() const 
        {
            return _windowSurfacesMap;
        }

    private:
        VkInstance _instance = VK_NULL_HANDLE;
        mutable std::unordered_map<void*, VkSurfaceKHR> _windowSurfacesMap;
    };
}
