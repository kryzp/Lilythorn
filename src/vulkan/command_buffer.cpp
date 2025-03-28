#include "command_buffer.h"

#include "core/common.h"

#include "core.h"
#include "render_target.h"
#include "render_info.h"
#include "shader.h"

using namespace llt;

CommandBuffer CommandBuffer::fromGraphics()
{
	return CommandBuffer(g_vkCore->m_graphicsQueue.getCurrentFrame().commandBuffer);
}

CommandBuffer CommandBuffer::fromCompute()
{
	return CommandBuffer(g_vkCore->m_computeQueues[0].getCurrentFrame().commandBuffer);
}

CommandBuffer CommandBuffer::fromTransfer()
{
	//	if (m_transferQueues.size() > 0)
	//		return CommandBuffer(g_vkCore->m_transferQueues[0].getCurrentFrame().commandBuffer);
	return fromGraphics();
}

CommandBuffer::CommandBuffer(VkCommandBuffer buffer)
	: m_buffer(buffer)
	, m_viewport()
	, m_scissor()
	, m_currentTarget(nullptr)
	, m_currentRenderInfo()
	, m_isRendering(false)
{
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::beginRecording()
{
	cauto &currentFrame = g_vkCore->m_graphicsQueue.getCurrentFrame();
	vkWaitForFences(g_vkCore->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(m_buffer, &commandBufferBeginInfo),
		"Failed to begin recording command buffer"
	);
}

void CommandBuffer::submit()
{
	VkSemaphoreSubmitInfo emptyComputeSemaphore = {};
	submit(emptyComputeSemaphore);
}

void CommandBuffer::submit(VkSemaphoreSubmitInfo computeSemaphore)
{
	cauto &currentFrame = g_vkCore->m_graphicsQueue.getCurrentFrame();

	LLT_VK_CHECK(
		vkEndCommandBuffer(m_buffer),
		"Failed to record command buffer"
	);

	VkCommandBufferSubmitInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	bufferInfo.deviceMask = 0;
	bufferInfo.commandBuffer = m_buffer;

	VkSemaphoreSubmitInfo renderFinishedSemaphore = {};
	VkSemaphoreSubmitInfo waitSemaphoreSubmitInfos[2] = {};
	
	int signalSemaphoreCount = 0;
	int waitSemaphoreCount = 0;

	if (m_currentTarget && m_currentTarget->getType() == RENDER_TARGET_TYPE_SWAPCHAIN)
	{
		waitSemaphoreCount++;
		waitSemaphoreSubmitInfos[0] = ((Swapchain *)m_currentTarget)->getImageAvailableSemaphoreSubmitInfo();

		signalSemaphoreCount++;
		renderFinishedSemaphore = ((Swapchain *)m_currentTarget)->getRenderFinishedSemaphoreSubmitInfo();
	}

	if (computeSemaphore.semaphore)
	{
		waitSemaphoreCount++;
		waitSemaphoreSubmitInfos[1] = computeSemaphore;
	}

//	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT };

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.flags = 0;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &bufferInfo;

	submitInfo.signalSemaphoreInfoCount = signalSemaphoreCount;
	submitInfo.pSignalSemaphoreInfos = &renderFinishedSemaphore;

	submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
	submitInfo.pWaitSemaphoreInfos = waitSemaphoreSubmitInfos;

	vkResetFences(g_vkCore->m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit2(g_vkCore->m_graphicsQueue.getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit draw command to buffer"
	);

	m_currentTarget = nullptr;
}

void CommandBuffer::beginRendering(GenericRenderTarget *target)
{
	m_currentTarget = target;

	beginRendering(m_currentTarget->getRenderInfo());
}

void CommandBuffer::beginRendering(const RenderInfo &info)
{
	m_currentRenderInfo = info;

	if (m_currentTarget)
		m_currentTarget->beginRendering(*this);

	VkRenderingInfo renderInfo = info.getInfo();

	m_viewport.x = 0.0f;
	m_viewport.y = (float)info.getHeight();
	m_viewport.width = (float)info.getWidth();
	m_viewport.height = -(float)info.getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;

	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { info.getWidth(), info.getHeight() };

	m_isRendering = true;

	vkCmdBeginRendering(m_buffer, &renderInfo);
}

void CommandBuffer::endRendering()
{
	vkCmdEndRendering(m_buffer);

	if (m_currentTarget)
		m_currentTarget->endRendering(*this);

	m_isRendering = false;
}

const RenderInfo &CommandBuffer::getCurrentRenderInfo() const
{
	return m_currentRenderInfo;
}

void CommandBuffer::bindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
{
	vkCmdBindPipeline(
		m_buffer,
		bindPoint,
		pipeline
	);
}

void CommandBuffer::drawIndexed(
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	int32_t vertexOffset,
	uint32_t firstInstance
)
{
	vkCmdSetViewport(m_buffer, 0, 1, &m_viewport);
	vkCmdSetScissor(m_buffer, 0, 1, &m_scissor);

	vkCmdDrawIndexed(
		m_buffer,
		indexCount,
		instanceCount,
		firstIndex,
		vertexOffset,
		firstInstance
	);
}

void CommandBuffer::drawIndexedIndirect(
	VkBuffer buffer,
	VkDeviceSize offset,
	uint32_t drawCount,
	uint32_t stride
)
{
	vkCmdDrawIndexedIndirect(
		m_buffer,
		buffer,
		offset,
		drawCount,
		stride
	);
}

void CommandBuffer::drawIndexedIndirectCount(
	VkBuffer buffer,
	VkDeviceSize offset,
	VkBuffer countBuffer,
	VkDeviceSize countBufferOffset,
	uint32_t maxDrawCount,
	uint32_t stride
)
{
	vkCmdDrawIndexedIndirectCount(
		m_buffer,
		buffer,
		offset,
		countBuffer,
		countBufferOffset,
		maxDrawCount,
		stride
	);
}

void CommandBuffer::bindDescriptorSets(
	uint32_t firstSet,
	VkPipelineLayout layout,
	const Vector<VkDescriptorSet> &descriptorSets,
	const Vector<uint32_t> &dynamicOffsets
)
{
	vkCmdBindDescriptorSets(
		m_buffer,
		m_isRendering ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		layout,
		firstSet,
		descriptorSets.size(),
		descriptorSets.data(),
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);
}

void CommandBuffer::setViewport(const VkViewport &viewport)
{
	m_viewport.x = viewport.x;
	m_viewport.y = viewport.height - viewport.y;
	m_viewport.width = viewport.width;
	m_viewport.height = -viewport.height;
}

void CommandBuffer::setScissor(const VkRect2D &scissor)
{
	m_scissor = scissor;
}

void CommandBuffer::pushConstants(
	VkPipelineLayout layout,
	VkShaderStageFlagBits stageFlags,
	uint32_t size,
	void *data,
	uint32_t offset
)
{
	vkCmdPushConstants(
		m_buffer,
		layout,
		stageFlags,
		offset,
		size,
		data
	);
}

void CommandBuffer::bindVertexBuffers(
	uint32_t firstBinding,
	uint32_t count,
	VkBuffer *buffers,
	VkDeviceSize *offsets
)
{
	vkCmdBindVertexBuffers(
		m_buffer,
		firstBinding,
		count,
		buffers,
		offsets
	);
}

void CommandBuffer::bindIndexBuffer(
	VkBuffer buffer,
	VkDeviceSize offset,
	VkIndexType indexType
)
{
	vkCmdBindIndexBuffer(
		m_buffer,
		buffer,
		offset,
		indexType
	);
}

void CommandBuffer::pipelineBarrier(
	VkDependencyFlags dependencyFlags,
	const Vector<VkMemoryBarrier2> &memoryBarriers,
	const Vector<VkBufferMemoryBarrier2> &bufferMemoryBarriers,
	const Vector<VkImageMemoryBarrier2> &imageMemoryBarriers
)
{
	VkDependencyInfo dependency = {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;

	dependency.dependencyFlags = dependencyFlags;
	
	dependency.memoryBarrierCount = memoryBarriers.size();
	dependency.pMemoryBarriers = memoryBarriers.data();
	
	dependency.bufferMemoryBarrierCount = bufferMemoryBarriers.size();
	dependency.pBufferMemoryBarriers =	bufferMemoryBarriers.data();
	
	dependency.imageMemoryBarrierCount = imageMemoryBarriers.size();
	dependency.pImageMemoryBarriers = imageMemoryBarriers.data();

	vkCmdPipelineBarrier2(
		m_buffer,
		&dependency
	);
}

void CommandBuffer::transitionForMipmapGeneration(Texture &texture)
{
	// todo: bad
	if (texture.getImageLayout() != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		texture.transitionLayout(*this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
}

void CommandBuffer::generateMipmaps(const Texture &texture)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = texture.getImage();
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = texture.getLayerCount();
	barrier.subresourceRange.levelCount = 1;

	for (int i = 1; i < texture.getMipLevels(); i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

		pipelineBarrier(
			0,
			{}, {}, { barrier }
		);

		for (int face = 0; face < texture.getFaceCount(); face++)
		{
			int srcMipWidth  = (int)texture.getWidth()  >> (i - 1);
			int srcMipHeight = (int)texture.getHeight() >> (i - 1);
			int dstMipWidth  = (int)texture.getWidth()  >> (i - 0);
			int dstMipHeight = (int)texture.getHeight() >> (i - 0);

			VkImageBlit blit = {};

			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { srcMipWidth, srcMipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = face;
			blit.srcSubresource.layerCount = 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { dstMipWidth, dstMipHeight, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = face;
			blit.dstSubresource.layerCount = 1;

			blitImage(
				texture.getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				texture.getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				{ blit },
				VK_FILTER_LINEAR
			);
		}

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		pipelineBarrier(
			0,
			{}, {}, { barrier }
		);
	}

	barrier.subresourceRange.baseMipLevel = texture.getMipLevels() - 1;

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	pipelineBarrier(
		0,
		{}, {}, { barrier }
	);
}

void CommandBuffer::blitImage(
	VkImage srcImage, VkImageLayout srcImageLayout,
	VkImage dstImage, VkImageLayout dstImageLayout,
	const Vector<VkImageBlit> &regions,
	VkFilter filter
)
{
	vkCmdBlitImage(
		m_buffer,
		srcImage, srcImageLayout,
		dstImage, dstImageLayout,
		regions.size(),
		regions.data(),
		filter
	);
}

void CommandBuffer::copyBufferToBuffer(
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	const Vector<VkBufferCopy> &regions
)
{
	vkCmdCopyBuffer(
		m_buffer,
		srcBuffer,
		dstBuffer,
		regions.size(),
		regions.data()
	);
}

void CommandBuffer::copyBufferToImage(
	VkBuffer srcBuffer,
	VkImage dstImage,
	VkImageLayout dstImageLayout,
	const Vector<VkBufferImageCopy> &regions
)
{
	vkCmdCopyBufferToImage(
		m_buffer,
		srcBuffer,
		dstImage,
		dstImageLayout,
		regions.size(),
		regions.data()
	);
}

void CommandBuffer::writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query)
{
	vkCmdWriteTimestamp(
		m_buffer,
		pipelineStage,
		pool,
		query
	);
}

void CommandBuffer::resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount)
{
	vkCmdResetQueryPool(
		m_buffer,
		pool,
		firstQuery,
		queryCount
	);
}

void CommandBuffer::beginCompute()
{
	cauto &currentFrame = g_vkCore->m_computeQueues[0].getCurrentFrame(); // current buffer comes from here! (note to myself tomorrow after i get sleep)

	vkWaitForFences(g_vkCore->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetCommandPool(g_vkCore->m_device, currentFrame.commandPool, 0);

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	LLT_VK_CHECK(
		vkBeginCommandBuffer(m_buffer, &commandBufferBeginInfo),
		"Failed to begin recording compute command buffer"
	);
}

void CommandBuffer::endCompute(VkSemaphore signalSemaphore)
{
	cauto &currentFrame = g_vkCore->m_computeQueues[0].getCurrentFrame();

	LLT_VK_CHECK(
		vkEndCommandBuffer(m_buffer),
		"Failed to record compute command buffer"
	);

	VkSubmitInfo submitInfo = {};

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_buffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	vkResetFences(g_vkCore->m_device, 1, &currentFrame.inFlightFence);

	LLT_VK_CHECK(
		vkQueueSubmit(g_vkCore->m_computeQueues[0].getQueue(), 1, &submitInfo, currentFrame.inFlightFence),
		"Failed to submit compute command buffer"
	);
}

void CommandBuffer::dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ)
{
	vkCmdDispatch(m_buffer, gcX, gcY, gcZ);
}

VkCommandBuffer CommandBuffer::getHandle() const
{
	return m_buffer;
}
