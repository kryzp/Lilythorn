#include "texture.h"

#include "math/calc.h"

#include "rendering/bindless_resource_mgr.h"

#include "core.h"
#include "util.h"
#include "image.h"

using namespace llt;

Texture::Texture()
	: m_parent(nullptr)
	, m_image(VK_NULL_HANDLE)
	, m_imageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
	, m_mipmapCount(1)
	, m_viewCache()
	, m_numSamples(VK_SAMPLE_COUNT_1_BIT)
	, m_transient(false)
	, m_stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
	, m_allocation()
	, m_allocationInfo()
	, m_format()
	, m_tiling()
	, m_type()
	, m_width(0)
	, m_height(0)
	, m_depth(1)
	, m_isDepthTexture(false)
	, m_isUAV(false)
{
}

Texture::~Texture()
{
	cleanUp();
}

void Texture::cleanUp()
{
	if (m_image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(g_vkCore->m_vmaAllocator, m_image, m_allocation);
		m_image = VK_NULL_HANDLE;
	}

	for (auto &[id, view] : m_viewCache)
	{
		view.cleanUp();
	}

	m_viewCache.clear();

	m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_format = VK_FORMAT_MAX_ENUM;
	m_tiling = VK_IMAGE_TILING_MAX_ENUM;
	m_type = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
	m_width = 0;
	m_height = 0;
	m_mipmapCount = 0;
	m_numSamples = VK_SAMPLE_COUNT_1_BIT;
	m_transient = false;
}

