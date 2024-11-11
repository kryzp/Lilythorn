#include "shader_buffer_mgr.h"
#include "../common.h"

llt::ShaderBufferMgr* llt::g_shaderBufferManager = nullptr;

using namespace llt;

ShaderBufferMgr::ShaderBufferMgr()
	: m_buffers()
{
}

ShaderBufferMgr::~ShaderBufferMgr()
{
	for (auto& buf : m_buffers) {
		buf->cleanUp();
	}
}

void ShaderBufferMgr::unbindAll()
{
	for (auto& buf : m_buffers) {
		buf->unbind();
	}
}

void ShaderBufferMgr::resetBufferUsageInFrame()
{
	for (auto& buf : m_buffers) {
		buf->resetBufferUsageInFrame();
	}
}

void ShaderBufferMgr::bindToDescriptorBuilder(DescriptorBuilder* builder, VkShaderStageFlagBits stage)
{
	for (auto& buf : m_buffers)
	{
		if (buf->isBound())
		{
			builder->bindBuffer(
				buf->getBoundIdx(),
				&buf->getDescriptor(),
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				stage
			);
		}
	}
}

Vector<uint32_t> ShaderBufferMgr::getDynamicOffsets() const
{
	Vector<uint32_t> result;

	for (auto& buf : m_buffers)
	{
		if (buf->isBound()) {
			result.pushBack(buf->getDynamicOffset());
		}
	}

	return result;
}

ShaderBuffer* ShaderBufferMgr::createUBO()
{
	ShaderBuffer* ubo = new ShaderBuffer();
	ubo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_UBO);
	m_buffers.pushBack(ubo);
	return m_buffers.back();
}

ShaderBuffer* ShaderBufferMgr::createSSBO()
{
	ShaderBuffer* ssbo = new ShaderBuffer();
	ssbo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_SSBO);
	m_buffers.pushBack(ssbo);
	return m_buffers.back();
}