#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	int init(GLFWwindow* window);
	void cleanup();
	~VulkanRenderer();

private:
	GLFWwindow* window;
	// Vulkan Components
	VkInstance instance;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;

	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	VkDebugUtilsMessengerEXT debugMessenger;
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();

	bool checkInstanceExtensionSupport(const std::vector<const char*>& extensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkValidationLayersSupport() const;
	void getPhysicalDevice();
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	void setupDebugMessenger();
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector <VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	);
};

