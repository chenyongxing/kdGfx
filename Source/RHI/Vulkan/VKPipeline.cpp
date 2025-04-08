#include "VKPipeline.h"
#include "VKDevice.h"
#include "VKBindSetLayout.h"

namespace kdGfx
{
	inline static VkBlendFactor ToVkBlendFactor(BlendFactor blendFactor)
	{
		switch (blendFactor)
		{
		case BlendFactor::Zero:
			return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::One:
			return VK_BLEND_FACTOR_ONE;
		case BlendFactor::SrcColor:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::InvSrcColor:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::SrcAlpha:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::InvSrcAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DstColor:
			return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::InvDstColor:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::DstAlpha:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::InvDstAlpha:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		default:
			assert(false);
			return VK_BLEND_FACTOR_MAX_ENUM;
		}
	}

	inline static VkBlendOp ToVkBlendOp(BlendOp blendOp)
	{
		switch (blendOp)
		{
		case BlendOp::Add:
			return VK_BLEND_OP_ADD;
		case BlendOp::Sub:
			return VK_BLEND_OP_SUBTRACT;
		case BlendOp::RevSub:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		default:
			assert(false);
			return VK_BLEND_OP_MAX_ENUM;
		}
	}

	VKPipeline::VKPipeline(VKDevice& device) :
		_device(device)
	{
	}

	VKPipeline::~VKPipeline()
	{
		if(_pipeline)	vkDestroyPipeline(_device.getDevice(), _pipeline, nullptr);
		if (_pipelineLayout)	vkDestroyPipelineLayout(_device.getDevice(), _pipelineLayout, nullptr);
	}

