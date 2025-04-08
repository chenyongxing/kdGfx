#include "DXFence.h"
#include "DXDevice.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXFence::DXFence(DXDevice& device, uint64_t initialValue) :
		_device(device)
	{
		_device.getDevice()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
		_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	DXFence::~DXFence()
	{
		CloseHandle(_fenceEvent);
	}

	void DXFence::signal(uint64_t value)
	{
		_fence->Signal(value);
	}

	void DXFence::wait(uint64_t value)
	{
		_fence->SetEventOnCompletion(value, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}

	uint64_t DXFence::getCompletedValue()
	{
		return _fence->GetCompletedValue();
	}
}
