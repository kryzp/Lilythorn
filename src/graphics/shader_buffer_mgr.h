#ifndef SHADER_BUFFER_MGR_
#define SHADER_BUFFER_MGR_

#include "../container/vector.h"

#include "descriptor_builder.h"
#include "shader_buffer.h"

namespace llt
{
	class ShaderBufferMgr
	{
		// start off with 16kB of shader memory per ubo / ssbo for each frame.
		// this will increase automatically if it isn't enough, however it is a good baseline.
		static constexpr uint64_t INIT_BUFFER_SIZE = KILOBYTES(16) * mgc::FRAMES_IN_FLIGHT;

	public:
		ShaderBufferMgr();
		~ShaderBufferMgr();

		void unbindAll();
		void resetBufferUsageInFrame();

		void bindToDescriptorBuilder(DescriptorBuilder* builder, VkShaderStageFlagBits stage);

		Vector<uint32_t> getDynamicOffsets();

		ShaderBuffer* createUBO();
		ShaderBuffer* createSSBO();

	private:
		void sortBoundOffsets(Vector<Pair<uint32_t, uint32_t>>& offsets, int lo, int hi);

		Vector<ShaderBuffer*> m_buffers;
	};

	extern ShaderBufferMgr* g_shaderBufferManager;
}

#endif // SHADER_BUFFER_MGR_
