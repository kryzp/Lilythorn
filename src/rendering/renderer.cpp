#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "input/input.h"

#include "vulkan/backend.h"
#include "vulkan/backbuffer.h"
#include "vulkan/command_buffer.h"
#include "vulkan/texture.h"

#include "rendering/material.h"
#include "rendering/material_system.h"
#include "rendering/mesh_loader.h"
#include "rendering/texture_mgr.h"
#include "rendering/render_target_mgr.h"

#include "math/colour.h"

using namespace llt;

Renderer::Renderer()
//	, m_gBuffer(nullptr)
//	, m_gpuParticles()
	: m_currentScene()
	, m_skyboxMesh()
	, m_skyboxPipeline()
	, m_skyboxSet()
	, m_descriptorPool()
	, m_descriptorLayoutCache()
//	, m_postProcessPipeline()
{
}

void Renderer::init()
{
	g_gpuBufferManager->createGlobalStagingBuffers();

	m_descriptorPool.init(64 * mgc::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f }
	});

	initVertexTypes();

	g_meshLoader = new MeshLoader();

	g_textureManager->loadDefaultTexturesAndSamplers();
	g_shaderManager->loadDefaultShaders();

	g_materialSystem = new MaterialSystem();
	g_materialSystem->loadDefaultTechniques();
	g_materialSystem->init();

	createSkyboxResources();

	addRenderObjects();

//	m_gBuffer = g_renderTargetManager->createTarget(
//		"gBuffer", // YES I KNOW THIS ISNT A DEFERRED RENDERING GBUFFER BUT WHATEVER
//		m_backbuffer->getWidth(),
//		m_backbuffer->getHeight(),
//		{
//			VK_FORMAT_B8G8R8A8_UNORM, // colour
//			VK_FORMAT_R32G32_SFLOAT, // motion
//			VK_FORMAT_R32G32B32A32_SFLOAT // normals
//		},
//		VK_SAMPLE_COUNT_1_BIT
//	);
//
//	m_gBuffer->createDepthAttachment();
//
//	m_gpuParticles.init(m_shaderParamsBuffer);
//	
//	createPostProcessResources();
}

void Renderer::cleanUp()
{
	m_descriptorPool.cleanUp();
	m_descriptorLayoutCache.cleanUp();

	delete g_meshLoader;
	delete g_materialSystem;
}

void Renderer::addRenderObjects()
{
	auto assimpModel = m_currentScene.createRenderObject();
	assimpModel->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	assimpModel->transform.setRotation(glm::radians(0.0f), { 1.0f, 0.0f, 0.0f });
	assimpModel->transform.setScale({ 4.0f, 4.0f, 4.0f });
	assimpModel->transform.setOrigin({ 0.0f, 0.0f, 0.0f });
	assimpModel->mesh = g_meshLoader->loadMesh("model", "../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");
	assimpModel->mesh->setOwner(&(*assimpModel));
}

void Renderer::createPostProcessResources()
{
/*
	DescriptorLayoutBuilder ppDescriptorLayoutBuilder;
	ppDescriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	BoundTexture gBufferAttachment;
	gBufferAttachment.texture = m_gBuffer->getAttachment(0);
	gBufferAttachment.sampler = g_textureManager->getSampler("linear");

	DescriptorWriter ppDescriptorWriter;
	ppDescriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gBufferAttachment.getImageInfo());
	
	VkDescriptorSetLayout ppDescriptorSetLayout = ppDescriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, nullptr);
	
	ShaderProgram *ppVS = g_shaderManager->create("postProcessVertex", "../../res/shaders/raster/post_process_vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
	ShaderProgram *ppPS = g_shaderManager->create("postProcessFragment", "../../res/shaders/raster/post_process_ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	
	m_postProcessPipeline.setVertexDescriptor(g_modelVertexFormat);
	m_postProcessPipeline.setDepthTest(false);
	m_postProcessPipeline.setDepthWrite(false);
	m_postProcessPipeline.bindShader(ppVS);
	m_postProcessPipeline.bindShader(ppPS);
	m_postProcessPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_postProcessPipeline.setDescriptorSetLayout(ppDescriptorSetLayout);
	
	m_descriptorPool.setSizes(64 * mgc::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f }
	});
	
	m_postProcessDescriptorSet = m_descriptorPool.allocate(ppDescriptorSetLayout);
	
	ppDescriptorWriter.updateSet(m_postProcessDescriptorSet);
*/
}

