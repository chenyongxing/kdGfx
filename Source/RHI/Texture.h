#pragma once

#include "Memory.h"

namespace kdGfx
{
    class TextureView;
    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual std::shared_ptr<TextureView> createView(const TextureViewDesc& desc) = 0;

        inline const TextureDesc& getDesc() const { return _desc; }
        inline Format getFormat() const { return _desc.format; }
        inline uint32_t getWidth() const { return _desc.width; }
        inline uint32_t getHeight() const { return _desc.height; }
        // 当前追踪状态。跨命令切换状态需要确认前面状态执行完毕
        inline TextureState getState() const { return _state; }
        inline void setState(TextureState state) const { _state = state; }
		// 是否在外部分配的显存
        inline bool isPlaced() const { return !_memory.expired(); }
        inline std::shared_ptr<Memory> getMemory() const { return _memory.lock(); }

    protected:
        TextureDesc _desc;
        mutable TextureState _state = TextureState::Undefined;
		std::weak_ptr<Memory> _memory;
		size_t _memoryOffset = 0;
    };

    class TextureView
    {
    public:
        virtual ~TextureView() = default;

        inline const TextureViewDesc& getDesc() const { return _desc; }

    protected:
        TextureViewDesc _desc;
    };

    class Sampler
    {
    public:
        virtual ~Sampler() = default;

        inline const SamplerDesc& getDesc() const { return _desc; }

    protected:
        SamplerDesc _desc;
    };
}
