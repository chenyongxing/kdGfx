#include "DescriptorAllocator.h"
#include "PCH.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DescriptorAllocator::DescriptorAllocator(ComPtr<ID3D12Device> device, 
		D3D12_DESCRIPTOR_HEAP_TYPE type, UINT size) :
		_device(device),
		_type(type),
		_incrementSize(_device->GetDescriptorHandleIncrementSize(type))
	{
		expandSize(size);
	}

	void DescriptorAllocator::expandSize(UINT newSize)
	{
		if (newSize <= _size)
			return;

		bool shaderVisible = !(_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc =
		{
			.Type = _type,
			.NumDescriptors = newSize,
			.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		};
		ComPtr<ID3D12DescriptorHeap> newHeap;
		_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&newHeap));

		D3D12_CPU_DESCRIPTOR_HANDLE newCpuStart = newHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 0; i < _count - 1; ++i)
		{
			// 仅拷贝未释放的
			UINT id = i + 1;
			if (_freeList.find(id) == _freeList.end())
			{
				D3D12_CPU_DESCRIPTOR_HANDLE oldHandle = getCpuHandle(id);
				D3D12_CPU_DESCRIPTOR_HANDLE newHandle = newCpuStart;
				newHandle.ptr += i * _incrementSize;
				_device->CopyDescriptorsSimple(1, newHandle, oldHandle, _type);
			}
		}

		_descriptorHeap = newHeap;
		_cpuStartHandle = newHeap->GetCPUDescriptorHandleForHeapStart();
		if (shaderVisible)	_gpuStartHandle = newHeap->GetGPUDescriptorHandleForHeapStart();
		
		if (_size > 0)	spdlog::info("d3d12 DescriptorAllocator expand size {} -> {}", _size, newSize);
		_size = newSize;
	}

	DescriptorID DescriptorAllocator::allocate()
	{
		if (!_freeList.empty())
		{
			DescriptorID id = *_freeList.begin();
			_freeList.erase(_freeList.begin());
			return id;
		}

		if (_count > _size)
		{
			expandSize(_size * 2);
		}

		return _count++;
	}

	std::vector<DescriptorID> DescriptorAllocator::allocate(uint32_t size)
	{
		std::vector<DescriptorID> result;

		// 在freeList查找一段连续的size个可用ID
		auto it = _freeList.begin();
		while (it != _freeList.end())
		{
			DescriptorID start = *it;
			DescriptorID current = start;
			auto next = it;

			// 检查后续的ID是否连续
			for (uint32_t i = 1; i < size; i++)
			{
				next++;
				if (next == _freeList.end() || *next != current + 1)
				{
					break; //发现不连续的ID
				}
				current++;
			}

			// 如果找到了足够长的连续ID
			if (current - start + 1 == size)
			{
				for (uint32_t i = 0; i < size; i++)
				{
					result.push_back(start + i);
					_freeList.erase(start + i);
				}
				return result;
			}

			it++;
		}

		// 若freeList没有连续的块，从count处直接分配
		if (_count + size > _size)
		{
			expandSize((_count + size) * 2);
		}

		DescriptorID start = _count;
		_count += size;

		for (uint32_t i = 0; i < size; i++)
		{
			result.push_back(start + i);
		}

		return result;
	}

	void DescriptorAllocator::free(DescriptorID id)
	{
		if (id == 0) return;

		_freeList.insert(id);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::getCpuHandle(DescriptorID id)
	{
		if (id >= _count) return {};

		D3D12_CPU_DESCRIPTOR_HANDLE handle = _cpuStartHandle;
		handle.ptr += (id - 1) * _incrementSize;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::getGpuHandle(DescriptorID id)
	{
		if (id >= _count) return {};

		D3D12_GPU_DESCRIPTOR_HANDLE handle = _gpuStartHandle;
		handle.ptr += (id - 1) * _incrementSize;
		return handle;
	}
}
