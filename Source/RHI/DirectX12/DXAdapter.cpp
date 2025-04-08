#include "DXAdapter.h"
#include "DXDevice.h"
#include "Misc.h"

namespace kdGfx
{
	kdGfx::DXAdapter::DXAdapter(DXInstance& instance, const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter) :
		_instance(instance),
		_adapter(adapter)
	{
		DXGI_ADAPTER_DESC desc = {};
		adapter->GetDesc(&desc);
		_name = WStringToString(desc.Description);
		_discreteGPU = desc.DedicatedVideoMemory > 0;
	}

	std::shared_ptr<Device> DXAdapter::createDevice()
	{
		return std::make_shared<DXDevice>(*this);
	}
}
