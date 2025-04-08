#include <directx/d3dx12.h>

#include "DXPipeline.h"
#include "DXDevice.h"
#include "DXBindSetLayout.h"

using namespace Microsoft::WRL;

namespace kdGfx
{
	inline static D3D12_SHADER_VISIBILITY ToDxShaderVisibility(ShaderType type)
	{
		switch (type)
		{
		case ShaderType::Vertex:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderType::Pixel:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}
	}

	inline static D3D12_BLEND ToDxBlend(BlendFactor blendFactor)
	{
		switch (blendFactor)
		{
		case BlendFactor::Zero:
			return D3D12_BLEND_ZERO;
		case BlendFactor::One:
			return D3D12_BLEND_ONE;
		case BlendFactor::SrcColor:
			return D3D12_BLEND_SRC_COLOR;
		case BlendFactor::InvSrcColor:
			return D3D12_BLEND_INV_SRC_COLOR;
		case BlendFactor::SrcAlpha:
			return D3D12_BLEND_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case BlendFactor::DstColor:
			return D3D12_BLEND_DEST_COLOR;
		case BlendFactor::InvDstColor:
			return D3D12_BLEND_INV_DEST_COLOR;
		case BlendFactor::DstAlpha:
			return D3D12_BLEND_DEST_ALPHA;
		case BlendFactor::InvDstAlpha:
			return D3D12_BLEND_INV_DEST_ALPHA;
		default:
			assert(false);
			return D3D12_BLEND_ZERO;
		}
	}

	inline static D3D12_BLEND_OP ToDxBlendOp(BlendOp blendOp)
	{
		switch (blendOp)
		{
		case BlendOp::Add:
			return D3D12_BLEND_OP_ADD;
		case BlendOp::Sub:
			return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp::RevSub:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		default:
			assert(false);
			return D3D12_BLEND_OP_MAX;
		}
	}

	DXPipeline::DXPipeline(DXDevice& device) :
		_device(device)
	{
	}

	void DXPipeline::_createRootSignature(const PushConstantLayout& pushConstantLayout, const std::vector<std::shared_ptr<BindSetLayout>>& bindSetLayouts)
	{
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;

		// 创建rootSig之前保证ranges内存不被释放
		std::vector<D3D12_DESCRIPTOR_RANGE> rangesCache;
		uint32_t rangeCount = 0;
		for (const auto& bindSetLayout : bindSetLayouts)
		{
			auto dxBindSetLayout = std::dynamic_pointer_cast<DXBindSetLayout>(bindSetLayout);
			rangeCount += dxBindSetLayout->getRangesSize();
		}
		rangesCache.reserve(rangeCount);
		for (const auto& bindSetLayout : bindSetLayouts)
		{
			auto dxBindSetLayout = std::dynamic_pointer_cast<DXBindSetLayout>(bindSetLayout);
			for (const auto& entryLayout : dxBindSetLayout->getEntryLayouts())
			{
				UINT rootParamIndex = rootParams.size();
				auto& rootParam = rootParams.emplace_back();
				auto& range = rangesCache.emplace_back();
				range = dxBindSetLayout->getRange(entryLayout.binding);
				rootParam.InitAsDescriptorTable(1, &range, ToDxShaderVisibility(entryLayout.visible));

				DXBindSlot slot =
				{
					.shaderRegister = entryLayout.shaderRegister,
					.space = entryLayout.space
				};
				slot.setType(entryLayout.type);
				_slotRootParamIndexMap[slot] = rootParamIndex;
			}
		}

		// Push Constant 放到最后
		if (pushConstantLayout.size > 0)
		{
			_pushConstantRootParamIndex = rootParams.size();
			auto& pushConstantRootParam = rootParams.emplace_back();
			pushConstantRootParam.InitAsConstants(pushConstantLayout.size / 4, pushConstantLayout.shaderRegister, pushConstantLayout.space);
		}
		
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootParams.size(), rootParams.data(), 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		ComPtr<ID3DBlob> rootSignBlob;
		ComPtr<ID3DBlob> errorBlob;
		D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignBlob, &errorBlob);
		if (errorBlob)
		{
			std::string msg((const char*)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
			spdlog::error("failed to serialize root signature : \n{}", msg);
		}
		_device.getDevice()->CreateRootSignature(0,
			rootSignBlob->GetBufferPointer(), rootSignBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
	}

	DXComputePipeline::DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc) :
		DXPipeline(device),
		_desc(desc)
	{
		_createRootSignature(desc.pushConstantLayout, desc.bindSetLayouts);

		_createPipelineState();
	}

	void DXComputePipeline::_createPipelineState()
	{
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_CS cs;
		};
		PipelineStateStream psoStream =
		{
			.rootSignature = _rootSignature.Get(),
			.cs = D3D12_SHADER_BYTECODE{ _desc.shader.code, _desc.shader.codeSize },
		};

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &psoStream;
		streamDesc.SizeInBytes = sizeof(psoStream);

