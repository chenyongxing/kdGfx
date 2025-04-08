#include "DXCommandQueue.h"
#include "DXDevice.h"
#include "DXFence.h"
#include "DXCommandList.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXCommandQueue::DXCommandQueue(DXDevice& device, CommandListType type) :
		_device(device)
	{
        D3D12_COMMAND_LIST_TYPE dxType = D3D12_COMMAND_LIST_TYPE_DIRECT;
        switch (type)
		{
        case CommandListType::Compute:
            dxType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;
        case CommandListType::Copy:
            dxType = D3D12_COMMAND_LIST_TYPE_COPY;
            break;
        }
        D3D12_COMMAND_QUEUE_DESC queueDesc = 
		{
			.Type = dxType,
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH
		};
		if (FAILED(_device.getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue))))
		{
			RuntimeError("failed to create d3d12 commandQueue");
		}
	}

	void DXCommandQueue::signal(const std::shared_ptr<Fence>& fence, uint64_t value)
	{
		auto dxFence = std::dynamic_pointer_cast<DXFence>(fence);
		if (!dxFence) return;
		if (!dxFence->getFence()) return;

		_commandQueue->Signal(dxFence->getFence().Get(), value);
	}

	void DXCommandQueue::wait(const std::shared_ptr<Fence>& fence, uint64_t value)
	{
		auto dxFence = std::dynamic_pointer_cast<DXFence>(fence);
		if (!dxFence) return;
		if (!dxFence->getFence()) return;

		_commandQueue->Wait(dxFence->getFence().Get(), value);
	}

	void DXCommandQueue::waitIdle()
	{
		uint64_t fenceValue = 0;

		ComPtr<ID3D12Fence> fence;
		_device.getDevice()->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

		_commandQueue->Signal(fence.Get(), ++fenceValue);
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);

		CloseHandle(fenceEvent);
	}

	void DXCommandQueue::submit(const std::vector<std::shared_ptr<CommandList>>& commandLists)
	{
		std::vector<ID3D12CommandList*> dxCommandLists;
		for (auto& commandList : commandLists)
		{
			auto dxCommandList = std::dynamic_pointer_cast<DXCommandList>(commandList);
			if (!dxCommandList) continue;
			if (!dxCommandList->getCommandList()) continue;
			dxCommandLists.emplace_back(dxCommandList->getCommandList().Get());
		}
		_commandQueue->ExecuteCommandLists(dxCommandLists.size(), dxCommandLists.data());
	}
}
