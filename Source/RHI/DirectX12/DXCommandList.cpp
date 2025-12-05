#include <directx/d3dx12.h>
#include <pix.h>

#include "DXCommandList.h"
#include "DXDevice.h"
#include "DXBuffer.h"
#include "DXTexture.h"
#include "DXPipeline.h"
#include "DXBindSet.h"
#include "DescriptorAllocator.h"
#include "Misc.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	uint32_t GetBytesPerPixel(Format format)
	{
		switch (format)
		{
		case Format::RGBA8Unorm:
		case Format::RGBA8Srgb:
		case Format::BGRA8Unorm:
		case Format::BGRA8Srgb:
		case Format::R32Uint:
		case Format::R32Sfloat:
		case Format::D32Sfloat:
		case Format::D24UnormS8Uint:
		case Format::S8Uint:
			return 4;
		case Format::R8Unorm:
			return 1;
		case Format::R16Uint:
		case Format::D16Unorm:
			return 2;
		case Format::RGBA16Unorm:
		case Format::RGBA16Sfloat:
			return 8;
		case Format::RG32Sfloat:
			return 8;
		case Format::RGB32Sfloat:
			return 12;
		case Format::RGBA32Sfloat:
			return 16;
		default:
			assert(false);
			return 0;
		}
	}

	DXCommandList::DXCommandList(DXDevice& device, CommandListType type) :
		_device(device),
		_type(type)
	{
		D3D12_COMMAND_LIST_TYPE dxType = D3D12_COMMAND_LIST_TYPE_DIRECT;
		switch (_type)
		{
		case CommandListType::Compute:
			dxType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			break;
		case CommandListType::Copy:
			dxType = D3D12_COMMAND_LIST_TYPE_COPY;
			break;
		}

		_device.getDevice()->CreateCommandAllocator(dxType, IID_PPV_ARGS(&_commandAllocator));
		_device.getDevice()->CreateCommandList(0, dxType, _commandAllocator.Get(), nullptr, IID_PPV_ARGS(&_commandList));

		_commandList.As(&_commandList4);
	}

	void DXCommandList::reset()
	{
		if (!_closed)	_commandList4->Close();
		
		_commandAllocator->Reset();
		_commandList4->Reset(_commandAllocator.Get(), nullptr);
	}

	void DXCommandList::begin()
	{
		if (_type != CommandListType::Copy)
		{
			std::vector<ID3D12DescriptorHeap*> heaps =
			{
				_device.getCbvSrvUavDA()->getDescriptorHeap().Get(),
				_device.getSamplerDA()->getDescriptorHeap().Get(),
			};
			_commandList4->SetDescriptorHeaps(heaps.size(), heaps.data());
		}
	}

	void DXCommandList::end()
	{
		_commandList4->Close();
		_closed = true;
	}

	void DXCommandList::beginLabel(const std::string& label, uint32_t color)
	{
		PIXBeginEvent(_commandList4.Get(), color, StringToWString(label).c_str());
	}

	void DXCommandList::endLabel()
	{
		PIXEndEvent(_commandList4.Get());
	}

	void DXCommandList::resourceBarrier(const TextureBarrierDesc& desc)
	{
		auto dxTexture = std::dynamic_pointer_cast<DXTexture>(desc.texture);
		if (!dxTexture) return;
		if (!dxTexture->getResource()) return;

		D3D12_RESOURCE_STATES stateBefore = _device.toDxResourceStates(desc.oldState);
		D3D12_RESOURCE_STATES stateAfter = _device.toDxResourceStates(desc.newState);
		if (stateBefore == stateAfter) return;

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition
		(
			dxTexture->getResource().Get(),
			stateBefore,
			stateAfter
		);
		_commandList4->ResourceBarrier(1, &barrier);

		dxTexture->setState(desc.newState);
	}

	void DXCommandList::setPipeline(const std::shared_ptr<Pipeline>& pipeline)
	{
		auto dxRasterPipeline = std::dynamic_pointer_cast<DXRasterPipeline>(pipeline);
		if (dxRasterPipeline)
		{
			_commandList4->SetGraphicsRootSignature(dxRasterPipeline->getRootSignature().Get());
			_commandList4->SetPipelineState(dxRasterPipeline->getPipelineState().Get());
			_commandList4->IASetPrimitiveTopology(dxRasterPipeline->getTopology());

			_stateRasterPipeline = dxRasterPipeline.get();
			_stateComputePipeline = nullptr;
		}
		else
		{
			auto dxComputePipeline = std::dynamic_pointer_cast<DXComputePipeline>(pipeline);
			if (dxComputePipeline)
			{
				_commandList4->SetComputeRootSignature(dxComputePipeline->getRootSignature().Get());
				_commandList4->SetPipelineState(dxComputePipeline->getPipelineState().Get());

				_stateComputePipeline = dxComputePipeline.get();
				_stateRasterPipeline = nullptr;
			}
			else
				assert(false);
		}

		_stateBindSets.clear();
		_stateRootDescriptorMap.clear();
	}

	void DXCommandList::setPushConstant(const void* data)
	{
		if (_stateRasterPipeline)
		{
			_commandList4->SetGraphicsRoot32BitConstants(_stateRasterPipeline->getPushConstantRootParamIndex(),
				_stateRasterPipeline->getDesc().pushConstantLayout.size / 4, data, 0);
		}
		else if (_stateComputePipeline)
		{
			_commandList4->SetComputeRoot32BitConstants(_stateComputePipeline->getPushConstantRootParamIndex(),
				_stateComputePipeline->getDesc().pushConstantLayout.size / 4, data, 0);
		}
	}

	void DXCommandList::setBindSet(uint32_t set, const std::shared_ptr<BindSet>& bindSet)
	{
		auto dxBindSet = std::dynamic_pointer_cast<DXBindSet>(bindSet);
		_stateBindSets.insert(dxBindSet.get());
	}

	void DXCommandList::beginRenderPass(const RenderPassDesc& desc)
	{
		std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> renderTargetDescs(desc.colorAttachments.size());
		for (size_t i = 0; i < renderTargetDescs.size(); i++)
		{
			const auto& attach = desc.colorAttachments[i];
			auto dxTextureView = std::dynamic_pointer_cast<DXTextureView>(attach.textureView);
			D3D12_RENDER_PASS_RENDER_TARGET_DESC& renderTargetDesc = renderTargetDescs[i];
			renderTargetDesc.cpuDescriptor = dxTextureView->getRtvCPUHandle();
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				break;
			case LoadOp::DontCare:
				renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
				break;
			default:
				renderTargetDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				renderTargetDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
				break;
			default:
				renderTargetDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				break;
			}
			memcpy(renderTargetDesc.BeginningAccess.Clear.ClearValue.Color, attach.clearValue.data(), sizeof(float) * 4);

			if (dxTextureView->getTexture().isPlaced())
			{
				_commandList4->DiscardResource(dxTextureView->getTexture().getResource().Get(), nullptr);
			}
		}

		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
		depthStencilDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
		depthStencilDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
		depthStencilDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
		if (desc.depthAttachment.textureView)
		{
			const auto& attach = desc.depthAttachment;
			auto dxTextureView = std::dynamic_pointer_cast<DXTextureView>(desc.depthAttachment.textureView);
			if (dxTextureView->getTexture().isPlaced())
			{
				_commandList4->DiscardResource(dxTextureView->getTexture().getResource().Get(), nullptr);
			}
			depthStencilDesc.cpuDescriptor = dxTextureView->getDsvCPUHandle();
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				depthStencilDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				break;
			case LoadOp::DontCare:
				depthStencilDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
				break;
			default:
				depthStencilDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				depthStencilDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
				break;
			default:
				depthStencilDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				break;
			}
			depthStencilDesc.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = attach.clearValue;
		}
		if (desc.stencilAttachment.textureView)
		{
			const auto& attach = desc.stencilAttachment;
			if (!desc.depthAttachment.textureView)
			{
				auto dxTextureView = std::dynamic_pointer_cast<DXTextureView>(desc.stencilAttachment.textureView);
				if (dxTextureView->getTexture().isPlaced())
				{
					_commandList4->DiscardResource(dxTextureView->getTexture().getResource().Get(), nullptr);
				}
				depthStencilDesc.cpuDescriptor = dxTextureView->getDsvCPUHandle();
			}
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				depthStencilDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				break;
			case LoadOp::DontCare:
				depthStencilDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
				break;
			default:
				depthStencilDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				depthStencilDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
				break;
			default:
				depthStencilDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				break;
			}
			depthStencilDesc.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = attach.clearValue;
		}

		bool hasDepthStencil = desc.depthAttachment.textureView || desc.stencilAttachment.textureView;
		_commandList4->BeginRenderPass(renderTargetDescs.size(), renderTargetDescs.data(), 
			hasDepthStencil ? &depthStencilDesc : nullptr, D3D12_RENDER_PASS_FLAG_NONE);
	}

	void DXCommandList::endRenderPass()
	{
		_commandList4->EndRenderPass();
	}

	void DXCommandList::setViewport(float x, float y, float width, float height)
	{
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = x;
		viewport.TopLeftY = y;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		_commandList4->RSSetViewports(1, &viewport);
	}

	void DXCommandList::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		D3D12_RECT rect = { long(x), long(y), long(x + width), long(y + height) };
		_commandList4->RSSetScissorRects(1, &rect);
	}

	void DXCommandList::setStencilReference(uint32_t reference)
	{
		_commandList4->OMSetStencilRef(reference);
	}

	void DXCommandList::setVertexBuffer(uint32_t slot, const std::shared_ptr<Buffer>& buffer, size_t offset)
	{
		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
		vertexBufferView.BufferLocation = dxBuffer->getResource()->GetGPUVirtualAddress() + offset;
		vertexBufferView.SizeInBytes = buffer->getSize() - offset;
		vertexBufferView.StrideInBytes = _stateRasterPipeline->getDesc().getVertexStride();
		_commandList4->IASetVertexBuffers(slot, 1, &vertexBufferView);
	}

	void DXCommandList::setIndexBuffer(const std::shared_ptr<Buffer>& buffer, size_t offset, Format indexForamt)
	{
		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);
		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = dxBuffer->getResource()->GetGPUVirtualAddress() + offset;
		indexBufferView.SizeInBytes = buffer->getSize() - offset;
		indexBufferView.Format = _device.toDxgiFormat(indexForamt);
		_commandList4->IASetIndexBuffer(&indexBufferView);
	}

	void DXCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		_applyRootDescriptorTable(_stateRasterPipeline);

		_commandList4->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void DXCommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		_applyRootDescriptorTable(_stateRasterPipeline);

		_commandList4->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void DXCommandList::drawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, uint32_t drawCount)
	{
		_applyRootDescriptorTable(_stateRasterPipeline);

		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);
		auto _commandSignature = _device.getCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, dxBuffer->getDesc().stride);

		_commandList4->ExecuteIndirect(_commandSignature, drawCount, dxBuffer->getResource().Get(), 0, nullptr, 0);
	}

	void DXCommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		_applyRootDescriptorTable(_stateComputePipeline, true);

		_commandList4->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void DXCommandList::copyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, size_t size)
	{
		auto dxBufferSrc = std::dynamic_pointer_cast<DXBuffer>(src);
		auto dxBufferDst = std::dynamic_pointer_cast<DXBuffer>(dst);

		_commandList->CopyBufferRegion(dxBufferDst->getResource().Get(), 0, 
			dxBufferSrc->getResource().Get(), 0, size);
	}

	void DXCommandList::copyBufferToTexture(const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<Texture>& texture, uint32_t mipLevel)
	{
		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);
		auto dxTexture = std::dynamic_pointer_cast<DXTexture>(texture);

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = dxTexture->getResource().Get();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = mipLevel;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		footprint.Offset = 0; 
		footprint.Footprint.Format = dxTexture->getDxgiFormat();
		footprint.Footprint.Width = texture->getWidth();
		footprint.Footprint.Height = texture->getHeight();
		footprint.Footprint.Depth = 1;
		footprint.Footprint.RowPitch = texture->getWidth() * GetBytesPerPixel(dxTexture->getFormat());

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = dxBuffer->getResource().Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = footprint;

		_commandList4->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	void DXCommandList::copyTextureToBuffer(const std::shared_ptr<Texture>& texture, const std::shared_ptr<Buffer>& buffer, uint32_t mipLevel)
	{
		auto dxBuffer = std::dynamic_pointer_cast<DXBuffer>(buffer);
		auto dxTexture = std::dynamic_pointer_cast<DXTexture>(texture);

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = dxTexture->getResource().Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		srcLocation.SubresourceIndex = mipLevel;

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		footprint.Offset = 0;
		footprint.Footprint.Format = dxTexture->getDxgiFormat();
		footprint.Footprint.Width = texture->getWidth();
		footprint.Footprint.Height = texture->getHeight();
		footprint.Footprint.Depth = 1;
		footprint.Footprint.RowPitch = texture->getWidth() * GetBytesPerPixel(dxTexture->getFormat());

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = dxBuffer->getResource().Get();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		dstLocation.PlacedFootprint = footprint;

		_commandList4->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
	}

	void DXCommandList::copyTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst, 
		glm::uvec2 size, glm::ivec2 srcOffset, glm::ivec2 dstOffset)
	{
		auto dxTextureSrc = std::dynamic_pointer_cast<DXTexture>(src);
		auto dxTextureDst = std::dynamic_pointer_cast<DXTexture>(dst);

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = dxTextureDst->getResource().Get();
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = dxTextureSrc->getResource().Get();
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_BOX srcBox = {};
		srcBox.left = srcOffset.x;
		srcBox.top = srcOffset.y;
		srcBox.right = srcOffset.x + size.x;
		srcBox.bottom = srcOffset.y + size.y;
		srcBox.front = 0;
		srcBox.back = 1;

		_commandList4->CopyTextureRegion(&dstLocation, dstOffset.x, dstOffset.y, 0, &srcLocation, &srcBox);
	}

	void DXCommandList::resolveTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst)
	{
		auto dxTextureSrc = std::dynamic_pointer_cast<DXTexture>(src);
		auto dxTextureDst = std::dynamic_pointer_cast<DXTexture>(dst);

		_commandList4->ResolveSubresource(dxTextureDst->getResource().Get(), 0,
			dxTextureSrc->getResource().Get(), 0, dxTextureDst->getDxgiFormat());
	}

	void DXCommandList::_applyRootDescriptorTable(DXPipeline* dxPipeline, bool isCompute)
	{
		if (_type == CommandListType::Copy)	return;
		if (!dxPipeline)	return;

		for (auto& bindSet : _stateBindSets)
		{
			for (const auto& entryLayout : bindSet->getBindSetLayout()->getEntryLayouts())
			{
				DXBindSlot slot = 
				{
					.shaderRegister = entryLayout.shaderRegister,
					.space = entryLayout.space
				};
				slot.setType(entryLayout.type);

				UINT rootParamIndex = dxPipeline->getSlotRootParamIndex(slot);
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = bindSet->getSlotGPUHandle(slot);
				if (_stateRootDescriptorMap.count(rootParamIndex) && _stateRootDescriptorMap[rootParamIndex].ptr == gpuHandle.ptr)
					break;

				_stateRootDescriptorMap[rootParamIndex] = gpuHandle;
				if (isCompute)
				{
					_commandList4->SetComputeRootDescriptorTable(rootParamIndex, gpuHandle);
				}
				else
				{
					_commandList4->SetGraphicsRootDescriptorTable(rootParamIndex, gpuHandle);
				}
			}
		}
	}
}
