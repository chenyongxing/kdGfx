#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../BindSetLayout.h"

namespace kdGfx
{
    class DXDevice;

    class DXBindSetLayout : public BindSetLayout
    {
    public:
        DXBindSetLayout(DXDevice& device, const std::vector<BindEntryLayout>& entryLayouts);

        inline uint32_t getRangesSize() const { return _bindingRangeMap.size(); }
        inline const D3D12_DESCRIPTOR_RANGE& getRange(uint32_t binding) const 
        {
            return _bindingRangeMap.at(binding);
        }

    private:
        DXDevice& _device;
        // 每个bindset的binding是唯一的，可以作为key
        std::unordered_map<uint32_t, D3D12_DESCRIPTOR_RANGE> _bindingRangeMap;
    };
}
