#pragma once

#include <wrl.h>
#include <directx/d3d12.h>

#include "../Pipeline.h"
#include "DXBindSet.h"

namespace kdGfx
{
    class DXDevice;

    class DXPipeline : public Pipeline
    {
    public:
        DXPipeline(DXDevice& device);
        virtual ~DXPipeline() = default;

        inline Microsoft::WRL::ComPtr<ID3D12PipelineState> getPipelineState() const { return _pipelineState; }
        inline Microsoft::WRL::ComPtr<ID3D12RootSignature> getRootSignature() const { return _rootSignature; }
        inline UINT getPushConstantRootParamIndex() const { return _pushConstantRootParamIndex; }
        inline UINT getSlotRootParamIndex(const DXBindSlot& slot) const { return _slotRootParamIndexMap.at(slot); }

    protected:
        void _createRootSignature(const PushConstantLayout& pushConstantLayout, const std::vector<std::shared_ptr<BindSetLayout>>& bindSetLayouts);
        virtual void _createPipelineState() = 0;

        DXDevice& _device;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
        UINT _pushConstantRootParamIndex = 0;
        std::map<DXBindSlot, UINT> _slotRootParamIndexMap;
    };

    class DXComputePipeline : public DXPipeline
    {
    public:
        DXComputePipeline(DXDevice& device, const ComputePipelineDesc& desc);

        inline const ComputePipelineDesc& getDesc() const { return _desc; }

    private:
        void _createPipelineState() override;

        ComputePipelineDesc _desc;
    };

    class DXRasterPipeline : public DXPipeline
    {
    public:
        DXRasterPipeline(DXDevice& device, const RasterPipelineDesc& desc);

        inline const RasterPipelineDesc& getDesc() const { return _desc; }
        inline D3D_PRIMITIVE_TOPOLOGY getTopology() const { return _topology; }

    private:
        void _createPipelineState() override;

        RasterPipelineDesc _desc;
        D3D_PRIMITIVE_TOPOLOGY _topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    };
}
