#ifndef VK_BUFFER_MGR_H_
#define VK_BUFFER_MGR_H_

#include "../container/vector.h"
#include "gpu_buffer.h"

namespace llt
{
	class GPUBufferMgr
	{
	public:
		GPUBufferMgr();
		~GPUBufferMgr();

		GPUBuffer* createBuffer(VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, uint64_t size);

		GPUBuffer* createStagingBuffer(uint64_t size);
		GPUBuffer* createVertexBuffer(uint64_t vertexCount, uint32_t vertexSize);
		GPUBuffer* createIndexBuffer(uint64_t indexCount);
		GPUBuffer* createUniformBuffer(uint64_t size);
		GPUBuffer* createShaderStorageBuffer(uint64_t size);

	private:
		Vector<GPUBuffer*> m_vertexBuffers;
		Vector<GPUBuffer*> m_indexBuffers;
	};

	extern GPUBufferMgr* g_gpuBufferManager;
}

#endif // VK_BUFFER_MGR_H_
