#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../Buffer.h"
#include "DXDevice.h"
#include "DXMemory.h"

namespace kdGfx
{
    class DXBuffer : public Buffer
    {
    public:
        DXBuffer(DXDevice& device, const BufferDesc& desc);
        DXBuffer(DXDevice& device, const BufferDesc& desc, const std::shared_ptr<DXMemory>& memory, size_t offset);

        void* map() override;
        void unmap() override;

        inline Microsoft::WRL::ComPtr<ID3D12Resource> getResource() const { return _resource; }

    private:
        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12Resource> _resource;
        void* _mappedPtr = nullptr;
    };
}
