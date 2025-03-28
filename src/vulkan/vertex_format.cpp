#include "vertex_format.h"

llt::VertexFormat llt::g_primitiveVertexFormat;
llt::VertexFormat llt::g_primitiveUvVertexFormat;
llt::VertexFormat llt::g_modelVertexFormat;

using namespace llt;

void llt::initVertexTypes()
{
	g_primitiveVertexFormat.addBinding(sizeof(PrimitiveVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(PrimitiveVertex, position) }
	});

	g_primitiveUvVertexFormat.addBinding(sizeof(PrimitiveUVVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(PrimitiveUVVertex, position) },
		{ VK_FORMAT_R32G32_SFLOAT, offsetof(PrimitiveUVVertex, uv) }
	});

	g_modelVertexFormat.addBinding(sizeof(ModelVertex), VK_VERTEX_INPUT_RATE_VERTEX, {
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, position) },
		{ VK_FORMAT_R32G32_SFLOAT, offsetof(ModelVertex, uv) },
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, colour) },
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, normal) },
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, tangent) },
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(ModelVertex, bitangent) }
	});
}

VertexFormat::VertexFormat()
	: m_attributes()
	, m_bindings()
	, m_size(0)
{
}

VertexFormat::~VertexFormat()
{
}

void VertexFormat::addBinding(uint32_t stride, VkVertexInputRate inputRate, const Vector<AttributeDescription> &attributes)
{
	if (inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
		m_size = stride;

	uint32_t binding = m_bindings.size();

	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = binding;
	bindingDescription.stride = stride;
	bindingDescription.inputRate = inputRate;

	for (auto &attrib : attributes)
	{
		VkVertexInputAttributeDescription attributeDescription = {};
		attributeDescription.binding = binding;
		attributeDescription.location = m_attributes.size();
		attributeDescription.format = attrib.format;
		attributeDescription.offset = attrib.offset;

		m_attributes.pushBack(attributeDescription);
	}

	m_bindings.pushBack(bindingDescription);
}

void VertexFormat::clearAttributes()
{
	m_attributes.clear();
}

void VertexFormat::clearBindings()
{
	m_bindings.clear();
}

const Vector<VkVertexInputAttributeDescription> &VertexFormat::getAttributeDescriptions() const
{
	return m_attributes;
}

const Vector<VkVertexInputBindingDescription> &VertexFormat::getBindingDescriptions() const
{
	return m_bindings;
}

uint64_t VertexFormat::getVertexSize() const
{
	return m_size;
}
