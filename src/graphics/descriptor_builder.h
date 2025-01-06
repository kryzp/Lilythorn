#ifndef DESCRIPTOR_BUILDER_H_
#define DESCRIPTOR_BUILDER_H_

#include "descriptor_pool_mgr.h"
#include "descriptor_cache.h"

#include "../container/vector.h"

namespace llt
{
	class DescriptorBuilder
	{
	public:
		DescriptorBuilder() = default;
		~DescriptorBuilder() = default;

		void clear();

		uint64_t getHash() const;

		void build(VkDescriptorSet& set, const VkDescriptorSetLayout& layout, uint32_t count, uint64_t hash);
		void buildLayout(VkDescriptorSetLayout& layout, VkDescriptorSetLayoutCreateFlags flags);

		void bindBuffer(uint32_t idx, const VkDescriptorBufferInfo* info, VkDescriptorType type, VkShaderStageFlags stageFlags, int count, VkDescriptorBindingFlags flags);
		void bindImage(uint32_t idx, const VkDescriptorImageInfo* info, VkDescriptorType type, VkShaderStageFlags stageFlags, int count, VkDescriptorBindingFlags flags);

	private:
		Vector<VkWriteDescriptorSet> m_writes;
		Vector<VkDescriptorSetLayoutBinding> m_bindings;
		Vector<VkDescriptorBindingFlags> m_flags;
	};
}

#endif // DESCRIPTOR_BUILDER_H_
