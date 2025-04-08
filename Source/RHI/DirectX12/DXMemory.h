#pragma once

#include "../Memory.h"
#include "DXDevice.h"

namespace kdGfx
{
    class DXMemory : public Memory
    {
    public:
        DXMemory(DXDevice& device, const MemoryDesc& desc);

        inline Microsoft::WRL::ComPtr<ID3D12Heap> getHeap() const { return _heap; }

    private:
        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12Heap> _heap;
    };
}
