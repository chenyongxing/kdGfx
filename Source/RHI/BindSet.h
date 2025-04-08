#pragma once

#include "BaseTypes.h"
#include "Buffer.h"
#include "Texture.h"

namespace kdGfx
{
	// 绑定到管线的资源集合，给着色器使用
    class BindSet
    {
    public:
        virtual ~BindSet() = default;

        virtual void bindBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer) = 0;
        virtual void bindTexture(uint32_t binding, const std::shared_ptr<TextureView>& textureView) = 0;
        virtual void bindSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler) = 0;
        virtual void bindTexture(uint32_t binding, TextureView* textureView) = 0;
        virtual void bindTextures(uint32_t binding, const std::vector<std::shared_ptr<TextureView>>& textureViews) = 0;

        const std::shared_ptr<BindSetLayout> getBindSetLayout() const { return _layout; }

    protected:
        std::shared_ptr<BindSetLayout> _layout;
    };
}
