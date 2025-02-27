#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "../common.h"

#include "../container/array.h"
#include "../container/string.h"

#include "texture.h"
#include "shader.h"
#include "shader_buffer.h"
#include "pipeline.h"

namespace llt
{
	class SubMesh;

	enum ShaderPassType
	{
		SHADER_PASS_FORWARD,
		SHADER_PASS_SHADOW,
		SHADER_PASS_MAX_ENUM
	};

	struct ShaderPass
	{
		ShaderEffect* shader;
		VkDescriptorSet set;
		GraphicsPipeline pipeline;
	};

	class Technique
	{
	public:
		Technique() = default;
		~Technique() = default;

		VertexFormat vertexFormat;

		ShaderEffect* passes[SHADER_PASS_MAX_ENUM];
		// todo: default parameters

		bool depthTest = true;
		bool depthWrite = true;
	};

	struct MaterialData
	{
		Vector<BoundTexture> textures;
		ShaderParameters parameters;
		String technique;

		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto& t : textures) {
				hash::combine(&result, &t);
			}

			hash::combine(&result, &technique);

			return result;
		}
	};

	class Material
	{
	public:
		Material() = default;
		~Material() = default;

		uint64_t getHash() const;

		void bindPipeline(ShaderPassType type);

		void render(ShaderPassType type, const RenderPass& pass);
		void renderMesh(ShaderPassType type, const SubMesh& mesh);

		VertexFormat vertexFormat;
		Vector<BoundTexture> textures;
		DynamicShaderBuffer* parameterBuffer;
		ShaderPass passes[SHADER_PASS_MAX_ENUM];
	};
}

#endif // MATERIAL_H_