void Renderer::createSkyboxResources()
{
	Vector<PrimitiveVertex> skyboxVertices =
	{
		{ { -0.5f,  0.5f,  0.5f } },
		{ { -0.5f, -0.5f,  0.5f } },
		{ {  0.5f, -0.5f,  0.5f } },
		{ {  0.5f,  0.5f,  0.5f } },
		{ { -0.5f,  0.5f, -0.5f } },
		{ { -0.5f, -0.5f, -0.5f } },
		{ {  0.5f, -0.5f, -0.5f } },
		{ {  0.5f,  0.5f, -0.5f } }
	};

	Vector<uint16_t> skyboxIndices =
	{
		0, 1, 2,
		2, 3, 0,

		5, 4, 7,
		7, 6, 5,

		1, 5, 6,
		6, 2, 1,

		4, 0, 3,
		3, 7, 4,

		4, 5, 1,
		1, 0, 4,

		3, 2, 6,
		6, 7, 3
	};

	m_skyboxMesh.build(
		g_primitiveVertexFormat,
		skyboxVertices.data(), skyboxVertices.size(),
		skyboxIndices.data(), skyboxIndices.size()
	);

	BoundTexture skyboxCubemap;
	skyboxCubemap.texture = g_textureManager->getTexture("environmentMap");
	skyboxCubemap.sampler = g_textureManager->getSampler("linear");

	DescriptorLayoutBuilder descriptorLayoutBuilder;
	DescriptorWriter descriptorWriter;

	descriptorLayoutBuilder.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, skyboxCubemap.getImageInfo());

	VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS, &m_descriptorLayoutCache);
	m_skyboxSet = m_descriptorPool.allocate(layout);

	descriptorWriter.updateSet(m_skyboxSet);

	m_skyboxPipeline.setPushConstantsSize(sizeof(float)*16 * 2);
	m_skyboxPipeline.bindShader(g_shaderManager->get("primitiveVS"));
	m_skyboxPipeline.bindShader(g_shaderManager->get("skyboxFragment"));
	m_skyboxPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_skyboxPipeline.setDepthTest(false);
	m_skyboxPipeline.setDepthWrite(false);
	m_skyboxPipeline.setCullMode(VK_CULL_MODE_BACK_BIT);
	m_skyboxPipeline.setDescriptorSetLayout(layout);
	m_skyboxPipeline.setMSAA(g_vulkanBackend->m_backbuffer->getMSAA());
}

void Renderer::renderSkybox(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target)
{
	RenderInfo info = target->getRenderInfo();

	ShaderParameters params;
	params.setValue<glm::mat4>("proj", camera.getProj());
	params.setValue<glm::mat4>("view", camera.getRotationMatrix());

	if (m_skyboxPipeline.getPipeline() == VK_NULL_HANDLE)
		m_skyboxPipeline.buildGraphicsPipeline(info);

	buffer.bindPipeline(m_skyboxPipeline);

	buffer.bindDescriptorSets(
		m_skyboxPipeline.getBindPoint(),
		m_skyboxPipeline.getPipelineLayout(),
		0,
		1, &m_skyboxSet,
		0, nullptr
	);

	buffer.setViewport(info);
	buffer.setScissor(info);

	buffer.pushConstants(
		m_skyboxPipeline.getPipelineLayout(),
		m_skyboxPipeline.getShaderStage(),
		params
	);

	m_skyboxMesh.render(buffer);
}

void Renderer::setScene(const Scene &scene)
{
	m_currentScene = scene;
}

void Renderer::render(const Camera &camera, float deltaTime)
{
	g_materialSystem->updateImGui();

	ImGui::ShowDemoWindow();
	ImGui::Render();

	m_currentScene.updatePrevMatrices();

//	m_gBuffer->toggleClear(true);
//	m_gBuffer->setClearColours(Colour::black());

	CommandBuffer graphicsBuffer = CommandBuffer::fromGraphics();

	graphicsBuffer.beginRendering(g_vulkanBackend->m_backbuffer);

	graphicsBuffer.setViewport(g_vulkanBackend->m_backbuffer->getRenderInfo());
	graphicsBuffer.setScissor(g_vulkanBackend->m_backbuffer->getRenderInfo());

	renderSkybox(graphicsBuffer, camera, g_vulkanBackend->m_backbuffer);

	m_currentScene.render(graphicsBuffer, camera, g_vulkanBackend->m_backbuffer);

	graphicsBuffer.endRendering();
	graphicsBuffer.submit();

	renderImGui(graphicsBuffer);

//	renderParticles(camera, deltaTime);
//	g_vulkanBackend->beginGraphics();
//	renderPostProcess();
//	g_vulkanBackend->endGraphics();

	g_vulkanBackend->swapBuffers();
}

void Renderer::renderImGui(CommandBuffer &buffer)
{
	RenderInfo renderInfo;
	
	renderInfo.setSize(
		g_vulkanBackend->m_backbuffer->getWidth(),
		g_vulkanBackend->m_backbuffer->getHeight()
	);
	
	renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		g_vulkanBackend->m_backbuffer->getCurrentSwapchainImageView(),
		g_vulkanBackend->getImGuiAttachmentFormat()
	);

	buffer.beginRendering(renderInfo);

	ImDrawData *drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, buffer.getBuffer());

	buffer.endRendering();
	buffer.submit();
}

void Renderer::renderParticles(CommandBuffer &buffer, const Camera &camera, const GenericRenderTarget *target, float deltaTime)
{
/*
	m_gBuffer->toggleClear(false);

	m_pushConstants.set("time", deltaTime);
	g_vulkanBackend->setPushConstants(m_pushConstants);
	m_gpuParticles.dispatchCompute(camera);

	glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
	m_shaderParams.set("currModelMatrix", particleMatrix);
	m_shaderParams.set("prevModelMatrix", particleMatrix);
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_gpuParticles.render();
*/
}

void Renderer::renderPostProcess()
{
/*
	RenderPass pass;
	pass.setMesh(m_quadMesh);

	m_postProcessPipeline.bind();

	vkCmdBindDescriptorSets(
		g_vulkanBackend->graphicsQueue.getCurrentFrame().commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_postProcessPipeline.getPipelineLayout(),
		0,
		1, &m_postProcessDescriptorSet,
		0, nullptr
	);

	m_postProcessPipeline.render(pass);
*/
}

// todo: water. post processing pipeline.
