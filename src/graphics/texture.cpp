#include "texture.h"
#include "backend.h"
#include "util.h"

using namespace llt;

TextureInfo Texture::getInfo() const { return {
	.format = m_format,
	.tiling = m_tiling,
	.type = m_type
}; }

Texture::Texture()
	: m_parent(nullptr)
	, m_depth(1)
	, m_mipmapped(false)
	, m_image(VK_NULL_HANDLE)
	, m_imageMemory(VK_NULL_HANDLE)
	, m_imageLayout()
	, m_view(VK_NULL_HANDLE)
	, m_format()
	, m_tiling()
	, m_type()
	, m_width(0)
	, m_height(0)
	, m_mipmapCount(1)
	, m_numSamples(VK_SAMPLE_COUNT_1_BIT)
	, m_transient(false)
	, m_uav(false)
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
		vkDestroyImage(g_vulkanBackend->device, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}

	if (m_imageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(g_vulkanBackend->device, m_imageMemory, nullptr);
		m_imageMemory = VK_NULL_HANDLE;
	}

	if (m_view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(g_vulkanBackend->device, m_view, nullptr);
		m_view = VK_NULL_HANDLE;
	}

	m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_format = (VkFormat)0;
	m_tiling = (VkImageTiling)0;
	m_type = (VkImageViewType)0;
	m_width = 0;
	m_height = 0;
	m_mipmapCount = 0;
	m_numSamples = VK_SAMPLE_COUNT_1_BIT;
	m_transient = false;
}

void Texture::fromImage(const Image& image, VkImageViewType type, uint32_t mipLevels, VkSampleCountFlagBits numSamples)
{
	initSize(image.getWidth(), image.getHeight());
	initMetadata(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, type);
	initMipLevels(mipLevels);
	initSampleCount(numSamples);
	initTransient(false);
}

void Texture::initSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

void Texture::initMetadata(VkFormat format, VkImageTiling tiling, VkImageViewType type)
{
	m_format = format;
	m_tiling = tiling;
	m_type = type;
}

void Texture::initMipLevels(uint32_t getMipLevels)
{
	m_mipmapCount = getMipLevels;
}

void Texture::initSampleCount(VkSampleCountFlagBits getNumSamples)
{
	m_numSamples = getNumSamples;
}

void Texture::initTransient(bool isTransient)
{
	m_transient = isTransient;
}

