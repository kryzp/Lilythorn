#ifndef VK_TEXTURE_H_
#define VK_TEXTURE_H_

#include <vulkan/vulkan.h>

#include "../third_party/vk_mem_alloc.h"

#include "gpu_buffer.h"
#include "texture_sampler.h"
#include "image.h"

#include "../common.h"

namespace llt
{
	class RenderTarget;

	enum TextureProperty
	{
		TEXTURE_PROPERTY_NONE = 0,
		TEXTURE_PROPERTY_MIPMAPPED,
		TEXTURE_PROPERTY_MAX_ENUM
	};

	struct TextureInfo
	{
		VkFormat format;
		VkImageTiling tiling;
		VkImageViewType type;
	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		void cleanUp();

		void fromImage(const Image& image, VkImageViewType type, uint32_t mipLevels, VkSampleCountFlagBits numSamples);

		void setSize(uint32_t width, uint32_t height);
		void setProperties(VkFormat format, VkImageTiling tiling, VkImageViewType type);
		void setMipLevels(uint32_t mipLevels);
		void setSampleCount(VkSampleCountFlagBits numSamples);
		void setTransient(bool isTransient);

		void flagAsDepthTexture();
		bool isDepthTexture() const;

		void createInternalResources();

		void transitionLayout(VkImageLayout newLayout);
		void generateMipmaps() const;

		void setParent(RenderTarget* getParent);
		const RenderTarget* getParent() const;
		bool hasParent() const;

		bool isMipmapped() const;
		void setMipmapped(bool mipmapped);

		bool isUnorderedAccessView() const;
		void setUnorderedAccessView(bool uav);

		uint32_t getLayerCount() const;
		uint32_t getFaceCount() const;

		VkImage getImage() const;
		VkImageView getImageView() const;

		uint32_t width() const;
		uint32_t height() const;

		uint32_t getMipLevels() const;
		VkSampleCountFlagBits getNumSamples() const;
		bool isTransient() const;

		TextureInfo getInfo() const;

	private:
		VkImageView generateView() const;

		RenderTarget* m_parent;

		VkImage m_image;
		VkImageLayout m_imageLayout;
		VkImageView m_view;
		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_numSamples;
		bool m_transient;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VkFormat m_format;
		VkImageTiling m_tiling;
		VkImageViewType m_type;

		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_depth;
		bool m_mipmapped;
		bool m_isDepthTexture;

		bool m_uav;
	};

	/**
	* Wrapper around a texture and sampler pair
	*/
	class SampledTexture
	{
	public:
		SampledTexture();
		SampledTexture(const Texture* texture, TextureSampler* sampler);
		~SampledTexture() = default;

		const VkDescriptorImageInfo& getInfo();

		void bind(uint32_t idx);
		void unbind();
		uint32_t getBoundIdx() const;
		bool isBound() const;

		const Texture* texture;
		TextureSampler* sampler;

	private:
		VkDescriptorImageInfo m_info;

		uint32_t m_boundIdx;
		bool m_isBound;
	};
}

#endif // VK_TEXTURE_H_
