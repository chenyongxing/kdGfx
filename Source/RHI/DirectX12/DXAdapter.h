#pragma once

#include <wrl.h>
#include <dxgi.h>

#include "../Adapter.h"
#include "DXInstance.h"

namespace kdGfx
{
    class DXAdapter : public Adapter
    {
    public:
        DXAdapter(DXInstance& instance, const Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter);

        std::shared_ptr<Device> createDevice() override;

        inline const DXInstance& getInstance() const { return _instance; }
        inline Microsoft::WRL::ComPtr<IDXGIAdapter1> getAdapter() const { return _adapter; }

    private:
        DXInstance& _instance;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> _adapter;
    };
}