void Texture::createInternalResources()
{
	VkImageType vkimagetype = (VkImageType)0;

	switch (m_type)
	{
		case VK_IMAGE_VIEW_TYPE_1D:
			vkimagetype = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
			vkimagetype = VK_IMAGE_TYPE_1D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D:
			vkimagetype = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			vkimagetype = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_3D:
			vkimagetype = VK_IMAGE_TYPE_3D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE:
			vkimagetype = VK_IMAGE_TYPE_2D;
			break;

		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			vkimagetype = VK_IMAGE_TYPE_2D;
			break;

		default:
			LLT_ERROR("[VULKAN:TEXTURE] Failed to find VkImageType given VkImageViewType: %d", m_type);
			break;
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = vkimagetype;
	createInfo.extent.width = m_width;
	createInfo.extent.height = m_height;
	createInfo.extent.depth = m_depth;
	createInfo.mipLevels = m_mipmapCount;
	createInfo.arrayLayers = getFaceCount();
	createInfo.format = m_format;
	createInfo.tiling = m_tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = m_transient ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

	if (VkResult result = vkCreateImage(g_vulkanBackend->device, &createInfo, nullptr, &m_image); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:TEXTURE|DEBUG] Failed to create command pool: %d", result);
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(g_vulkanBackend->device, m_image, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = vkutil::findMemoryType(g_vulkanBackend->physicalData.device, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (VkResult result = vkAllocateMemory(g_vulkanBackend->device, &allocInfo, nullptr, &m_imageMemory); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:TEXTURE|DEBUG] Failed to allocate memory for image: %d", result);
	}

	vkBindImageMemory(g_vulkanBackend->device, m_image, m_imageMemory, 0);

	m_view = generate_view();
}

VkImageView Texture::generate_view() const
{
	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = m_image;
	view_create_info.viewType = m_type;
	view_create_info.format = m_format;

	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = m_mipmapCount;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = getLayerCount();

	if (vkutil::hasStencilComponent(m_format)) {
		// depth AND stencil is not allowed for sampling!
		// so, use depth instead.
		view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	VkImageViewUsageCreateInfo flag_restriction;
	flag_restriction.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageView ret = {};

	if (VkResult result = vkCreateImageView(g_vulkanBackend->device, &view_create_info, nullptr, &ret); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:TEXTURE|DEBUG] Failed to create texture image view: %d", result);
	}

	return ret;
}

// also transitions the texture into SHADER_READ_ONLY layout
void Texture::generateMipmaps() const
{
	LLT_ASSERT(m_imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, "[VULKAN:TEXTURE|DEBUG] Texture must be in TRANSFER_DST_OPTIMAL layout to generate mipmaps.");

	VkFormatProperties formatProperties = {};
	vkGetPhysicalDeviceFormatProperties(g_vulkanBackend->physicalData.device, m_format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		LLT_ERROR("[VULKAN:TEXTURE|DEBUG] Texture image format doesn't support linear blitting.");
		return;
	}

	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, g_vulkanBackend->device);
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = getLayerCount();
		barrier.subresourceRange.levelCount = 1;

		for (int i = 1; i < m_mipmapCount; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			// set up a pipeline barrier for the transferring stage
			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			// go through all faces and blit them
			for (int face = 0; face < getFaceCount(); face++)
			{
				int src_mip_width  = (int)m_width  >> (i - 1);
				int src_mip_height = (int)m_height >> (i - 1);
				int dst_mip_width  = (int)m_width  >> (i - 0);
				int dst_mip_height = (int)m_height >> (i - 0);

				VkImageBlit blit = {};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { src_mip_width, src_mip_height, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = face;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { dst_mip_width, dst_mip_height, 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = face;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(
					cmdBuffer,
					m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR
				);
			}

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		barrier.subresourceRange.baseMipLevel = m_mipmapCount - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}
	vkutil::endSingleTimeGraphicsCommands(cmdBuffer);
}

void Texture::transitionLayout(VkImageLayout newLayout)
{
	VkCommandBuffer cmdBuffer = vkutil::beginSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, g_vulkanBackend->device);
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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

		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_NONE;

		if (m_imageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (m_imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (m_imageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			LLT_ERROR("[VULKAN:TEXTURE|DEBUG] Unsupported layout transition for image.");
		}

		vkCmdPipelineBarrier(
			cmdBuffer,
			srcStage, dstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		m_imageLayout = newLayout;
	}
	vkutil::endSingleTimeCommands(g_vulkanBackend->graphicsQueue.getCurrentFrame().commandPool, cmdBuffer, g_vulkanBackend->device, g_vulkanBackend->graphicsQueue.getQueue());
}

void Texture::setParent(RenderTarget* getParent)
{
	m_parent = getParent;
}

const RenderTarget* Texture::getParent() const
{
	return m_parent;
}

bool Texture::hasParent() const
{
	return m_parent != nullptr;
}

bool Texture::isMipmapped() const
{
	return m_mipmapped;
}

void Texture::setMipmapped(bool mipmapped)
{
	m_mipmapped = mipmapped;
}

bool Texture::isUnorderedAccessView() const
{
	return m_uav;
}

void Texture::setUnorderedAccessView(bool uav)
{
	m_uav = uav;
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

VkImageView Texture::getImageView() const
{
	return m_view;
}

uint32_t Texture::width() const
{
	return m_width;
}

uint32_t Texture::height() const
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
