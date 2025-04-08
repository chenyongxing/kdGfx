#include "DXInstance.h"
#include "DXAdapter.h"

#include <directx/d3d12.h>

using namespace Microsoft::WRL;

// dx12 Agility SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 615; } // D3D12_SDK_VERSION
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace kdGfx
{
	DXInstance::DXInstance(bool debug)
	{
        _backend = BackendType::DirectX12;

        uint32_t flags = 0;
        if (debug && IsDebuggerPresent())
        {
            ComPtr<ID3D12Debug> debug_controller;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
			{
                debug_controller->EnableDebugLayer();
            }
            flags = DXGI_CREATE_FACTORY_DEBUG;
        }

		if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&_dxgiFactory))))
		{
			RuntimeError("failed to create dxgi factory");
		}

        namespace fs = std::filesystem;
        auto d3d12CoreDLL = fs::path(fs::path(".") / D3D12SDKPath / "D3D12Core.dll");
        if (!fs::exists(d3d12CoreDLL))
        {
            RuntimeError("failed to find d3d12 agility sdk");
        }
	}

	std::vector<std::shared_ptr<Adapter>> DXInstance::enumerateAdapters()
	{
        std::vector<std::shared_ptr<Adapter>> adapters;

        ComPtr<IDXGIFactory6> dxgi_factory6;
        _dxgiFactory.As(&dxgi_factory6);

        auto nextAdapted = [&](uint32_t adapterIndex, ComPtr<IDXGIAdapter1>& adapter)
         {
            if (dxgi_factory6)
            {
                return dxgi_factory6->EnumAdapterByGpuPreference(adapterIndex, 
                    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
            }
            else
            {
                return _dxgiFactory->EnumAdapters1(adapterIndex, &adapter);
            }
        };

        ComPtr<IDXGIAdapter1> adapter;
        for (uint32_t adapterIndex = 0; DXGI_ERROR_NOT_FOUND != nextAdapted(adapterIndex, adapter); ++adapterIndex)
        {
            adapters.emplace_back(std::make_shared<DXAdapter>(*this, adapter));
        }

        return adapters;
	}
}
