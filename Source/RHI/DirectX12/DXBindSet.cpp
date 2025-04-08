#include "DXBindSet.h"
#include "DXDevice.h"
#include "DXBuffer.h"
#include "DXTexture.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXBindSet::DXBindSet(DXDevice& device, const std::shared_ptr<BindSetLayout>& layout) :
		_device(device)
	{
		_layout = layout;
	}

	DXBindSet::~DXBindSet()
	{
		for (auto pair : _slotDescriptorsMap)
		{
			// sampler是外部管理的
			if (pair.first.type != DXBindSlot::Sampler)
			{
				for (auto descriptorId : pair.second)
				{
					_device.getCbvSrvUavDA()->free(descriptorId);
				}
			}
		}
	}

	void DXBindSet::bindBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer)
	{
		const auto& entryLayouts = _layout->getEntryLayouts();
		auto it = std::find_if(entryLayouts.begin(), entryLayouts.end(),
			[binding](const auto& entryLayout) {
				return entryLayout.binding == binding;
			});
		const BindEntryLayout& entryLayout = *it;
		DXBindSlot::Type slotType = DXBindSlot::getType(entryLayout.type);
		_clearSlotDescriptors(binding, slotType);

		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);
		if (!dxBuffer) return;

		DescriptorID id = _device.getCbvSrvUavDA()->allocate();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getCbvSrvUavDA()->getCpuHandle(id);
		
		if (slotType == DXBindSlot::ConstantBuffer)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cvbDesc =
			{
				.BufferLocation = dxBuffer->getResource()->GetGPUVirtualAddress(),
				.SizeInBytes = (UINT)MemAlign(dxBuffer->getSize(), 255)
			};
			_device.getDevice()->CreateConstantBufferView(&cvbDesc, cpuHandle);
		}
		else if (slotType == DXBindSlot::ShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
			{
				.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer =
				{
					.NumElements = (UINT)(dxBuffer->getSize() / dxBuffer->getDesc().stride),
					.StructureByteStride = dxBuffer->getDesc().stride
				}
			};
			_device.getDevice()->CreateShaderResourceView(dxBuffer->getResource().Get(), &srvDesc, cpuHandle);
		}
		else if (slotType == DXBindSlot::UnorderedAccess)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
			{
				.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
				.Buffer =
				{
					.NumElements = (UINT)(dxBuffer->getSize() / dxBuffer->getDesc().stride),
					.StructureByteStride = dxBuffer->getDesc().stride
				}
			};
			_device.getDevice()->CreateUnorderedAccessView(dxBuffer->getResource().Get(), nullptr, &uavDesc, cpuHandle);
		}
		else assert(false);
		
		DXBindSlot slot = std::move(_findSlot(binding, slotType));
		_slotDescriptorsMap[slot] = { id };
	}

	void DXBindSet::bindTexture(uint32_t binding, const std::shared_ptr<TextureView>& textureView)
	{
		bindTexture(binding, textureView.get());
	}

	void DXBindSet::bindSampler(uint32_t binding, const std::shared_ptr<Sampler>& sampler)
	{
		auto dxSampler = std::dynamic_pointer_cast<DXSampler>(sampler);

		DXBindSlot slot = std::move(_findSlot(binding, DXBindSlot::Sampler));
		_slotDescriptorsMap[slot] = { dxSampler->getDescriptorID() };
	}

	void DXBindSet::bindTexture(uint32_t binding, TextureView* textureView)
	{
		const auto& entryLayouts = _layout->getEntryLayouts();
		auto it = std::find_if(entryLayouts.begin(), entryLayouts.end(),
			[binding](const auto& entryLayout) {
				return entryLayout.binding == binding;
			});
		const BindEntryLayout& entryLayout = *it;
		DXBindSlot::Type slotType = DXBindSlot::getType(entryLayout.type);
		_clearSlotDescriptors(binding, slotType);

		auto dxTextureView = dynamic_cast<DXTextureView*>(textureView);
		if (!dxTextureView) return;

		DescriptorID id = _device.getCbvSrvUavDA()->allocate();
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getCbvSrvUavDA()->getCpuHandle(id);

		if (slotType == DXBindSlot::ShaderResource)
		{
			DXGI_FORMAT format = dxTextureView->getTexture().getDxgiFormat();
			if (HasAnyBits(dxTextureView->getTexture().getDesc().usage, TextureUsage::DepthStencilAttachment | TextureUsage::Sampled))
			{
				if (format == DXGI_FORMAT_R32_TYPELESS)
				{
					format = DXGI_FORMAT_R32_FLOAT;
				}
			}
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
			{
				.Format = format,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D =
				{
					.MostDetailedMip = dxTextureView->getDesc().baseMipLevel,
					.MipLevels = dxTextureView->getDesc().levelCount
				}
			};
			_device.getDevice()->CreateShaderResourceView(dxTextureView->getTexture().getResource().Get(), &srvDesc, cpuHandle);
		}
		else if (slotType == DXBindSlot::UnorderedAccess)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
			{
				.Format = dxTextureView->getTexture().getDxgiFormat(),
				.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D,
				.Texture2D =
				{
					.MipSlice = dxTextureView->getDesc().baseMipLevel
				}
			};
			_device.getDevice()->CreateUnorderedAccessView(dxTextureView->getTexture().getResource().Get(), nullptr, &uavDesc, cpuHandle);
		}
		else assert(false);

		DXBindSlot slot = std::move(_findSlot(binding, slotType));
		_slotDescriptorsMap[slot] = { id };
	}

	void DXBindSet::bindTextures(uint32_t binding, const std::vector<std::shared_ptr<TextureView>>& textureViews)
	{
		const auto& entryLayouts = _layout->getEntryLayouts();
		auto it = std::find_if(entryLayouts.begin(), entryLayouts.end(),
			[binding](const auto& entryLayout) {
				return entryLayout.binding == binding;
			});
		const BindEntryLayout& entryLayout = *it;
		DXBindSlot::Type slotType = DXBindSlot::getType(entryLayout.type);
		assert(slotType == DXBindSlot::ShaderResource);
		_clearSlotDescriptors(binding, slotType);

		// 需要保证连续分配
		std::vector<DescriptorID> ids = _device.getCbvSrvUavDA()->allocate(textureViews.size());
		for (uint32_t i = 0; i < textureViews.size(); i++)
		{
			const auto& dxTextureView = std::dynamic_pointer_cast<DXTextureView>(textureViews[i]);
			DescriptorID id = ids[i];
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getCbvSrvUavDA()->getCpuHandle(id);
			_device.getDevice()->CreateShaderResourceView(dxTextureView->getTexture().getResource().Get(), nullptr, cpuHandle);
		}

		DXBindSlot slot = std::move(_findSlot(binding, slotType));
		_slotDescriptorsMap[slot] = ids;
	}

	void DXBindSet::_clearSlotDescriptors(uint32_t binding, DXBindSlot::Type type)
	{
		// 每个bindset的binding是唯一的
		for (const auto& entryLayout : _layout->getEntryLayouts())
		{
			if (entryLayout.binding == binding)
			{
				DXBindSlot slot = 
				{
					.type = type,
					.shaderRegister = entryLayout.shaderRegister,
					.space = entryLayout.space
				};
				if (_slotDescriptorsMap.count(slot))
				{
					for (auto& descriptorId : _slotDescriptorsMap[slot])
					{
						_device.getCbvSrvUavDA()->free(descriptorId);
					}
				}
				return;
			}
		}
	}

	DXBindSlot DXBindSet::_findSlot(uint32_t binding, DXBindSlot::Type type)
	{
		DXBindSlot slot = { .type = type };
		bool findSlot = false;
		for (const auto& entryLayout : _layout->getEntryLayouts())
		{
			if (entryLayout.binding == binding)
			{
				slot.shaderRegister = entryLayout.shaderRegister;
				slot.space = entryLayout.space;
				findSlot = true;
				break;
			}
		}
		if (!findSlot)
		{
			spdlog::error("failed to find DXBindSet slot");
		}
		return slot;
	}
}
