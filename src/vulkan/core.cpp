#include "core.h"

#include "core/platform.h"

#include "math/calc.h"

#include "third_party/volk.h"
#include "third_party/imgui/imgui_impl_sdl3.h"

#include "util.h"
#include "texture.h"
#include "swapchain.h"

#include "rendering/render_target_mgr.h"
#include "rendering/texture_mgr.h"
#include "rendering/shader_mgr.h"
#include "rendering/bindless_resource_mgr.h"

llt::VulkanCore *llt::g_vkCore = nullptr;

using namespace llt;

#if LLT_DEBUG

static bool g_debugEnableValidationLayers = false;

static bool debugHasValidationLayerSupport()
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	Vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (int i = 0; i < LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS); i++)
	{
		bool hasLayer = false;
		const char *layerName0 = vkutil::VALIDATION_LAYERS[i];

		for (int j = 0; j < availableLayers.size(); j++)
		{
			const char *layerName1 = availableLayers[j].layerName;

			if (cstr::compare(layerName0, layerName1) == 0) {
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer) {
			return false;
		}
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData
)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		LLT_ERROR("Validation Layer (SEVERITY: %d) (TYPE: %d): %s", messageSeverity, messageType, pCallbackData->pMessage);
	}

	return VK_FALSE;
}

static VkResult debugCreateDebugUtilsMessengerExt(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
	const VkAllocationCallbacks *allocator,
	VkDebugUtilsMessengerEXT *debugMessenger
)
{
	auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (fn) {
		return fn(instance, createInfo, allocator, debugMessenger);
	}

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void debugDestroyDebugUtilsMessengerExt(
	VkInstance instance,
	VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks *allocator
)
{
	auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (fn) {
		fn(instance, messenger, allocator);
	}
}

static void debugPopulateDebugUtilsMessengerCreateInfoExt(VkDebugUtilsMessengerCreateInfoEXT *info)
{
	info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	info->pfnUserCallback = vkDebugCallback;
	info->pUserData = nullptr;
}

#endif // LLT_DEBUG

static Vector<const char*> getInstanceExtensions()
{
	uint32_t extCount = 0;
	const char *const *names = g_platform->vkGetInstanceExtensions(&extCount);

	if (!names) {
		LLT_ERROR("Unable to get instance extension count.");
	}

	Vector<const char*> extensions(extCount);

	for (int i = 0; i < extCount; i++) {
		extensions[i] = names[i];
	}

#if LLT_DEBUG
	extensions.pushBack(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensions.pushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // LLT_DEBUG

#if LLT_MAC_SUPPORT
	extensions.pushBack(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	extensions.pushBack("VK_EXT_metal_surface");
#endif // LLT_MAC_SUPPORT

	extensions.pushBack(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

VulkanCore::VulkanCore(const Config &config)
	: m_instance()
	, m_device()
	, m_physicalData()
	, m_maxMsaaSamples()
	, m_swapChainImageFormat()
	, m_vmaAllocator()
	, m_pipelineCache()
	, m_descriptorLayoutCache()
	, m_graphicsQueue()
	, m_computeQueues()
	, m_transferQueues()
	, m_pipelineProcessCache()
	, m_swapchain()
	, m_currentFrameIdx()
#if LLT_DEBUG
	, m_debugMessenger()
#endif // LLT_DEBUG
	, m_imGuiDescriptorPool()
{
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = config.name,
		.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.pEngineName = "Lilythorn",
		.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	volkInitialize();

#if LLT_DEBUG
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (debugHasValidationLayerSupport())
	{
		LLT_LOG("Validation layer support verified.");

		debugPopulateDebugUtilsMessengerCreateInfoExt(&debugCreateInfo);

		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

		g_debugEnableValidationLayers = true;
	}
	else
	{
		LLT_LOG("No validation layer support.");

		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;

		g_debugEnableValidationLayers = false;
	}
#endif // LLT_DEBUG

	// get our extensions
	auto extensions = getInstanceExtensions();
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

#if LLT_MAC_SUPPORT
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif // LLT_MAC_SUPPORT

	// create the vulkan instance
	LLT_VK_CHECK(
		vkCreateInstance(&createInfo, nullptr, &m_instance),
		"Failed to create instance"
	);

	volkLoadInstance(m_instance);

#if LLT_DEBUG
	LLT_VK_CHECK(
		debugCreateDebugUtilsMessengerExt(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger),
		"Failed to create debug messenger"
	);
#endif // LLT_DEBUG

	m_pipelineCache.init();

	// set up all of the core managers of resources
	g_gpuBufferManager		= new GPUBufferMgr();
	g_textureManager      	= new TextureMgr();
	g_shaderManager       	= new ShaderMgr();
	g_renderTargetManager 	= new RenderTargetMgr();
	g_bindlessResources		= new BindlessResourceManager();

	// finished :D
	LLT_LOG("Vulkan Backend Initialized!");
}

VulkanCore::~VulkanCore()
{
	// wait until we are synced-up with the gpu before proceeding
	syncStall();

	// ---

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	m_descriptorLayoutCache.cleanUp();

	m_imGuiDescriptorPool.cleanUp();

	m_swapchain->cleanUpSwapChain();

	delete g_bindlessResources;
	delete g_renderTargetManager;
	delete g_shaderManager;
	delete g_textureManager;
	delete g_gpuBufferManager;

	m_swapchain->cleanUpTextures();

	m_pipelineCache.dispose();
	vkDestroyPipelineCache(m_device, m_pipelineProcessCache, nullptr);

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyFence(m_device, m_graphicsQueue.getFrame(i).inFlightFence, nullptr);
		vkDestroyCommandPool(m_device, m_graphicsQueue.getFrame(i).commandPool, nullptr);

		for (int j = 0; j < m_computeQueues.size(); j++) {
			vkDestroyFence(m_device, m_computeQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(m_device, m_computeQueues[j].getFrame(i).commandPool, nullptr);
		}

		for (int j = 0; j < m_transferQueues.size(); j++) {
			vkDestroyFence(m_device, m_transferQueues[j].getFrame(i).inFlightFence, nullptr);
			vkDestroyCommandPool(m_device, m_transferQueues[j].getFrame(i).commandPool, nullptr);
		}
	}

	delete m_swapchain;

	vmaDestroyAllocator(m_vmaAllocator);

	vkDestroyDevice(m_device, nullptr);

#if LLT_DEBUG
	// if we have validation layers then destroy them
	if (g_debugEnableValidationLayers) {
		debugDestroyDebugUtilsMessengerExt(m_instance, m_debugMessenger, nullptr);
		LLT_LOG("Destroyed validation layers!");
	}
#endif // LLT_DEBUG

	// finished with vulkan, may now safely destroy the vulkan instance
	vkDestroyInstance(m_instance, nullptr);

	// //

	LLT_LOG("Vulkan Backend Destroyed!");
}

void VulkanCore::enumeratePhysicalDevices()
{
	VkSurfaceKHR surface = m_swapchain->getSurface();

	// get the total devices we have
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	// no viable physical device found, exit program
	if (!deviceCount) {
		LLT_ERROR("Failed to find GPUs with Vulkan support!");
	}

	VkPhysicalDeviceProperties2 properties = {};
	properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

	VkPhysicalDeviceFeatures2 features = {};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	Vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	vkGetPhysicalDeviceProperties2(devices[0], &properties);
	vkGetPhysicalDeviceFeatures2(devices[0], &features);

	m_physicalData.device = devices[0];
	m_physicalData.properties = properties;
	m_physicalData.features = features;

	// get the actual number of samples we can take
	m_maxMsaaSamples = getMaxUsableSampleCount();

	// try to rank our physical devices and select one accordingly
	bool hasEssentials = false;
	uint32_t usability0 = vkutil::assignPhysicalDeviceUsability(surface, m_physicalData.device, properties, features, &hasEssentials);

	// select the device of the highest usability
	int i = 1;
	for (; i < deviceCount; i++)
	{
		vkGetPhysicalDeviceProperties2(devices[i], &m_physicalData.properties);
		vkGetPhysicalDeviceFeatures2(devices[i], &m_physicalData.features);

		uint32_t usability1 = vkutil::assignPhysicalDeviceUsability(surface, devices[i], properties, features, &hasEssentials);

		if (usability1 > usability0 && hasEssentials)
		{
			usability0 = usability1;

			m_physicalData.device = devices[i];
			m_physicalData.properties = properties;
			m_physicalData.features = features;

			m_maxMsaaSamples = getMaxUsableSampleCount();
		}
	}

	// still no device with the abilities we need, exit the program
	if (m_physicalData.device == VK_NULL_HANDLE) {
		LLT_ERROR("Unable to find a suitable GPU!");
	}

	LLT_LOG("Selected a suitable GPU: %d", i);
}

void VulkanCore::createLogicalDevice()
{
	const float QUEUE_PRIORITY = 1.0f;

	Vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	queueCreateInfos.pushBack({
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = m_graphicsQueue.getFamilyIdx().value(),
		.queueCount = 1,
		.pQueuePriorities = &QUEUE_PRIORITY
	});

	for (auto &computeQueue : m_computeQueues)
	{
		if (!computeQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		queueCreateInfos.pushBack({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = computeQueue.getFamilyIdx().value(),
			.queueCount = 1,
			.pQueuePriorities = &QUEUE_PRIORITY
		});
	}

	for (auto &transferQueue : m_transferQueues)
	{
		if (!transferQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		queueCreateInfos.pushBack({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = transferQueue.getFamilyIdx().value(),
			.queueCount = 1,
			.pQueuePriorities = &QUEUE_PRIORITY
		});
	}

	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeaturesExt = {};
	descriptorIndexingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexingFeaturesExt.runtimeDescriptorArray = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingPartiallyBound = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
	descriptorIndexingFeaturesExt.pNext = nullptr;

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeaturesExt = {};
	dynamicRenderingFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeaturesExt.dynamicRendering = VK_TRUE;
	dynamicRenderingFeaturesExt.pNext = &descriptorIndexingFeaturesExt;

	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeaturesExt = {};
	bufferDeviceAddressFeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeaturesExt.bufferDeviceAddress = VK_TRUE;
	bufferDeviceAddressFeaturesExt.pNext = &dynamicRenderingFeaturesExt;

	VkPhysicalDeviceSynchronization2Features synchronisation2FeaturesExt = {};
	synchronisation2FeaturesExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	synchronisation2FeaturesExt.synchronization2 = VK_TRUE;
	synchronisation2FeaturesExt.pNext = &bufferDeviceAddressFeaturesExt;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.ppEnabledExtensionNames = vkutil::DEVICE_EXTENSIONS;
	createInfo.enabledExtensionCount = LLT_ARRAY_LENGTH(vkutil::DEVICE_EXTENSIONS);
	createInfo.pEnabledFeatures = &m_physicalData.features.features;
	createInfo.pNext = &synchronisation2FeaturesExt;

#if LLT_DEBUG
	// enable the validation layers on the device
	if (g_debugEnableValidationLayers) {
		createInfo.enabledLayerCount = LLT_ARRAY_LENGTH(vkutil::VALIDATION_LAYERS);
		createInfo.ppEnabledLayerNames = vkutil::VALIDATION_LAYERS;
	}
	LLT_LOG("Enabled validation layers!");
#endif // LLT_DEBUG

	// create it
	LLT_VK_CHECK(
		vkCreateDevice(m_physicalData.device, &createInfo, nullptr, &m_device),
		"Failed to create logical device"
	);

	volkLoadDevice(m_device);

	// get the device queues for the four core vulkan families
	VkQueue tmpQueue;

	vkGetDeviceQueue(m_device, m_graphicsQueue.getFamilyIdx().value(), 0, &tmpQueue);
	m_graphicsQueue.init(tmpQueue);

	for (auto &computeQueue : m_computeQueues)
	{
		if (!computeQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(m_device, computeQueue.getFamilyIdx().value(), 0, &tmpQueue);
		computeQueue.init(tmpQueue);
	}

	for (auto &transferQueue : m_transferQueues)
	{
		if (!transferQueue.getFamilyIdx().hasValue()) {
			continue;
		}

		vkGetDeviceQueue(m_device, transferQueue.getFamilyIdx().value(), 0, &tmpQueue);
		transferQueue.init(tmpQueue);
	}

	// init memory allocator
	createVmaAllocator();

	// print out current device version
	uint32_t version = 0;
	VkResult result = vkEnumerateInstanceVersion(&version);

	if (result == VK_SUCCESS)
	{
		uint32_t major = VK_VERSION_MAJOR(version);
		uint32_t minor = VK_VERSION_MINOR(version);

		LLT_LOG("Using version: %d.%d", major, minor);
	}

	LLT_LOG("Created logical device!");
}

void VulkanCore::createVmaAllocator()
{
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.physicalDevice = m_physicalData.device;
	allocatorCreateInfo.device = m_device;
	allocatorCreateInfo.instance = m_instance;
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	LLT_VK_CHECK(
		vmaCreateAllocator(&allocatorCreateInfo, &m_vmaAllocator),
		"Failed to create memory allocator"
	);

	LLT_LOG("Created memory allocator!");
}

void VulkanCore::createPipelineProcessCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = nullptr;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = nullptr;

	LLT_VK_CHECK(
		vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineProcessCache),
		"Failed to process pipeline cache"
	);

	LLT_LOG("Created graphics pipeline process cache!");
}

/*
void VulkanBackend::createComputeResources()
{
	VkSemaphoreCreateInfo computeSemaphoreCreateInfo = {};
	computeSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		LLT_VK_CHECK(
			vkCreateSemaphore(m_device, &computeSemaphoreCreateInfo, nullptr, &m_computeFinishedSemaphores[i]),
			"Failed to create compute finished semaphore"
		);
	}

	LLT_LOG("Created compute resources!");
}
*/

void VulkanCore::createCommandPools()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		createInfo.queueFamilyIndex = m_graphicsQueue.getFamilyIdx().value();

		LLT_VK_CHECK(
			vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsQueue.getFrame(i).commandPool),
			"Failed to create graphics command pool"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = m_computeQueues[j].getFamilyIdx().value();

			LLT_VK_CHECK(
				vkCreateCommandPool(m_device, &createInfo, nullptr, &m_computeQueues[j].getFrame(i).commandPool),
				"Failed to create compute command pool"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			createInfo.queueFamilyIndex = m_transferQueues[j].getFamilyIdx().value();

			LLT_VK_CHECK(
				vkCreateCommandPool(m_device, &createInfo, nullptr, &m_transferQueues[j].getFrame(i).commandPool),
				"Failed to create transfer command pool"
			);
		}
	}

	LLT_LOG("Created command pool!");
}

void VulkanCore::createCommandBuffers()
{
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_graphicsQueue.getFrame(i).commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		LLT_VK_CHECK(
			vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &m_graphicsQueue.getFrame(i).commandBuffer),
			"Failed to create graphics command buffer"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			auto &computeQueue = m_computeQueues[j];

			commandBufferAllocateInfo.commandPool = computeQueue.getFrame(i).commandPool;

			LLT_VK_CHECK(
				vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &computeQueue.getFrame(i).commandBuffer),
				"Failed to create compute command buffer"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			auto &transferQueue = m_transferQueues[j];

			commandBufferAllocateInfo.commandPool = transferQueue.getFrame(i).commandPool;

			LLT_VK_CHECK(
				vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, &transferQueue.getFrame(i).commandBuffer),
				"Failed to create transfer command buffer"
			);
		}
	}

	LLT_LOG("Created command buffer!");
}

Swapchain *VulkanCore::createSwapchain()
{
	m_swapchain = new Swapchain();
	m_swapchain->createSurface();

	enumeratePhysicalDevices();

	findQueueFamilies(m_physicalData.device, m_swapchain->getSurface());

	createLogicalDevice();
	createPipelineProcessCache();

	createCommandPools();
	createCommandBuffers();

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		LLT_VK_CHECK(
			vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_graphicsQueue.getFrame(i).inFlightFence),
			"Failed to create graphics in flight fence"
		);

		for (int j = 0; j < m_computeQueues.size(); j++)
		{
			LLT_VK_CHECK(
				vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_computeQueues[j].getFrame(i).inFlightFence),
				"Failed to create compute in flight fence"
			);
		}

		for (int j = 0; j < m_transferQueues.size(); j++)
		{
			LLT_VK_CHECK(
				vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_transferQueues[j].getFrame(i).inFlightFence),
				"Failed to create transfer in flight fence"
			);
		}
	}

	LLT_LOG("Created in flight fence!");

