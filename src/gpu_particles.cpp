#include "gpu_particles.h"

#include "graphics/backend.h"
#include "graphics/shader_buffer_mgr.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace llt;

struct Particle
{
	glm::vec3 pos;
	float _padding0;
	glm::vec3 vel;
	float _padding1;
};

GPUParticles::GPUParticles()
	: m_particleBuffer()
	, m_computeProgram(nullptr)
	, m_computeParams()
	, m_computeParamsBuffer()
	, m_particleMesh()
	, m_particleVertexFormat()
	, m_particleGraphicsPipeline()
	, m_particleComputePipeline()
{
}

GPUParticles::~GPUParticles()
{
}

void GPUParticles::init(const ShaderBuffer* shaderParams)
{
	m_computeProgram = g_shaderManager->create("particleCompute", "../res/shaders/compute/particles.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	uint64_t particleDataSize = sizeof(Particle) * PARTICLE_COUNT;
	Particle* particleData = new Particle[PARTICLE_COUNT];

	for (int i = 0; i < PARTICLE_COUNT; i++)
	{
		particleData[i].pos.x = i - 0.2f;
		particleData[i].pos.y = 1.6f;
		particleData[i].pos.z = 0.0f;

		particleData[i].vel.x = 0.0f;
		particleData[i].vel.y = 0.0f;
		particleData[i].vel.z = 0.0f;
	}

	m_particleBuffer = g_shaderBufferManager->createSSBO();
	m_particleBuffer->pushData(particleData, particleDataSize);

	delete[] particleData;

	m_particleVertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));

	m_particleVertexFormat.addBinding(1, sizeof(Particle), VK_VERTEX_INPUT_RATE_INSTANCE);
	m_particleVertexFormat.addAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, pos));

	Vector<MyVertex> particleVtx =
	{
		// front face
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },

		// right face
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },

		// back face
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },

		// left face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },

		// top face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },

		// bottom face
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
	};

	Vector<uint16_t> particleIdx = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23
	};

	TextureSampler* pixelSampler = g_textureManager->getSampler("nearest");

	m_particleMesh.build(particleVtx.data(), particleVtx.size(), sizeof(MyVertex), particleIdx.data(), particleIdx.size());

	m_computeParamsBuffer = g_shaderBufferManager->createUBO();
	m_computeParams.set("viewProjMatrix", glm::identity<glm::mat4>());
	m_computeParamsBuffer->pushData(m_computeParams);

	m_particleComputePipeline.bindShader(m_computeProgram);

	m_particleComputePipeline.bindBuffer(0, m_computeParamsBuffer);
	m_particleComputePipeline.bindBuffer(1, m_particleBuffer);
	m_particleComputePipeline.bindTexture(2, g_renderTargetManager->get("gBuffer")->getAttachment(1), pixelSampler);
	m_particleComputePipeline.bindTexture(3, g_renderTargetManager->get("gBuffer")->getAttachment(2), pixelSampler);
	m_particleComputePipeline.bindTexture(4, g_renderTargetManager->get("gBuffer")->getDepthAttachment(), pixelSampler);

	m_particleGraphicsPipeline.setVertexDescriptor(m_particleVertexFormat);
	m_particleGraphicsPipeline.setDepthTest(true);
	m_particleGraphicsPipeline.setDepthWrite(true);
	m_particleGraphicsPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_particleGraphicsPipeline.bindShader(g_shaderManager->get("vertexInstanced"));
	m_particleGraphicsPipeline.bindShader(g_shaderManager->get("fragment"));
	m_particleGraphicsPipeline.bindBuffer(0, shaderParams);
	m_particleGraphicsPipeline.bindTexture(1, g_textureManager->getTexture("stone"), pixelSampler);
}

void GPUParticles::dispatchCompute(const Camera& camera)
{
	g_vulkanBackend->beginCompute();

	glm::mat4 vpMatrix = camera.getProj() * camera.getView();

	m_computeParams.set("viewProjMatrix", vpMatrix);
	m_computeParams.set("inverseViewProjMatrix", glm::inverse(vpMatrix));
	m_computeParamsBuffer->pushData(m_computeParams);

	m_particleComputePipeline.bind();
	m_particleComputePipeline.dispatch(1, 1, 1);

	g_vulkanBackend->endCompute();
}

void GPUParticles::render()
{
	RenderOp pass;
	pass.setInstanceData(PARTICLE_COUNT, 0, m_particleBuffer->getBuffer());
	pass.setMesh(m_particleMesh);

	g_vulkanBackend->waitOnCompute();

	g_vulkanBackend->beginGraphics(g_renderTargetManager->get("gBuffer"));

	m_particleGraphicsPipeline.bind();
	m_particleGraphicsPipeline.render(pass);

	g_vulkanBackend->endGraphics();
}
