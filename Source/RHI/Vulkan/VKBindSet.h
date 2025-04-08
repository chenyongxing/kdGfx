#pragma once

#include <vulkan/vulkan.h>

#include "../BindSet.h"
#include "VKBindSetLayout.h"

namespace kdGfx
{
    class VKDevice;

    class VKBindSet : public BindSet
    {
    public:
        VKBindSet(VKDevice& device, const std::shared_ptr<BindSetLayout>& layout);
        virtual ~VKBindSet();

        void bindBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer) override;
        void bindTexture(uint32_t binding, const std::shared_ptr<TextureView>& textureView) override;
        void bindSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler) override;
		void bindTexture(uint32_t binding, TextureView* textureView) override;
        void bindTextures(uint32_t binding, const std::vector<std::shared_ptr<TextureView>>& textureViews) override;
        void bindTexture(uint32_t binding, VkImageView imageView);
        
        inline VkDescriptorSet getDescriptorSet() const { return _descriptorSet; }

    private:
        VKDevice& _device;
        VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
        std::unordered_map<uint32_t, VkDescriptorType> _entriesType;
    };
}
