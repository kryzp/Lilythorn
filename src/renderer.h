#ifndef RENDERER_H_
#define RENDERER_H_

#include "container/vector.h"

#include "graphics/vertex_descriptor.h"
#include "graphics/sub_mesh.h"
#include "graphics/shader.h"

#include "gpu_particles.h"

namespace llt
{
	class Backbuffer;
	class ShaderProgram;
	class ShaderBuffer;
	class Camera;

	struct MyVertex
	{
		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 col;
		glm::vec3 norm;
	};

	class Renderer
	{
		static constexpr int INSTANCED_LENGTH = 16;
		static constexpr int INSTANCED_CUBE_COUNT = INSTANCED_LENGTH*INSTANCED_LENGTH*INSTANCED_LENGTH;

	public:
		Renderer();
		~Renderer();

		void init(Backbuffer* backbuffer);
		void render(const Camera& camera, float deltaTime, float elapsedTime);

	private:
		void loadTextures();
		void loadShaders();
		void setupShaderParameters();
		void createQuadMesh();
		void createBlockMesh();
		void createSkybox();
		void setupVertexFormats();
		void createInstanceData();

		Backbuffer* m_backbuffer;

		ShaderProgram* m_vertexShaderInstanced;
		ShaderProgram* m_vertexShader;
		ShaderProgram* m_fragmentShader;
		ShaderProgram* m_fragmentShaderSkybox;

		SubMesh m_quadMesh;
		SubMesh m_blockMesh;
		SubMesh m_skyboxMesh;

		Texture* m_skyboxTexture;
		Texture* m_stoneTexture;
		
		RenderTarget* m_target;
		
		TextureSampler* m_linearSampler;
		TextureSampler* m_nearestSampler;

		VertexDescriptor m_vertexFormat;
		VertexDescriptor m_instancedVertexFormat;

		ShaderBuffer* m_instanceBuffer;

		ShaderParameters m_shaderParams;
		ShaderBuffer* m_shaderParamsBuffer;

		ShaderParameters m_pushConstants;

		GPUParticles m_gpuParticles;
	};
}

#endif // RENDERER_H_
