#include "shader_buffer_mgr.h"
#include "backend.h"
#include "buffer_mgr.h"

using namespace llt;

ShaderBufferManager::ShaderBufferManager()
	: m_buffer(nullptr)
	, m_dynamicOffset()
	, m_info()
	, m_usageInFrame()
	, m_offset(0)
	, m_maxSize(0)
	, m_type(SHADER_BUFFER_NONE)
{
}

void ShaderBufferManager::init(uint64_t initialSize, ShaderBufferType type)
{
	m_type = type;

	reallocateBuffer(initialSize);
	m_usageInFrame.clear();
}

void ShaderBufferManager::cleanUp()
{
	delete m_buffer;
	m_buffer = nullptr;
}

void ShaderBufferManager::pushData(const void* data, uint64_t size, int currentFrame, bool* modified)
{
	// calculate the total used memory so far
	uint64_t totalUsedMemory = 0;
	for (int i = 0; i < m_usageInFrame.size(); i++) {
		totalUsedMemory += m_usageInFrame[i];
	}

	// check if we need to increase in size
	if (totalUsedMemory + size > m_maxSize) {
		reallocateBuffer(m_maxSize * 2);
	}

	// wrap back around to zero if we can't fit all of our data at the current point
	if (m_offset + size >= m_maxSize) {
		m_offset = 0;
	}

	// calculate the aligned dynamic offset
	uint32_t dynamicOffset = vkutil::calcShaderBufferAlignedSize(
		m_offset,
		g_vulkanBackend->physicalData.properties
	);

	// set the dynamic offset
	m_dynamicOffset = dynamicOffset;

	m_info.offset = 0;

	if (m_info.range != size && modified) {
		(*modified) = true;
	}

	m_info.range = size;

	if (m_type == SHADER_BUFFER_UBO)
	{
		// actually write the data into the ubo
		m_buffer->writeDataToMe(data, size, dynamicOffset);
	}
	else
	{
		// actually write the data into the ssbo
		Buffer* stage = g_bufferManager->createStagingBuffer(size);
		stage->writeDataToMe(data, size, 0);
		stage->writeToBuffer(m_buffer, size, 0, dynamicOffset);
		delete stage;
	}

	// move forward and increment the ubo usage in the current frame
	m_offset += size;
	m_usageInFrame[currentFrame] += size;

	// again, if we have moved past the maximum size, wrap back around to zero.
	if (m_offset >= m_maxSize) {
		m_offset = 0;
	}
}

void ShaderBufferManager::reallocateBuffer(uint64_t size)
{
	delete m_buffer;

	VkDeviceSize bufferSize = vkutil::calcShaderBufferAlignedSize(size, g_vulkanBackend->physicalData.properties);

	m_maxSize = size;
	m_offset = 0;

	if (m_type == SHADER_BUFFER_UBO)
	{
		m_buffer = g_bufferManager->createUBO(bufferSize);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		m_buffer = g_bufferManager->createSSBO(bufferSize);
	}
	else
	{
		LLT_ERROR("[VULKAN:SHADERBUFFERMGR|DEBUG] Unsupported ShaderBufferType: %d.", m_type);
	}

	m_info.buffer = m_buffer->getBuffer();
	m_info.offset = 0;
	m_info.range = 0;

	// all of our descriptor sets are invalid now so we have to clear our caches
	g_vulkanBackend->clearDescriptorCacheAndPool();

	if (m_type == SHADER_BUFFER_UBO)
	{
		LLT_LOG("[VULKAN:SHADERBUFFERMGR] (Re)Allocated ubo with size %llu.", size);
	}
	else if (m_type == SHADER_BUFFER_SSBO)
	{
		LLT_LOG("[VULKAN:SHADERBUFFERMGR] (Re)Allocated ssbo with size %llu.", size);
	}
}

void ShaderBufferManager::resetBufferUsageInFrame(int currentFrame)
{
	m_usageInFrame[currentFrame] = 0;
}

const VkDescriptorBufferInfo& ShaderBufferManager::getDescriptor() const
{
	return m_info;
}

uint32_t ShaderBufferManager::getDynamicOffset() const
{
	return m_dynamicOffset;
}
