#ifndef VK_GENERIC_RENDER_TARGET_H_
#define VK_GENERIC_RENDER_TARGET_H_

#include "third_party/volk.h"

#include "container/array.h"

#include "core/common.h"

#include "render_info.h"

namespace llt
{
	class VulkanCore;
	class RenderInfo;
	class Colour;
	class CommandBuffer;
	class Texture;

	enum RenderTargetType
	{
		RENDER_TARGET_TYPE_TEXTURE,
		RENDER_TARGET_TYPE_SWAPCHAIN,
		RENDER_TARGET_TYPE_MAX_ENUM
	};

	class GenericRenderTarget
	{
	public:
		GenericRenderTarget();
		GenericRenderTarget(uint32_t width, uint32_t height);
		virtual ~GenericRenderTarget();

		virtual void cleanUp() = 0;

		const RenderInfo &getRenderInfo() const;

		virtual void beginRendering(CommandBuffer &cmd) = 0;
		virtual void endRendering(CommandBuffer &cmd) = 0;

		virtual Texture *getAttachment(int idx) = 0;
		virtual Texture *getDepthAttachment() = 0;

		virtual void setClearColour(int idx, const Colour &colour) = 0;
		virtual void setDepthStencilClear(float depth, uint32_t stencil) = 0;

		uint64_t getAttachmentCount() const;

		void setClearColours(const Colour &colour);

		void toggleClear(bool clear);
		void toggleColourClear(int idx, bool clear);
		void toggleDepthStencilClear(bool clear);

		uint32_t getWidth() const;
		uint32_t getHeight() const;
		RenderTargetType getType() const;

	protected:
		uint32_t m_width;
		uint32_t m_height;
		RenderTargetType m_type;

		RenderInfo m_renderInfo;
	};
}

#endif // VK_GENERIC_RENDER_TARGET_H_
