#pragma once

#include <vector>
#include <set>
#include <wrl.h>
#include <directx/d3d12.h>

namespace kdGfx
{
    using DescriptorID = UINT;
    class DescriptorAllocator final
    {
    public:
        DescriptorAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT size);

        void expandSize(UINT newSize);
        DescriptorID allocate();
        // 连续分配
        std::vector<DescriptorID> allocate(uint32_t size);
        void free(DescriptorID descriptor);
        D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(DescriptorID id);
        D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(DescriptorID id);

        inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> getDescriptorHeap() const { return _descriptorHeap; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Device> _device;
        D3D12_DESCRIPTOR_HEAP_TYPE _type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        UINT _size = 0;
        DescriptorID _count = 1;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _descriptorHeap;
        UINT _incrementSize = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE _cpuStartHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE _gpuStartHandle = {};
        std::set<DescriptorID> _freeList;
    };
}