//	createComputeResources();

	g_bindlessResources->init();

	m_swapchain->finalise();

	return m_swapchain;
}

VkSampleCountFlagBits VulkanCore::getMaxUsableSampleCount() const
{
	VkSampleCountFlags counts =
		m_physicalData.properties.properties.limits.framebufferColorSampleCounts &
		m_physicalData.properties.properties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT) {
		return VK_SAMPLE_COUNT_64_BIT;
	} else if (counts & VK_SAMPLE_COUNT_32_BIT) {
		return VK_SAMPLE_COUNT_32_BIT;
	} else if (counts & VK_SAMPLE_COUNT_16_BIT) {
		return VK_SAMPLE_COUNT_16_BIT;
	} else if (counts & VK_SAMPLE_COUNT_8_BIT) {
		return VK_SAMPLE_COUNT_8_BIT;
	} else if (counts & VK_SAMPLE_COUNT_4_BIT) {
		return VK_SAMPLE_COUNT_4_BIT;
	} else if (counts & VK_SAMPLE_COUNT_2_BIT) {
		return VK_SAMPLE_COUNT_2_BIT;
	} else if (counts & VK_SAMPLE_COUNT_1_BIT) {
		return VK_SAMPLE_COUNT_1_BIT;
	}

	LLT_ERROR("Could not find a maximum usable sample count!");
	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanCore::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	if (!queueFamilyCount) {
		LLT_ERROR("Failed to find any queue families!");
	}

	Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	uint32_t numQueuesFound[QUEUE_FAMILY_MAX_ENUM] = {};

	for (int i = 0; i < queueFamilyCount; i++)
	{
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && numQueuesFound[i] == 0)
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport)
			{
				m_graphicsQueue.setData(QUEUE_FAMILY_GRAPHICS, i);
				numQueuesFound[QUEUE_FAMILY_GRAPHICS]++;
				continue;
			}
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && numQueuesFound[QUEUE_FAMILY_COMPUTE] < queueFamilies[i].queueCount)
		{
			m_computeQueues.emplaceBack();
			m_computeQueues.back().setData(QUEUE_FAMILY_COMPUTE, i);
			numQueuesFound[QUEUE_FAMILY_COMPUTE]++;
			continue;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && numQueuesFound[QUEUE_FAMILY_TRANSFER] < queueFamilies[i].queueCount)
		{
			m_transferQueues.emplaceBack();
			m_transferQueues.back().setData(QUEUE_FAMILY_TRANSFER, i);
			numQueuesFound[QUEUE_FAMILY_TRANSFER]++;
			continue;
		}
	}
}

