#include <directx/d3dx12.h>

#include "DXBuffer.h"
#include "DXDevice.h"
#include "Misc.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
    DXBuffer::DXBuffer(DXDevice& device, const BufferDesc& desc) :
        _device(device)
    {
        _desc = desc;

        D3D12_HEAP_PROPERTIES heapProperties;
        switch (_desc.hostVisible) {
            case HostVisible::Upload:
                heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                break;
			case HostVisible::Readback: 
                heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
                break;
            default:
				heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			    break;
        }
        
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (HasAnyBits(desc.usage, BufferUsage::Storage))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(MemAlign(_desc.size, 255), flags);
        if (FAILED(_device.getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, 
            &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_resource))))
        {
            spdlog::error("failed to create dx12 buffer: {}", _desc.name);
            return;
        }
        _resource->SetName(StringToWString(_desc.name).c_str());
    }

    DXBuffer::DXBuffer(DXDevice& device, const BufferDesc& desc, const std::shared_ptr<DXMemory>& memory, size_t offset) :
        _device(device)
    {
        _desc = desc;

        D3D12_HEAP_PROPERTIES heapProperties;
        switch (_desc.hostVisible) {
        case HostVisible::Upload:
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            break;
        case HostVisible::Readback:
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
            break;
        default:
            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            break;
        }

        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
        if (HasAnyBits(desc.usage, BufferUsage::Storage))
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }
        D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(MemAlign(_desc.size, 255), flags);
        if (FAILED(device.getDevice()->CreatePlacedResource(memory->getHeap().Get(), offset, &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_resource))))
        {
            spdlog::error("failed to create dx12 buffer: {}", _desc.name);
            return;
        }
        _resource->SetName(StringToWString(_desc.name).c_str());

        _memory = memory;
        _memoryOffset = offset;
    }

    void* DXBuffer::map()
    {
        if (_mappedPtr == nullptr && (_desc.hostVisible != HostVisible::Invisible))
        {
            CD3DX12_RANGE readRange(0, 0);
            _resource->Map(0, &readRange, &_mappedPtr);
        }
        return _mappedPtr;
    }

    void DXBuffer::unmap()
    {
        if (_mappedPtr && (_desc.hostVisible != HostVisible::Invisible))
        {
            CD3DX12_RANGE writtenRange(0, _desc.size);
            _resource->Unmap(0, &writtenRange);
            _mappedPtr = nullptr;
        }
    }
}
