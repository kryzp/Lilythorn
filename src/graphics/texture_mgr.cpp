#include "texture_mgr.h"
#include "texture.h"
#include "texture_sampler.h"
#include "backend.h"
#include "descriptor_builder.h"

llt::TextureMgr* llt::g_textureManager = nullptr;

using namespace llt;

TextureMgr::TextureMgr()
	: m_textureCache()
	, m_samplerCache()
{
}

TextureMgr::~TextureMgr()
{
	for (auto& [name, texture] : m_textureCache) {
		delete texture;
	}

	m_textureCache.clear();

	for (auto& [name, sampler] : m_samplerCache) {
		delete sampler;
	}

	m_samplerCache.clear();
}

TextureSampler* TextureMgr::getSampler(const String& name)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache[name];
	}

	return nullptr;
}

Texture* TextureMgr::getTexture(const String& name)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	return nullptr;
}

Texture* TextureMgr::createFromImage(const String& name, const Image& image)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	// create the texture from the image with a sampling count of 1 and 4 mipmaps
	texture->fromImage(image, VK_IMAGE_VIEW_TYPE_2D, 4, VK_SAMPLE_COUNT_1_BIT);

	// all images have levels of mipmaps
	texture->setMipmapped(true);

	// create the internal resources for the texture
	texture->createInternalResources();

	// transition into the transfer destination layout for the staging buffer
	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// move the image data onto the staging buffer, and then copy the data on the staging buffer onto the texture
	GPUBuffer* stage = g_gpuBufferManager->createStagingBuffer(image.getSize());
	stage->writeDataToMe(image.getData(), image.getSize(), 0);
	stage->writeToTexture(texture, image.getSize());
	delete stage;

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createFromData(const String& name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte* data, uint64_t size)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);
	texture->setMipLevels(4);

	texture->createInternalResources();
    
	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// check if we're actually creating a texture from data or if we're just trying to create an empty texture for now (data = nullptr)
	if (data)
	{
		// this means we need to be mipmapped
		texture->setMipmapped(true);

		// transfer data into the texture via staging buffer
		GPUBuffer* stage = g_gpuBufferManager->createStagingBuffer(size);
		stage->writeDataToMe(data, size, 0);
		stage->writeToTexture(texture, size);
		delete stage;
	}
	else
	{
		// simply just transition into the shader reading layout
		texture->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createAttachment(const String& name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(width, height);
	texture->setProperties(format, tiling, VK_IMAGE_VIEW_TYPE_2D);

	texture->createInternalResources();

	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	texture->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_textureCache.insert(name, texture);
	return texture;
}

Texture* TextureMgr::createCubeMap(const String& name, VkFormat format, const Image& right, const Image& left, const Image& top, const Image& bottom, const Image& front, const Image& back)
{
	if (m_textureCache.contains(name)) {
		return m_textureCache.get(name);
	}

	Texture* texture = new Texture();

	texture->setSize(right.getWidth(), right.getHeight());
	texture->setProperties(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_CUBE);
	texture->setMipLevels(4);

	texture->setMipmapped(true);

	texture->createInternalResources();

	texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// create a staging buffer large enough for all 6 textures
	GPUBuffer* stage = g_gpuBufferManager->createStagingBuffer(right.getSize() * 6);

	const Image* sides[] = { &right, &left, &top, &bottom, &front, &back };

	for (int i = 0; i < 6; i++) {
		// transfer the data to the texture at some offset based on the index of the face
		stage->writeDataToMe(sides[i]->getData(), sides[i]->getSize(), sides[i]->getSize() * i);
		stage->writeToTexture(texture, sides[i]->getSize(), sides[i]->getSize() * i, i);
	}

	delete stage;

	m_textureCache.insert(name, texture);
	return texture;
}

TextureSampler* TextureMgr::createSampler(const String& name, const TextureSampler::Style& style)
{
	if (m_samplerCache.contains(name)) {
		return m_samplerCache.get(name);
	}

	TextureSampler* sampler = new TextureSampler(style);

	m_samplerCache.insert(name, sampler);
	return sampler;
}
