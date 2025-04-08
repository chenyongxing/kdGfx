#include <directx/d3dx12.h>

#include "DXTexture.h"
#include "DXDevice.h"
#include "Misc.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	DXTexture::DXTexture(DXDevice& device, const TextureDesc& desc) :
		_device(device)
	{
		_desc = desc;

		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		D3D12_RESOURCE_DESC resourceDesc = std::move(getResourceDesc(_device, _desc));
		_dxgiFormat = resourceDesc.Format;
		if (FAILED(_device.getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
			&resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_resource))))
		{
			spdlog::error("failed to create dx12 texture: {}", _desc.name);
			return;
		}

		_state = TextureState::General;
		_resource->SetName(StringToWString(_desc.name).c_str());
	}

	DXTexture::DXTexture(DXDevice& device, const TextureDesc& desc, const std::shared_ptr<DXMemory>& memory, size_t offset) :
		_device(device)
	{
		_desc = desc;
		
		D3D12_RESOURCE_DESC resourceDesc = std::move(getResourceDesc(_device, _desc));
		_dxgiFormat = resourceDesc.Format;
		if (FAILED(device.getDevice()->CreatePlacedResource(memory->getHeap().Get(), offset,
			&resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&_resource))))
		{
			spdlog::error("failed to create dx12 texture: {}", _desc.name);
			return;
		}

		_state = TextureState::General;
		_resource->SetName(StringToWString(_desc.name).c_str());
		_memory = memory;
		_memoryOffset = offset;
	}

	DXTexture::DXTexture(DXDevice& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const TextureDesc& desc) :
		_device(device),
		_resource(resource)
	{
		_desc = desc;
		_dxgiFormat = _device.toDxgiFormat(_desc.format);
		_state = TextureState::General;
		_resource->SetName(StringToWString(_desc.name).c_str());
	}

	std::shared_ptr<TextureView> DXTexture::createView(const TextureViewDesc& desc)
	{
		return std::make_shared<DXTextureView>(_device, *this, desc);
	}

	D3D12_RESOURCE_DESC DXTexture::getResourceDesc(DXDevice& device, const TextureDesc& desc)
	{
		DXGI_FORMAT dxgiFormat = device.toDxgiFormat(desc.format);

		if (HasAnyBits(desc.usage, TextureUsage::DepthStencilAttachment | TextureUsage::Sampled))
		{
			if (dxgiFormat == DXGI_FORMAT_D32_FLOAT)
			{
				dxgiFormat = DXGI_FORMAT_R32_TYPELESS;
			}
		}

		D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;
		if (HasAnyBits(desc.usage, TextureUsage::ColorAttachment))
		{
			resFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (HasAnyBits(desc.usage, TextureUsage::DepthStencilAttachment))
		{
			resFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (HasAnyBits(desc.usage, TextureUsage::Storage))
		{
			resFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		return CD3DX12_RESOURCE_DESC::Tex2D
		(
			dxgiFormat,
			desc.width,
			desc.height,
			std::max(desc.depth, desc.arrayLayers),
			desc.mipLevels,
			desc.sampleCount,
			0,
			resFlags
		);
	}

	DXTextureView::DXTextureView(DXDevice& device, DXTexture& texture, const TextureViewDesc& desc) :
		_device(device),
		_texture(texture)
	{
		_desc = desc;

		if (HasAnyBits(texture.getDesc().usage, TextureUsage::ColorAttachment))
		{
			_rtvID = _device.getRtvDA()->allocate();
			
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc =
			{
				.Format = texture.getDxgiFormat(),
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
				.Texture2D = { desc.baseMipLevel, desc.baseArrayLayer }
			};
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getRtvDA()->getCpuHandle(_rtvID);
			_device.getDevice()->CreateRenderTargetView(texture.getResource().Get(), &rtvDesc, cpuHandle);
		}
		else if (HasAnyBits(texture.getDesc().usage, TextureUsage::DepthStencilAttachment))
		{
			_dsvID = _device.getDsvDA()->allocate();
			
			DXGI_FORMAT format = texture.getDxgiFormat();
			if (HasAnyBits(texture.getDesc().usage, TextureUsage::DepthStencilAttachment | TextureUsage::Sampled))
			{
				if (format == DXGI_FORMAT_R32_TYPELESS)
				{
					format = DXGI_FORMAT_D32_FLOAT;
				}
			}
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = 
			{
				.Format = format,
				.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
				.Texture2D = { desc.baseMipLevel }
			};
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getDsvDA()->getCpuHandle(_dsvID);
			_device.getDevice()->CreateDepthStencilView(texture.getResource().Get(), &dsvDesc, cpuHandle);
		}
	}

	DXTextureView::~DXTextureView()
	{
		_device.getRtvDA()->free(_rtvID);
		_device.getDsvDA()->free(_dsvID);
	}

	DXSampler::DXSampler(DXDevice& device, const SamplerDesc& desc) :
		_device(device)
	{
		_desc = desc;

		_descriptorID = _device.getSamplerDA()->allocate();

		D3D12_SAMPLER_DESC samplerDesc =
		{
			.MaxAnisotropy = _desc.maxAnisotropy,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = { 0.0f, 0.0f, 0.0f, 1.0f},
			.MaxLOD = _desc.maxLOD
		};
		if (desc.filter == Filter::Linear)
		{
			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		}
		else if (desc.filter == Filter::Anisotropic)
		{
			samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		}
		if (desc.addressMode == AddressMode::Repeat)
		{
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
		else if (desc.addressMode == AddressMode::Mirror)
		{
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		}
		else if (desc.addressMode == AddressMode::Clamp)
		{
			samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _device.getSamplerDA()->getCpuHandle(_descriptorID);
		_device.getDevice()->CreateSampler(&samplerDesc, cpuHandle);
	}

	DXSampler::~DXSampler()
	{
		_device.getSamplerDA()->free(_descriptorID);
	}
}
