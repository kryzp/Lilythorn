#ifndef VK_UTIL_H_
#define VK_UTIL_H_

#include "third_party/volk.h"

#include "core/common.h"

#include "container/vector.h"
#include "container/optional.h"
#include "container/array.h"

#include "blend.h"
#include "shader.h"
#include "texture.h"
#include "texture_sampler.h"
#include "command_buffer.h"

#define LLT_VK_CHECK(_func_call, _error_msg) do{if(VkResult _VK_CHECK_RESULT_ABCDEFGH=_func_call;_VK_CHECK_RESULT_ABCDEFGH!=VK_SUCCESS){LLT_ERROR(_error_msg ": %d",_VK_CHECK_RESULT_ABCDEFGH);}}while(0);

namespace llt
{
	class VulkanCore;

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		Vector<VkSurfaceFormatKHR> surfaceFormats;
		Vector<VkPresentModeKHR> presentModes;
	};

	namespace vkutil
	{
#ifdef LLT_DEBUG
		static const char *VALIDATION_LAYERS[] = {
			"VK_LAYER_KHRONOS_validation"
		};
#endif // LLT_DEBUG

		static VkDynamicState DYNAMIC_STATES[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
//			VK_DYNAMIC_STATE_BLEND_CONSTANTS
		};

		static const char *DEVICE_EXTENSIONS[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
#ifdef LLT_MAC_SUPPORT
			"VK_KHR_portability_subset"
#endif // LLT_MAC_SUPPORT
		};

		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const Vector<VkSurfaceFormatKHR> &availableSurfaceFormats);
		VkPresentModeKHR chooseSwapPresentMode(const Vector<VkPresentModeKHR> &availablePresentModes, bool enableVsync);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

		uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t filter, VkMemoryPropertyFlags properties);

		VkFormat findSupportedFormat(VkPhysicalDevice device, const Vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat findDepthFormat(VkPhysicalDevice device);

		bool hasStencilComponent(VkFormat format);

		CommandBuffer beginSingleTimeCommands(VkCommandPool cmdPool);

		void endSingleTimeCommands(VkCommandPool cmdPool, const CommandBuffer &cmd, VkQueue graphics);

		void endSingleTimeGraphicsCommands(const CommandBuffer &cmd);
		void endSingleTimeTransferCommands(const CommandBuffer &cmd);

		uint64_t calcShaderBufferAlignedSize(uint64_t size);

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
		uint32_t assignPhysicalDeviceUsability(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties properties, VkPhysicalDeviceFeatures features, bool *hasEssentials);

		VkPipelineStageFlags getTransferPipelineStageFlags(VkImageLayout layout);
		VkAccessFlags getTransferAccessFlags(VkImageLayout layout);
	}
}

#endif // VK_UTIL_H_
