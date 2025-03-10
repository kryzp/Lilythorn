#ifndef COMMAND_BUFFER_H_
#define COMMAND_BUFFER_H_

#define VK_NO_PROTOTYPES
#include "third_party/volk.h"

#include "render_info.h"

namespace llt
{
	class GenericRenderTarget;
	class Pipeline;
	class ShaderEffect;

	class CommandBuffer
	{
	public:
		static CommandBuffer fromGraphics();
		static CommandBuffer fromCompute();
		static CommandBuffer fromTransfer();

		CommandBuffer(VkCommandBuffer buffer);
		~CommandBuffer();

		void submit(VkSemaphore computeSemaphore = VK_NULL_HANDLE);

		void beginRendering(GenericRenderTarget *target);
		void beginRendering(const RenderInfo &info);
		
		void endRendering();

		const RenderInfo &getCurrentRenderInfo() const;

		void bindPipeline(Pipeline &pipeline);

		void drawIndexed(
			uint32_t indexCount,
			uint32_t instanceCount = 1,
			uint32_t firstIndex = 0,
			int32_t vertexOffset = 0,
			uint32_t firstInstance = 0
		);

		void drawIndexedIndirect(
			VkBuffer buffer,
			VkDeviceSize offset,
			uint32_t drawCount,
			uint32_t stride
		);
		
		void drawIndexedIndirectCount(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkBuffer countBuffer,
			VkDeviceSize countBufferOffset,
			uint32_t maxDrawCount,
			uint32_t stride
		);

		void bindDescriptorSets(
			uint32_t firstSet,
			uint32_t setCount,
			const VkDescriptorSet *descriptorSets,
			uint32_t dynamicOffsetCount,
			const uint32_t *dynamicOffsets
		);

		void setViewport(const VkViewport &viewport);
		void setScissor(const VkRect2D &scissor);

		void pushConstants(
			VkShaderStageFlagBits stageFlags,
			uint32_t size,
			void *data,
			uint32_t offset = 0
		);

		void bindVertexBuffers(
			uint32_t firstBinding,
			uint32_t count,
			VkBuffer *buffers,
			VkDeviceSize *offsets
		);

		void bindIndexBuffer(
			VkBuffer buffer,
			VkDeviceSize offset,
			VkIndexType indexType
		);

		void pipelineBarrier(
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkDependencyFlags dependencyFlags,
			uint32_t memoryBarrierCount,		const VkMemoryBarrier *memoryBarriers,
			uint32_t bufferMemoryBarrierCount,	const VkBufferMemoryBarrier *bufferMemoryBarriers,
			uint32_t imageMemoryBarrierCount,	const VkImageMemoryBarrier *imageMemoryBarriers
		);

		void blitImage(
			VkImage srcImage, VkImageLayout srcImageLayout,
			VkImage dstImage, VkImageLayout dstImageLayout,
			uint32_t regionCount,
			const VkImageBlit *regions,
			VkFilter filter
		);

		void copyBufferToBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			uint32_t regionCount,
			const VkBufferCopy *regions
		);

		void copyBufferToImage(
			VkBuffer srcBuffer,
			VkImage dstImage,
			VkImageLayout dstImageLayout,
			uint32_t regionCount,
			const VkBufferImageCopy *regions
		);

		void writeTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool pool, uint32_t query);

		void resetQueryPool(VkQueryPool pool, uint32_t firstQuery, uint32_t queryCount);

		void beginCompute();
		void endCompute(VkSemaphore signalSemaphore);

		void dispatch(uint32_t gcX, uint32_t gcY, uint32_t gcZ);

		VkCommandBuffer getBuffer() const;

	private:
		void _beginRecording();

		VkCommandBuffer m_buffer;
		VkPipelineLayout m_currentPipelineLayout;

		VkViewport m_viewport;
		VkRect2D m_scissor;

		GenericRenderTarget *m_currentTarget;
		RenderInfo m_currentRenderInfo;
	};
}

#endif // COMMAND_BUFFER_H_
