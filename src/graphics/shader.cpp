#include "shader.h"
#include "backend.h"
#include "util.h"

using namespace llt;

const ShaderParameters::PackedData& ShaderParameters::getPackedConstants()
{
	if (m_dirtyConstants) {
		rebuildPackedConstantData();
		m_dirtyConstants = false;
	}

	return m_packedConstants;
}

void ShaderParameters::rebuildPackedConstantData()
{
	m_packedConstants.clear();
	m_packedConstants.resize(m_offsetAccumulator);

	for (auto& [name, param] : m_constants) {
		mem::copy(m_packedConstants.data() + param.offset, param.data, param.size);
	}
}

ShaderProgram::ShaderProgram()
	: m_shaderModule(VK_NULL_HANDLE)
{
}

ShaderProgram::~ShaderProgram()
{
	cleanUp();
}

void ShaderProgram::cleanUp()
{
	if (m_shaderModule == VK_NULL_HANDLE) {
		return;
	}

	vkDestroyShaderModule(g_vulkanBackend->device, m_shaderModule, nullptr);
	m_shaderModule = VK_NULL_HANDLE;
}

void ShaderProgram::loadFromSource(const char* source, uint64_t size)
{
	// create the shader module from source
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (const uint32_t*)source;

	if (VkResult result = vkCreateShaderModule(g_vulkanBackend->device, &moduleCreateInfo, nullptr, &m_shaderModule); result != VK_SUCCESS) {
		LLT_ERROR("[SHADER|DEBUG] Failed to create shader module: %d", result);
	}
}

VkPipelineShaderStageCreateInfo ShaderProgram::getShaderStageCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = type;
	shaderStage.module = m_shaderModule;
	shaderStage.pName = "main";

	return shaderStage;
}

VkShaderModule ShaderProgram::getModule() const
{
	return m_shaderModule;
}
