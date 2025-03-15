#include "texture_sampler.h"
#include "util.h"
#include "core.h"

using namespace llt;

TextureSampler::TextureSampler()
	: style()
	, m_sampler(VK_NULL_HANDLE)
{
}

TextureSampler::TextureSampler(const TextureSampler::Style &style)
	: style(style)
	, m_sampler(VK_NULL_HANDLE)
{
}

TextureSampler::~TextureSampler()
{
	cleanUp();
}

void TextureSampler::cleanUp()
{
	if (m_sampler == VK_NULL_HANDLE) {
		return;
	}

	vkDestroySampler(g_vkCore->m_device, m_sampler, nullptr);
	m_sampler = VK_NULL_HANDLE;
}

VkSampler TextureSampler::bind()
{
	if (m_sampler)
		return m_sampler;

	VkPhysicalDeviceProperties properties = g_vkCore->m_physicalData.properties;

	cleanUp();

	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.minFilter = style.filter;
	createInfo.magFilter = style.filter;
	createInfo.addressModeU = style.wrapX;
	createInfo.addressModeV = style.wrapY;
	createInfo.addressModeW = style.wrapZ;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = style.borderColour;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = VK_LOD_CLAMP_NONE;

	LLT_VK_CHECK(
		vkCreateSampler(g_vkCore->m_device, &createInfo, nullptr, &m_sampler),
		"Failed to create texture sampler"
	);

	return m_sampler;
}

VkSampler TextureSampler::sampler() const
{
	return m_sampler;
}
