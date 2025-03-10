#include "render_target_mgr.h"

#include "vulkan/render_target.h"
#include "vulkan/texture.h"

#include "math/colour.h"

llt::RenderTargetMgr *llt::g_renderTargetManager = nullptr;

using namespace llt;

RenderTargetMgr::RenderTargetMgr()
{
}

RenderTargetMgr::~RenderTargetMgr()
{
	for (auto& [name, target] : m_targets) {
		target->cleanUp();
	}

	m_targets.clear();
}

RenderTarget *RenderTargetMgr::get(const String &name)
{
	return m_targets.getOrDefault(name, nullptr);
}

RenderTarget *RenderTargetMgr::createTarget(const String &name, uint32_t width, uint32_t height, const Vector<VkFormat>& attachments, VkSampleCountFlagBits samples)
{
	if (m_targets.contains(name)) {
		return m_targets.get(name);
	}

	RenderTarget *result = new RenderTarget(width, height);
	result->setMSAA(samples);

	for (int i = 0; i < attachments.size(); i++)
	{
		Texture *texture = new Texture();

		texture->setSize(width, height);
		texture->setProperties(attachments[i], VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
		texture->setSampleCount(samples);

		texture->createInternalResources();

//		texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		texture->transitionLayoutSingle(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		texture->setParent(result);

		result->addAttachment(texture);
	}

	result->setClearColours(Colour::black());

	m_targets.insert(name, result);
	return result;
}
