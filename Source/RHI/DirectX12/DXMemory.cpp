#include "DXMemory.h"
#include "Misc.h"

namespace kdGfx
{
    DXMemory::DXMemory(DXDevice& device, const MemoryDesc& desc) :
        _device(device)
    {
		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        switch (desc.type)
        {
		case MemoryType::Upload:
			heapType = D3D12_HEAP_TYPE_UPLOAD;
			break;
        case MemoryType::Readback:
            heapType = D3D12_HEAP_TYPE_READBACK;
            break;
        }

        D3D12_HEAP_DESC heapDesc =
        {
            .SizeInBytes = MemAlign(desc.size, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT),
            .Properties = { .Type = heapType },
            .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
            .Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES
        };
        _device.getDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&_heap));
		_heap->SetName(StringToWString(desc.name).c_str());
    }
}
