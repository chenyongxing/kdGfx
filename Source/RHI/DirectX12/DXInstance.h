#pragma once

#include <wrl.h>
#include <dxgi1_6.h>

#include "../Instance.h"

namespace kdGfx
{
    class DXInstance : public Instance
    {
    public:
        DXInstance(bool debug);

        std::vector<std::shared_ptr<Adapter>> enumerateAdapters() override;

        inline Microsoft::WRL::ComPtr<IDXGIFactory4> getFactory() const { return _dxgiFactory; };

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
    };
}