	VKComputePipeline::VKComputePipeline(VKDevice& device, const ComputePipelineDesc& desc) :
		VKPipeline(device),
		_desc(desc)
	{
		if (!_desc.shader.code)
		{
			spdlog::error("pipeline shader code is null");
			return;
		}

		VkDevice vkDevice = _device.getDevice();

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkShaderModuleCreateInfo shaderCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = desc.shader.codeSize,
			.pCode = reinterpret_cast<const uint32_t*>(desc.shader.code)
		};
		VkResult vkResult = vkCreateShaderModule(vkDevice, &shaderCreateInfo, nullptr, &shaderModule);
		if (vkResult != VK_SUCCESS)
		{
			spdlog::error("failed to create compute shader");
			return;
		}
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = shaderModule,
			.pName = "main"
		};

		std::vector<VkDescriptorSetLayout> descSetLayouts(desc.bindSetLayouts.size());
		for (uint32_t i = 0; i < descSetLayouts.size(); ++i)
		{
			auto vkBindSetLayout = std::dynamic_pointer_cast<VKBindSetLayout>(desc.bindSetLayouts[i]);
			descSetLayouts[i] = vkBindSetLayout->getDescriptorSetLayout();
		}
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)descSetLayouts.size(),
			.pSetLayouts = descSetLayouts.data()
		};
		VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_COMPUTE_BIT, 0, desc.pushConstantLayout.size };
		if (desc.pushConstantLayout.size > 0)
		{
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		}
		vkResult = vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyShaderModule(vkDevice, shaderModule, nullptr);
			spdlog::error("failed to create pipelineLayout");
			return;
		}

		VkComputePipelineCreateInfo pipelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = shaderStageCreateInfo,
			.layout = _pipelineLayout
		};
		vkResult = vkCreateComputePipelines(vkDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &_pipeline);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyShaderModule(vkDevice, shaderModule, nullptr);
			vkDestroyPipelineLayout(vkDevice, _pipelineLayout, nullptr);
			spdlog::error("failed to create pipeline");
			return;
		}
		vkDestroyShaderModule(vkDevice, shaderModule, nullptr);
	}

	VKRasterPipeline::VKRasterPipeline(VKDevice& device, const RasterPipelineDesc& desc) :
		VKPipeline(device),
		_desc(desc)
	{
		if (!_desc.vertex.code || !_desc.pixel.code)
		{
			spdlog::error("pipeline shader code is null");
			return;
		}

		VkDevice vkDevice = _device.getDevice();

		// 顶点着色器
		VkShaderModule vertShaderModule = VK_NULL_HANDLE;
		VkShaderModuleCreateInfo vertShaderCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = desc.vertex.codeSize,
			.pCode = reinterpret_cast<const uint32_t*>(desc.vertex.code)
		};
		VkResult vkResult = vkCreateShaderModule(vkDevice, &vertShaderCreateInfo, nullptr, &vertShaderModule);
		if (vkResult != VK_SUCCESS)
		{
			spdlog::error("failed to create vertex shader");
			return;
		}
		VkPipelineShaderStageCreateInfo vertShaderStageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertShaderModule,
			.pName = "main"
		};
		// 片段着色器
		VkShaderModule fragShaderModule = VK_NULL_HANDLE;
		VkShaderModuleCreateInfo fragShaderCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = desc.pixel.codeSize,
			.pCode = reinterpret_cast<const uint32_t*>(desc.pixel.code)
		};
		vkResult = vkCreateShaderModule(vkDevice, &fragShaderCreateInfo, nullptr, &fragShaderModule);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
			spdlog::error("failed to create fragment shader");
			return;
		}
		VkPipelineShaderStageCreateInfo fragShaderStageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragShaderModule,
			.pName = "main"
		};
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 顶点输入
		uint32_t vertexStride = desc.getVertexStride();
		VkVertexInputBindingDescription vertexBindDesc = 
		{
			.stride = vertexStride,
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		std::vector<VkVertexInputAttributeDescription> vertexAttributeDescs;
		for (auto& input : desc.vertexAttributes)
		{
			vertexAttributeDescs.emplace_back(VkVertexInputAttributeDescription
				{
					.location = input.location,
					.format = _device.toVkFormat(input.format),
					.offset = input.offset
				});
		}
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescs.size(),
			.pVertexAttributeDescriptions = vertexAttributeDescs.data()
		};
		if (vertexStride > 0)
		{
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &vertexBindDesc;
		}
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		switch (desc.topology)
		{
		case Topology::PointList:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case Topology::LineList:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		case Topology::TriangleList:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		case Topology::TriangleStrip:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			break;
		case Topology::TriangleFan:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			break;
		}
		
		// 视口和裁剪区域。需要绘制命令动态修改
		VkPipelineViewportStateCreateInfo viewportState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1
		};
		
		// 光栅化状态
		VkPipelineRasterizationStateCreateInfo rasterizerState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.lineWidth = desc.lineWidth
		};
		switch (desc.polygonMode)
		{
		case PolygonMode::Fill:
			rasterizerState.polygonMode = VK_POLYGON_MODE_FILL;
			break;
		case PolygonMode::Line:
			rasterizerState.polygonMode = VK_POLYGON_MODE_LINE;
			break;
		}
		switch (desc.cullMode)
		{
		case CullMode::None:
			rasterizerState.cullMode = VK_CULL_MODE_NONE;
			break;
		case CullMode::Front:
			rasterizerState.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		case CullMode::Back:
			rasterizerState.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		}
		switch (desc.frontFace)
		{
		case FrontFace::Clockwise:
			rasterizerState.frontFace = VK_FRONT_FACE_CLOCKWISE;
			break;
		case FrontFace::CounterClockwise:
			rasterizerState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			break;
		}

		// 多重采样
		VkPipelineMultisampleStateCreateInfo multisampleState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = (VkSampleCountFlagBits)desc.sampleCount,
			.sampleShadingEnable = VK_FALSE
		};
		
		VkPipelineDepthStencilStateCreateInfo depthStencilState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = desc.depthTest.enable,
			.depthWriteEnable = desc.depthStencilWrite,
			.stencilTestEnable = desc.stencilTest.enable
		};
		switch (desc.depthTest.compareOp)
		{
		case CompareOp::Always:
			depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
			break;
		case CompareOp::Less:
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
			break;
		case CompareOp::Greater:
			depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
			break;
		case CompareOp::Equal:
			depthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
			break;
		}
		switch (desc.stencilTest.failOp)
		{
		case StencilOp::Keep:
			depthStencilState.front.failOp = VK_STENCIL_OP_KEEP;
			break;
		case StencilOp::Replace:
			depthStencilState.front.failOp = VK_STENCIL_OP_REPLACE;
			break;
		case StencilOp::Zero:
			depthStencilState.front.failOp = VK_STENCIL_OP_ZERO;
			break;
		case StencilOp::IncrClamp:
			depthStencilState.front.failOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case StencilOp::DecrClamp:
			depthStencilState.front.failOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case StencilOp::IncrWrap:
			depthStencilState.front.failOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case StencilOp::DecrWrap:
			depthStencilState.front.failOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
		}
		switch (desc.stencilTest.passOp)
		{
		case StencilOp::Keep:
			depthStencilState.front.passOp = VK_STENCIL_OP_KEEP;
			break;
		case StencilOp::Replace:
			depthStencilState.front.passOp = VK_STENCIL_OP_REPLACE;
			break;
		case StencilOp::Zero:
			depthStencilState.front.passOp = VK_STENCIL_OP_ZERO;
			break;
		case StencilOp::IncrClamp:
			depthStencilState.front.passOp = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			break;
		case StencilOp::DecrClamp:
			depthStencilState.front.passOp = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			break;
		case StencilOp::IncrWrap:
			depthStencilState.front.passOp = VK_STENCIL_OP_INCREMENT_AND_WRAP;
			break;
		case StencilOp::DecrWrap:
			depthStencilState.front.passOp = VK_STENCIL_OP_DECREMENT_AND_WRAP;
			break;
		}
		depthStencilState.front.depthFailOp = VK_STENCIL_OP_KEEP;
		switch (desc.stencilTest.compareOp)
		{
		case CompareOp::Always:
			depthStencilState.front.compareOp = VK_COMPARE_OP_ALWAYS;
			break;
		case CompareOp::Less:
			depthStencilState.front.compareOp = VK_COMPARE_OP_LESS;
			break;
		case CompareOp::Greater:
			depthStencilState.front.compareOp = VK_COMPARE_OP_GREATER;
			break;
		case CompareOp::Equal:
			depthStencilState.front.compareOp = VK_COMPARE_OP_EQUAL;
			break;
		}
		depthStencilState.front.compareMask = 0xFF;
		depthStencilState.front.writeMask = desc.depthStencilWrite ? 0xFF : 0;
		depthStencilState.back = depthStencilState.front;
		
		// 混合状态
		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(desc.colorFormats.size());
		if (desc.colorWrites.size() < desc.colorFormats.size())
		{
			_desc.colorWrites.resize(desc.colorFormats.size());
			std::fill(_desc.colorWrites.begin(), _desc.colorWrites.end(), true);
		}
		if (desc.colorBlends.size() < desc.colorFormats.size())
		{
			_desc.colorBlends.resize(desc.colorFormats.size());
			std::copy(desc.colorBlends.begin(), desc.colorBlends.end(), _desc.colorBlends.begin());
		}
		for (uint32_t i = 0; i < colorBlendAttachments.size(); ++i)
		{
			BlendState blendState = _desc.colorBlends[i];
			auto& colorBlendAttachment = colorBlendAttachments[i];
			colorBlendAttachment.blendEnable = blendState.enable;
			colorBlendAttachment.srcColorBlendFactor = ToVkBlendFactor(blendState.colorSrcFactor);
			colorBlendAttachment.dstColorBlendFactor = ToVkBlendFactor(blendState.colorDstFactor);
			colorBlendAttachment.colorBlendOp = ToVkBlendOp(blendState.colorOp);
			colorBlendAttachment.srcAlphaBlendFactor = ToVkBlendFactor(blendState.alphaSrcFactor);
			colorBlendAttachment.dstAlphaBlendFactor = ToVkBlendFactor(blendState.alphaDstFactor);
			colorBlendAttachment.alphaBlendOp = ToVkBlendOp(blendState.alphaOp);
			colorBlendAttachment.colorWriteMask = _desc.colorWrites[i] ? VK_COLOR_COMPONENT_R_BIT |
				VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT : 0;
		}
		VkPipelineColorBlendStateCreateInfo colorBlendState =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = (uint32_t)colorBlendAttachments.size(),
			.pAttachments = colorBlendAttachments.data()
		};
		
		// 动态管线状态
		std::vector<VkDynamicState> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE
		};
		VkPipelineDynamicStateCreateInfo dynamicState = 
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
		};
		
		// 管线资源访问布局
		std::vector<VkDescriptorSetLayout> descSetLayouts(desc.bindSetLayouts.size());
		for (uint32_t i = 0; i < descSetLayouts.size(); ++i)
		{
			auto vkBindSetLayout = std::dynamic_pointer_cast<VKBindSetLayout>(desc.bindSetLayouts[i]);
			descSetLayouts[i] = vkBindSetLayout->getDescriptorSetLayout();
		}
		VkPipelineLayoutCreateInfo pipelineLayoutInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (uint32_t)descSetLayouts.size(),
			.pSetLayouts = descSetLayouts.data()
		};
		VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_ALL_GRAPHICS, 0, desc.pushConstantLayout.size };
		if (desc.pushConstantLayout.size > 0)
		{
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		}
		vkResult = vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
			vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
			spdlog::error("failed to create pipelineLayout");
			return;
		}

		std::vector<VkFormat> colorAttachFormats(desc.colorFormats.size());
		for (uint32_t i = 0; i < colorAttachFormats.size(); ++i)
		{
			colorAttachFormats[i] = _device.toVkFormat(desc.colorFormats[i]);
		}
		VkFormat depthStencilFormat = _device.toVkFormat(desc.depthStencilFormat);
		bool hasDepth = desc.depthStencilFormat == Format::D16Unorm || 
			desc.depthStencilFormat == Format::D24UnormS8Uint || desc.depthStencilFormat == Format::D32Sfloat;
		bool hasStencil =  desc.depthStencilFormat == Format::D24UnormS8Uint || desc.depthStencilFormat == Format::S8Uint;
		VkPipelineRenderingCreateInfo renderingCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = (uint32_t)colorAttachFormats.size(),
			.pColorAttachmentFormats = colorAttachFormats.data(),
			.depthAttachmentFormat = hasDepth ? depthStencilFormat : VK_FORMAT_UNDEFINED,
			.stencilAttachmentFormat = hasStencil ? depthStencilFormat : VK_FORMAT_UNDEFINED
		};
		
		// 图形管线
		VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineInfo.pNext = &renderingCreateInfo;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizerState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = _pipelineLayout;
		vkResult = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);
		if (vkResult != VK_SUCCESS)
		{
			vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
			vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
			vkDestroyPipelineLayout(vkDevice, _pipelineLayout, nullptr);
			spdlog::error("failed to create pipeline");
			return;
		}

		// 构建完毕管线shader可以删除
		vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
		vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
	}
}
