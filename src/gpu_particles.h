#ifndef GPU_PARTICLES_H_
#define GPU_PARTICLES_H_

#include "graphics/vertex_descriptor.h"
#include "graphics/sub_mesh.h"
#include "graphics/shader.h"

#include "camera.h"

namespace llt
{
	class ShaderBuffer;

	class GPUParticles
	{
		constexpr static int PARTICLE_COUNT = 64;

	public:
		GPUParticles();
		~GPUParticles();

		void init();

		void dispatchCompute(const Camera& camera);
		void render();

	private:
		ShaderBuffer* m_particleBuffer;

		ShaderProgram* m_computeProgram;
		ShaderParameters m_computeParams;
		ShaderBuffer* m_computeParamsBuffer;

		SubMesh m_particleMesh;
		VertexDescriptor m_particleVertexFormat;
	};
}

#endif // GPU_PARTICLES_H_