		ComPtr<ID3D12Device2> device2;
		_device.getDevice().As(&device2);
		ComPtr<ID3D12PipelineState> pipelineState;
		if (FAILED(device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&_pipelineState))))
		{
			spdlog::error("failed to create pipeline");
		}
	}

	DXRasterPipeline::DXRasterPipeline(DXDevice& device, const RasterPipelineDesc& desc) :
		DXPipeline(device),
		_desc(desc)
	{
		_createRootSignature(desc.pushConstantLayout, desc.bindSetLayouts);

		_createPipelineState();

		switch (desc.topology)
		{
		case Topology::PointList:
			_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		case Topology::LineList:
			_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case Topology::TriangleList:
			_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case Topology::TriangleStrip:
			_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case Topology::TriangleFan:
			_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN;
			break;
		}
	}

	void DXRasterPipeline::_createPipelineState()
	{
		if (!_desc.vertex.code || !_desc.pixel.code)
		{
			spdlog::error("pipeline shader code is null");
			return;
		}

		CD3DX12_RASTERIZER_DESC rasterizerDesc = {};
		switch (_desc.polygonMode)
		{
		case PolygonMode::Fill:
			rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
			break;
		case PolygonMode::Point:
			rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
			break;
		}
		switch (_desc.cullMode)
		{
		case CullMode::None:
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
			break;
		case CullMode::Front:
			rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;
			break;
		case CullMode::Back:
			rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
			break;
		}
		switch (_desc.frontFace)
		{
		case FrontFace::Clockwise:
			rasterizerDesc.FrontCounterClockwise = FALSE;
			break;
		case FrontFace::CounterClockwise:
			rasterizerDesc.FrontCounterClockwise = TRUE;
			break;
		}

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
		for (auto& va : _desc.vertexAttributes)
		{
			D3D12_INPUT_ELEMENT_DESC input = {};
			input.SemanticName = va.semantic.c_str();
			input.AlignedByteOffset = va.offset;
			input.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			input.Format = _device.toDxgiFormat(va.format);
			inputElementDescs.emplace_back(input);
		}

		D3D12_RT_FORMAT_ARRAY rtFormats = {};
		rtFormats.NumRenderTargets = _desc.colorFormats.size();
		for (uint32_t i = 0; i < _desc.colorFormats.size(); ++i)
		{
			rtFormats.RTFormats[i] = _device.toDxgiFormat(_desc.colorFormats[i]);
		}

		CD3DX12_BLEND_DESC blendDesc = {};
		if (_desc.colorBlends.size() < _desc.colorFormats.size())
		{
			auto temp = _desc.colorBlends;
			_desc.colorBlends.resize(_desc.colorFormats.size());
			std::copy(temp.begin(), temp.end(), _desc.colorBlends.begin());
		}
		for (uint32_t i = 0; i < _desc.colorFormats.size(); ++i)
		{
			BlendState blendState = _desc.colorBlends[i];
			D3D12_RENDER_TARGET_BLEND_DESC& rtBlendDesc = blendDesc.RenderTarget[i];
			rtBlendDesc.BlendEnable = blendState.enable;
			rtBlendDesc.SrcBlend = ToDxBlend(blendState.colorSrcFactor);
			rtBlendDesc.DestBlend = ToDxBlend(blendState.colorDstFactor);
			rtBlendDesc.BlendOp = ToDxBlendOp(blendState.colorOp);
			rtBlendDesc.SrcBlendAlpha = ToDxBlend(blendState.alphaSrcFactor);
			rtBlendDesc.DestBlendAlpha = ToDxBlend(blendState.alphaDstFactor);
			rtBlendDesc.BlendOpAlpha = ToDxBlendOp(blendState.alphaOp);
			rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		}

		CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = (_desc.depthTest != CompareOp::Never);
		switch (_desc.depthTest)
		{
		case CompareOp::Less:
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			break;
		case CompareOp::Greater:
			depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			break;
		}

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC sample;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsFormat;
			CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC blend;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 depthStencil;
		};
		PipelineStateStream psoStream =
		{
			.rootSignature = _rootSignature.Get(),
			.vs = D3D12_SHADER_BYTECODE{ _desc.vertex.code, _desc.vertex.codeSize },
			.ps = D3D12_SHADER_BYTECODE{ _desc.pixel.code, _desc.pixel.codeSize },
			.rasterizer = rasterizerDesc,
			.sample = DXGI_SAMPLE_DESC{ _desc.sampleCount, 0 },
			.inputLayout = D3D12_INPUT_LAYOUT_DESC{ inputElementDescs.data(), (UINT)inputElementDescs.size() },
			.rtFormats = rtFormats,
			.dsFormat = _device.toDxgiFormat(_desc.depthStencilFormat),
			.blend = blendDesc,
			.depthStencil = depthStencilDesc
		};

		D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
		streamDesc.pPipelineStateSubobjectStream = &psoStream;
		streamDesc.SizeInBytes = sizeof(psoStream);

		ComPtr<ID3D12Device2> device2;
		_device.getDevice().As(&device2);
		ComPtr<ID3D12PipelineState> pipelineState;
		if (FAILED(device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&_pipelineState))))
		{
			spdlog::error("failed to create pipeline");
		}
	}
}
