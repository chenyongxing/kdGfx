#pragma once

#include <vulkan/vulkan.h>

#include "../Pipeline.h"

namespace kdGfx
{
    class VKDevice;

    class VKPipeline : public Pipeline
    {
    public:
        VKPipeline(VKDevice& device);
        virtual ~VKPipeline();

        inline VkPipeline getPipeline() const { return _pipeline; }
        inline VkPipelineLayout getPipelineLayout() const { return _pipelineLayout; }

    protected:
        VKDevice& _device;
        VkPipeline _pipeline = VK_NULL_HANDLE;
        VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    };

    class VKComputePipeline : public VKPipeline
    {
    public:
        VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc);

        inline const ComputePipelineDesc& getDesc() const { return _desc; }

    private:
        ComputePipelineDesc _desc;
    };

    class VKRasterPipeline : public VKPipeline
    {
    public:
        VKRasterPipeline(VKDevice& device, const RasterPipelineDesc& desc);

        inline const RasterPipelineDesc& getDesc() const { return _desc; }

    private:
        RasterPipelineDesc _desc;
    };
}
