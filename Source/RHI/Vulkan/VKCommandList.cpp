#include "VKCommandList.h"
#include "VKAPI.h"
#include "VKDevice.h"
#include "VKPipeline.h"
#include "VKBindSet.h"
#include "VKBuffer.h"
#include "VKTexture.h"

namespace kdGfx
{
	VKCommandList::VKCommandList(VKDevice& device, CommandListType type) :
		_device(device)
	{
		uint32_t queueFamilyIndex = device.getQueueFamilyIndex(type);

		VkCommandPoolCreateInfo cmdPoolCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queueFamilyIndex
		};
		vkCreateCommandPool(_device.getDevice(), &cmdPoolCreateInfo, nullptr, &_commandPool);

		VkCommandBufferAllocateInfo cmdAllocCreateInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = _commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};
		vkAllocateCommandBuffers(_device.getDevice(), &cmdAllocCreateInfo, &_commandBuffer);
	}

	VKCommandList::~VKCommandList()
	{
		vkFreeCommandBuffers(_device.getDevice(), _commandPool, 1, &_commandBuffer);
		vkDestroyCommandPool(_device.getDevice(), _commandPool, nullptr);
	}

	void VKCommandList::reset()
	{
		vkResetCommandPool(_device.getDevice(), _commandPool, 0);
		vkResetCommandBuffer(_commandBuffer, 0);
	}

	void VKCommandList::begin()
	{
		VkCommandBufferBeginInfo cmdBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		vkBeginCommandBuffer(_commandBuffer, &cmdBeginInfo);
	}

	void VKCommandList::end()
	{
		vkEndCommandBuffer(_commandBuffer);
	}

	void VKCommandList::beginLabel(const std::string& label, uint32_t color)
	{
		VkDebugUtilsLabelEXT labelInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
			.pLabelName = label.c_str()
		};
		labelInfo.color[0] = ((color >> 24) & 0xFF) / 255.0f;
		labelInfo.color[1] = ((color >> 16) & 0xFF) / 255.0f;
		labelInfo.color[2] = ((color >> 8) & 0xFF) / 255.0f;
		labelInfo.color[3] = ((color) & 0xFF) / 255.0f;
		vkCmdBeginDebugUtilsLabelKD(_commandBuffer, &labelInfo);
	}

	void VKCommandList::endLabel()
	{
		vkCmdEndDebugUtilsLabelKD(_commandBuffer);
	}

	void VKCommandList::resourceBarrier(const TextureBarrierDesc& desc)
	{
		auto vkTexture = std::dynamic_pointer_cast<VKTexture>(desc.texture);
		if (!vkTexture) return;
		if (!vkTexture->getImage()) return;

		VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		VkImageLayout oldLayout = _device.toVkImageLayout(desc.oldState);
		VkImageLayout newLayout = _device.toVkImageLayout(desc.newState);
		if (oldLayout == newLayout) return;

		imageBarrier.oldLayout = oldLayout;
		imageBarrier.newLayout = newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = vkTexture->getImage();

		imageBarrier.subresourceRange.aspectMask = _device.getAspectFlagsFromFormat(vkTexture->getVkFormat());
		imageBarrier.subresourceRange.baseMipLevel = desc.subRange.baseMipLevel;
		imageBarrier.subresourceRange.levelCount = desc.subRange.levelCount;
		imageBarrier.subresourceRange.baseArrayLayer = desc.subRange.baseArrayLayer;
		imageBarrier.subresourceRange.layerCount = desc.subRange.layerCount;

		imageBarrier.srcAccessMask = _device.getAccessFlagsFromImageLayout(oldLayout);
		imageBarrier.dstAccessMask = _device.getAccessFlagsFromImageLayout(newLayout);

		vkCmdPipelineBarrier(_commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

		vkTexture->setState(desc.newState);
	}

	void VKCommandList::setPipeline(const std::shared_ptr<Pipeline>& pipeline)
	{
		auto vkRasterPipeline = std::dynamic_pointer_cast<VKRasterPipeline>(pipeline);
		if (vkRasterPipeline)
		{
			vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkRasterPipeline->getPipeline());

			_stateRasterPipeline = vkRasterPipeline.get();
			_stateComputePipeline = nullptr;
		}
		else
		{
			auto vkComputePipeline = std::dynamic_pointer_cast<VKComputePipeline>(pipeline);
			if (vkComputePipeline)
			{
				vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkComputePipeline->getPipeline());
				
				_stateComputePipeline = vkComputePipeline.get();
				_stateRasterPipeline = nullptr;
			}
			else
				assert(false);
		}
	}

	void VKCommandList::setPushConstant(const void* data)
	{
		if (_stateRasterPipeline)
		{
			vkCmdPushConstants(_commandBuffer, _stateRasterPipeline->getPipelineLayout(),
				VK_SHADER_STAGE_ALL_GRAPHICS, 0, _stateRasterPipeline->getDesc().pushConstantLayout.size, data);
		}
		else if (_stateComputePipeline)
		{
			vkCmdPushConstants(_commandBuffer, _stateComputePipeline->getPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT, 0, _stateComputePipeline->getDesc().pushConstantLayout.size, data);
		}
		else
			assert(false);
	}

	void VKCommandList::setBindSet(uint32_t set, const std::shared_ptr<BindSet>& bindSet)
	{
		auto vkBindSet = std::dynamic_pointer_cast<VKBindSet>(bindSet);
		VkDescriptorSet descriptorSet = vkBindSet->getDescriptorSet();
		if (_stateRasterPipeline)
		{
			vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
				_stateRasterPipeline->getPipelineLayout(), set, 1, &descriptorSet, 0, nullptr);
		}
		else if (_stateComputePipeline)
		{
			vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
				_stateComputePipeline->getPipelineLayout(), set, 1, &descriptorSet, 0, nullptr);
		}
		else
			assert(false);
	}

	void VKCommandList::beginRenderPass(const RenderPassDesc& desc)
	{
		std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos(desc.colorAttachments.size());
		for (size_t i = 0; i < colorAttachmentInfos.size(); i++)
		{
			const auto& attach = desc.colorAttachments[i];
			auto vkTextureView = std::dynamic_pointer_cast<VKTextureView>(attach.textureView);
			VkRenderingAttachmentInfo& attachmentInfo = colorAttachmentInfos[i];
			attachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			attachmentInfo.imageView = vkTextureView->getImageView();
			attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case LoadOp::DontCare:
				attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}
			memcpy(&attachmentInfo.clearValue.color, attach.clearValue.data(), sizeof(float) * 4);
		}

		VkRenderingAttachmentInfo depthAttachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		VkRenderingAttachmentInfo stencilAttachmentInfo = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

		std::shared_ptr<TextureView> textureView;
		if (!desc.colorAttachments.empty())
		{
			textureView = desc.colorAttachments[0].textureView;
		}
		else if (desc.depthAttachment.textureView)
		{
			textureView = desc.depthAttachment.textureView;
		}
		else if (desc.stencilAttachment.textureView)
		{
			textureView = desc.stencilAttachment.textureView;
		}
		auto& vkTexture0 = std::dynamic_pointer_cast<VKTextureView>(textureView)->getTexture();
		VkRenderingInfo renderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
		renderingInfo.renderArea = { { 0, 0 }, { vkTexture0.getWidth(), vkTexture0.getHeight() }};
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
		renderingInfo.pColorAttachments = colorAttachmentInfos.data();

		if (desc.depthAttachment.textureView)
		{
			const auto& attach = desc.depthAttachment;
			auto vkTextureView = std::dynamic_pointer_cast<VKTextureView>(attach.textureView);
			depthAttachmentInfo.imageView = vkTextureView->getImageView();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case LoadOp::DontCare:
				depthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}
			depthAttachmentInfo.clearValue.depthStencil.depth = attach.clearValue;
			renderingInfo.pDepthAttachment = &depthAttachmentInfo;
		}

		if (desc.stencilAttachment.textureView)
		{
			const auto& attach = desc.stencilAttachment;
			auto vkTextureView = std::dynamic_pointer_cast<VKTextureView>(attach.textureView);
			stencilAttachmentInfo.imageView = vkTextureView->getImageView();
			stencilAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			stencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			switch (attach.loadOp)
			{
			case LoadOp::Clear:
				stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case LoadOp::DontCare:
				stencilAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}
			switch (attach.storeOp)
			{
			case StoreOp::DontCare:
				stencilAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}
			stencilAttachmentInfo.clearValue.depthStencil.stencil = attach.clearValue;
			renderingInfo.pStencilAttachment = &stencilAttachmentInfo;
		}
		
		vkCmdBeginRendering(_commandBuffer, &renderingInfo);
	}

	void VKCommandList::endRenderPass()
	{
		vkCmdEndRendering(_commandBuffer);
	}

	void VKCommandList::setViewport(float x, float y, float width, float height)
	{
		VkViewport viewport = {};
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(_commandBuffer, 0, 1, &viewport);
	}

	void VKCommandList::setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		VkRect2D scissor = {};
		scissor.offset = { x, y };
		scissor.extent = { width, height };
		vkCmdSetScissor(_commandBuffer, 0, 1, &scissor);
	}

	void VKCommandList::setStencilReference(uint32_t reference)
	{
		vkCmdSetStencilReference(_commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
	}

	void VKCommandList::setVertexBuffer(uint32_t slot, const std::shared_ptr<Buffer>& buffer, size_t offset)
	{
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);

		VkBuffer vertexBuffers[] = { vkBuffer->getBuffer()};
		VkDeviceSize offsets[] = { offset };
		vkCmdBindVertexBuffers(_commandBuffer, slot, 1, vertexBuffers, offsets);
	}

	void VKCommandList::setIndexBuffer(const std::shared_ptr<Buffer>& buffer, size_t offset, Format indexForamt)
	{
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);

		VkIndexType indexType = VK_INDEX_TYPE_UINT32;
		switch (indexForamt)
		{
		case Format::R16Uint:
			indexType = VK_INDEX_TYPE_UINT16;
			break;
		}
		vkCmdBindIndexBuffer(_commandBuffer, vkBuffer->getBuffer(), offset, indexType);
	}

	void VKCommandList::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VKCommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VKCommandList::drawIndexedIndirect(const std::shared_ptr<Buffer>& buffer, uint32_t drawCount)
	{
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);

		vkCmdDrawIndexedIndirect(_commandBuffer, vkBuffer->getBuffer(), 0, drawCount, vkBuffer->getDesc().stride);
	}

	void VKCommandList::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		vkCmdDispatch(_commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VKCommandList::copyBuffer(const std::shared_ptr<Buffer>& src, const std::shared_ptr<Buffer>& dst, size_t size)
	{
		auto vkBufferSrc = std::dynamic_pointer_cast<VKBuffer>(src);
		auto vkBufferDst = std::dynamic_pointer_cast<VKBuffer>(dst);

		VkBufferCopy copyRegion = 
		{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size
		};
		vkCmdCopyBuffer(_commandBuffer, vkBufferSrc->getBuffer(), vkBufferDst->getBuffer(), 1, &copyRegion);
	}

	void VKCommandList::copyBufferToTexture(const std::shared_ptr<Buffer>& buffer, const std::shared_ptr<Texture>& texture, uint32_t mipLevel)
	{
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);
		auto vkTexture = std::dynamic_pointer_cast<VKTexture>(texture);

		VkBufferImageCopy copyRegion = 
		{
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1 },
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { texture->getWidth(), texture->getHeight(), 1 }
		};
		vkCmdCopyBufferToImage(_commandBuffer, vkBuffer->getBuffer(), vkTexture->getImage(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}
	
	void VKCommandList::copyTextureToBuffer(const std::shared_ptr<Texture>& texture, const std::shared_ptr<Buffer>& buffer, uint32_t mipLevel)
	{
		auto vkBuffer = std::dynamic_pointer_cast<VKBuffer>(buffer);
		auto vkTexture = std::dynamic_pointer_cast<VKTexture>(texture);

		VkBufferImageCopy copyRegion =
		{
			.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1 },
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { texture->getWidth(), texture->getHeight(), 1 }
		};
		vkCmdCopyImageToBuffer(_commandBuffer,
			vkTexture->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			vkBuffer->getBuffer(), 1, &copyRegion);
	}

	void VKCommandList::copyTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst,
		glm::uvec2 size, glm::ivec2 srcOffset, glm::ivec2 dstOffset)
	{
		auto vkTextureSrc = std::dynamic_pointer_cast<VKTexture>(src);
		auto vkTextureDst = std::dynamic_pointer_cast<VKTexture>(dst);

		VkImageCopy copyRegion =
		{
			.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.srcOffset = { srcOffset.x, srcOffset.y, 0 },
			.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.dstOffset = { dstOffset.x, dstOffset.y, 0 },
			.extent = { size.x, size.y, 1 }
		};
		vkCmdCopyImage(_commandBuffer, vkTextureSrc->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			vkTextureDst->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

	void VKCommandList::resolveTexture(const std::shared_ptr<Texture>& src, const std::shared_ptr<Texture>& dst)
	{
		auto vkTextureSrc = std::dynamic_pointer_cast<VKTexture>(src);
		auto vkTextureDst = std::dynamic_pointer_cast<VKTexture>(dst);

		VkImageResolve resolveRegion =
		{
			.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.srcOffset = { 0, 0, 0 },
			.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			.dstOffset = { 0, 0, 0 },
			.extent = { dst->getWidth(), dst->getHeight(), 1 }
		};
		vkCmdResolveImage(_commandBuffer, vkTextureSrc->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			vkTextureDst->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &resolveRegion);
	}
}
