#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../Fence.h"

namespace kdGfx
{
    class DXDevice;

    class DXFence : public Fence
    {
    public:
        DXFence(DXDevice& device, uint64_t initialValue);
        virtual ~DXFence();

        void signal(uint64_t value) override;
        void wait(uint64_t value) override;
        uint64_t getCompletedValue() override;

        inline Microsoft::WRL::ComPtr<ID3D12Fence> getFence() const { return _fence; }

    private:
        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12Fence> _fence;
        HANDLE _fenceEvent = nullptr;
    };
}