void VulkanCore::onWindowResize(int width, int height)
{
	m_swapchain->onWindowResize(width, height);
}

int VulkanCore::getCurrentFrameIdx() const
{
	return m_currentFrameIdx;
}

VkCommandPool VulkanCore::getGraphicsCommandPool()
{
	return m_graphicsQueue.getCurrentFrame().commandPool;
}

VkCommandPool VulkanCore::getTransferCommandPool(int idx)
{
//	if (m_transferQueues.size() > 0)
//		return m_transferQueues[idx].getCurrentFrame().commandPool;

	return getGraphicsCommandPool();
}

VkCommandPool VulkanCore::getComputeCommandPool(int idx)
{
	return m_computeQueues[idx].getCurrentFrame().commandPool;
}

PipelineCache &VulkanCore::getPipelineCache()
{
	return m_pipelineCache;
}

const PipelineCache &VulkanCore::getPipelineCache() const
{
	return m_pipelineCache;
}

DescriptorLayoutCache &VulkanCore::getDescriptorLayoutCache()
{
	return m_descriptorLayoutCache;
}

const DescriptorLayoutCache &VulkanCore::getDescriptorLayoutCache() const
{
	return m_descriptorLayoutCache;
}

void VulkanCore::swapBuffers()
{
	cauto &currentFrame = g_vkCore->m_graphicsQueue.getCurrentFrame();

	vkWaitForFences(g_vkCore->m_device, 1, &currentFrame.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetCommandPool(g_vkCore->m_device, currentFrame.commandPool, 0);

	m_swapchain->swapBuffers();

	m_currentFrameIdx = (m_currentFrameIdx + 1) % mgc::FRAMES_IN_FLIGHT;

//	g_shaderBufferManager->resetBufferUsageInFrame();

	m_swapchain->acquireNextImage();
}

void VulkanCore::syncStall() const
{
	while (g_platform->getWindowSize() == glm::ivec2(0, 0)) {}
	vkDeviceWaitIdle(m_device);
}

void VulkanCore::createImGuiResources()
{
	m_imGuiDescriptorPool.init(1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					1.0f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	1.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			1.0f }
	});
}

ImGui_ImplVulkan_InitInfo VulkanCore::getImGuiInitInfo() const
{
	ImGui_ImplVulkan_InitInfo info = {};
	info.Instance = m_instance;
	info.PhysicalDevice = m_physicalData.device;
	info.Device = m_device;
	info.QueueFamily = m_graphicsQueue.getFamilyIdx().value();
	info.Queue = m_graphicsQueue.getQueue();
	info.PipelineCache = m_pipelineProcessCache;
	info.DescriptorPool = m_imGuiDescriptorPool.getPool();
	info.Allocator = nullptr;
	info.MinImageCount = mgc::FRAMES_IN_FLIGHT;
	info.ImageCount = mgc::FRAMES_IN_FLIGHT;
	info.CheckVkResultFn = nullptr;
	info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	info.UseDynamicRendering = true;
	info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
	info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_swapChainImageFormat;

	return info;
}
