#pragma once

#include <vulkan/vulkan.h>

#include "../Texture.h"
#include "VKDevice.h"
#include "VKMemory.h"

namespace kdGfx
{
    class VKTexture : public Texture
    {
    public:
        VKTexture(VKDevice& device, const TextureDesc& desc);
        VKTexture(VKDevice& device, const TextureDesc& desc, const std::shared_ptr<VKMemory>& memory, size_t offset);
        VKTexture(VKDevice& device, VkImage image, const TextureDesc& desc);
        virtual ~VKTexture();

        std::shared_ptr<TextureView> createView(const TextureViewDesc& desc) override;
        static VkImageCreateInfo getCreateInfo(VKDevice& device, const TextureDesc& desc);

        inline VkImage getImage() const { return _image; }
        inline VkFormat getVkFormat() const { return _vkFormat; }

    private:
        VKDevice& _device;
        VkDeviceMemory _imageMemory = VK_NULL_HANDLE;
        VkImage _image = VK_NULL_HANDLE;
        VkFormat _vkFormat = VK_FORMAT_UNDEFINED;
		bool _outerImage = false;
    };

    class VKTextureView : public TextureView
    {
    public:
        VKTextureView(VKDevice& device, VKTexture& texture, const TextureViewDesc& desc);
        virtual ~VKTextureView();

        inline VkImageView getImageView() const { return _imageView; }
        inline const VKTexture& getTexture() const { return _texture; }

    private:
        VKDevice& _device;
        VKTexture& _texture;
        VkImageView _imageView = VK_NULL_HANDLE;
    };

    class VKSampler : public Sampler
    {
    public:
        VKSampler(VKDevice& device, const SamplerDesc& desc);
        virtual ~VKSampler();

        inline VkSampler getSampler() const { return _sampler; }

    private:
        VKDevice& _device;
        VkSampler _sampler = VK_NULL_HANDLE;
    };
}
