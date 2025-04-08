#include <directx/d3dx12.h>

#include "DXBindSetLayout.h"
#include "DXDevice.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXBindSetLayout::DXBindSetLayout(DXDevice& device, const std::vector<BindEntryLayout>& entryLayouts) :
		_device(device)
	{
		_entryLayouts = entryLayouts;

		for (const auto& entryLayout : _entryLayouts)
		{
			D3D12_DESCRIPTOR_RANGE range = 
			{
				.NumDescriptors = entryLayout.count,
				.BaseShaderRegister = entryLayout.shaderRegister,
				.RegisterSpace = entryLayout.space
			};
			switch (entryLayout.type)
			{
			case BindEntryType::ConstantBuffer:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				break;
			case BindEntryType::ReadedBuffer:
			case BindEntryType::SampledTexture:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				break;
			case BindEntryType::StorageBuffer:
			case BindEntryType::StorageTexture:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				break;
			case BindEntryType::Sampler:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
				break;
			default:
				assert(false);
				break;
			}
			// bindless
			if (entryLayout.count == UINT32_MAX)
			{
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
			_bindingRangeMap[entryLayout.binding] = range;
		}
	}
}