void Texture::createInternalResources()
{
	VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

	switch (m_type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			imageType = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			imageType = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			imageType = VK_IMAGE_TYPE_3D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			imageType = VK_IMAGE_TYPE_2D;
			break;

		default:
			LLT_ERROR("Failed to find VkImageType given VkImageViewType: %d", m_type);
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = imageType;
	createInfo.extent.width = m_width;
	createInfo.extent.height = m_height;
	createInfo.extent.depth = m_depth;
	createInfo.mipLevels = m_mipmapCount;
	createInfo.arrayLayers = getFaceCount();
	createInfo.format = m_format;
	createInfo.tiling = m_tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = m_transient ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = m_numSamples;
	createInfo.flags = 0;

	if (isUnorderedAccessView()) {
		createInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	if (vkutil::hasStencilComponent(m_format)) {
		createInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	} else {
		createInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (m_type == VK_IMAGE_VIEW_TYPE_CUBE) {
		createInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	LLT_VK_CHECK(
		vmaCreateImage(g_vkCore->m_vmaAllocator, &createInfo, &vmaAllocInfo, &m_image, &m_allocation, &m_allocationInfo),
		"Failed to create image"
	);
}

TextureView Texture::getStandardView()
{
	return getView(getLayerCount(), 0, 0);
}

TextureView Texture::getView(int layerCount, int layer, int baseMipLevel)
{
	uint64_t hash = 0;

	hash::combine(&hash, &layerCount);
	hash::combine(&hash, &layer);
	hash::combine(&hash, &baseMipLevel);

	if (m_viewCache.contains(hash))
	{
		return m_viewCache[hash];
	}
	
	VkImageViewType viewType = m_type;

	if (m_type == VK_IMAGE_VIEW_TYPE_CUBE && layerCount == 1) {
		viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	}

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = m_image;
	viewCreateInfo.viewType = viewType;
	viewCreateInfo.format = m_format;

	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = baseMipLevel;
	viewCreateInfo.subresourceRange.levelCount = m_mipmapCount - baseMipLevel;
	viewCreateInfo.subresourceRange.baseArrayLayer = layer;
	viewCreateInfo.subresourceRange.layerCount = layerCount;

	if (vkutil::hasStencilComponent(m_format))
	{
		// depth AND stencil is not allowed for sampling!
		// so, use depth instead.
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageView view = {};

	LLT_VK_CHECK(
		vkCreateImageView(g_vkCore->m_device, &viewCreateInfo, nullptr, &view),
		"Failed to create texture image view."
	);

	TextureView textureView(view, m_format);

	if (viewType == VK_IMAGE_VIEW_TYPE_CUBE)
	{
		textureView.m_bindlessHandle = g_bindlessResources->registerCubemap(textureView);
	}
	else
	{
		textureView.m_bindlessHandle = g_bindlessResources->registerTexture2D(textureView);
	}

	m_viewCache.insert(
		hash,
		textureView
	);

	return textureView;
}

void Texture::transitionLayoutSingle(VkImageLayout newLayout)
{
	CommandBuffer cmd = vkutil::beginSingleTimeCommands(g_vkCore->m_graphicsQueue.getCurrentFrame().commandPool);
	transitionLayout(cmd, newLayout);
	vkutil::endSingleTimeGraphicsCommands(cmd);
}

void Texture::transitionLayout(CommandBuffer &cmd, VkImageLayout newLayout)
{
	//if (m_imageLayout == newLayout)
	//	return;

	VkImageMemoryBarrier2 barrier = getBarrier(newLayout);

	barrier.srcAccessMask = vkutil::getTransferAccessFlags(m_imageLayout);
	barrier.dstAccessMask = vkutil::getTransferAccessFlags(newLayout);

	barrier.srcStageMask = m_stage;
	barrier.dstStageMask = vkutil::getTransferPipelineStageFlags(newLayout);

	cmd.pipelineBarrier(
		0,
		{}, {}, { barrier }
	);

	m_stage = barrier.dstStageMask;
	m_imageLayout = newLayout;
}

// also transitions the texture into SHADER_READ_ONLY layout
void Texture::generateMipmaps(CommandBuffer &cmd)
{
	VkFormatProperties formatProperties = {};
	vkGetPhysicalDeviceFormatProperties(g_vkCore->m_physicalData.device, m_format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
	{
		LLT_ERROR("Texture image format doesn't support linear blitting.");
		return;
	}

	cmd.transitionForMipmapGeneration(*this);
	cmd.generateMipmaps(*this);

	m_imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // todo: this should be in the command buffer. also in general cmd.generateMipmaps is a bit of a mess
}

VkImageMemoryBarrier2 Texture::getBarrier() const
{
	return getBarrier(m_imageLayout);
}

VkImageMemoryBarrier2 Texture::getBarrier(VkImageLayout newLayout) const
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	
	barrier.oldLayout = m_imageLayout;
	barrier.newLayout = newLayout;
	
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	
	barrier.image = m_image;
	
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_mipmapCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = getLayerCount();
	
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	barrier.srcStageMask = 0;
	barrier.dstStageMask = 0;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return barrier;
}

void Texture::fromImage(const Image &image, VkImageViewType type, uint32_t mipLevels, VkSampleCountFlagBits numSamples)
{
	setSize(image.getWidth(), image.getHeight());

	VkFormat format = VK_FORMAT_MAX_ENUM;

	switch (image.getFormat())
	{
		case Image::Format::FORMAT_RGBA8:
			format = VK_FORMAT_R8G8B8A8_UNORM;
			break;

		case Image::Format::FORMAT_RGBAF:
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			break;

		default:
			LLT_ERROR("Unknown image format encountered when creating texture from image: %d", image.getFormat());
	}

	setProperties(format, VK_IMAGE_TILING_OPTIMAL, type);
	setMipLevels(mipLevels);
	setSampleCount(numSamples);
	setTransient(false);
}

void Texture::setSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void Texture::setProperties(VkFormat format, VkImageTiling tiling, VkImageViewType type)
{
	m_format = format;
	m_tiling = tiling;
	m_type = type;
}

void Texture::setMipLevels(uint32_t mipLevels)
{
	uint32_t maxMipLevels = 1 + CalcU::floor(CalcU::log2(CalcU::max(m_width, CalcU::max(m_height, m_depth))));

	m_mipmapCount = CalcU::min(maxMipLevels, mipLevels);
}

void Texture::setSampleCount(VkSampleCountFlagBits numSamples)
{
	m_numSamples = numSamples;
}

void Texture::setTransient(bool isTransient)
{
	m_transient = isTransient;
}

void Texture::flagAsDepthTexture()
{
	m_isDepthTexture = true;
}

bool Texture::isDepthTexture() const
{
	return m_isDepthTexture;
}

void Texture::setParent(RenderTarget *getParent)
{
	m_parent = getParent;
}

const RenderTarget *Texture::getParent() const
{
	return m_parent;
}

bool Texture::hasParent() const
{
	return m_parent != nullptr;
}

bool Texture::isUnorderedAccessView() const
{
	return m_isUAV;
}

void Texture::setUnorderedAccessView(bool uav)
{
	m_isUAV = uav;
}

uint32_t Texture::getLayerCount() const
{
	return (m_type == VK_IMAGE_VIEW_TYPE_1D_ARRAY || m_type == VK_IMAGE_VIEW_TYPE_2D_ARRAY) ? m_depth : getFaceCount();
}

uint32_t Texture::getFaceCount() const
{
	return (m_type == VK_IMAGE_VIEW_TYPE_CUBE) ? 6 : 1;
}

VkImage Texture::getImage() const
{
	return m_image;
}

uint32_t Texture::getWidth() const
{
	return m_width;
}

uint32_t Texture::getHeight() const
{
	return m_height;
}

uint32_t Texture::getMipLevels() const
{
	return m_mipmapCount;
}

VkSampleCountFlagBits Texture::getNumSamples() const
{
	return m_numSamples;
}

bool Texture::isTransient() const
{
	return m_transient;
}

VkFormat Texture::getFormat() const
{
	return m_format;
}

VkImageTiling Texture::getTiling() const
{
	return m_tiling;
}

VkImageViewType Texture::getType() const
{
	return m_type;
}

VkPipelineStageFlags Texture::getStage() const
{
	return m_stage;
}

VkImageLayout Texture::getImageLayout() const
{
	return m_imageLayout;
}
