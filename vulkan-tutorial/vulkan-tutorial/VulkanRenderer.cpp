#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
	
}

int VulkanRenderer::init(GLFWwindow* window)
{
	this->window = window;
	try
	{
		createInstance();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();

		setupDebugMessenger();
	} catch(const std::runtime_error& err)
	{
		printf("ERROR: %s\n", err.what());
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}

void VulkanRenderer::cleanup()
{
	for(const auto& image: swapChainImages)
	{
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapChain, nullptr);
	if (enableValidationLayers)
	{
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::setupDebugMessenger()
{
	if (!enableValidationLayers)
		return;
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

VkResult VulkanRenderer::createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanRenderer::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}
	uint32_t modeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, nullptr);
	if (modeCount > 0)
	{
		swapChainDetails.presentationModes.resize(modeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &modeCount, swapChainDetails.presentationModes.data());
	}
	return swapChainDetails;
}

// Best format is subjective, but ours will be:
// Format:		VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
// ColorSpace:	VK_COLOR_SPACE_SRGB_NON_LINEAR_KHR
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
	if (formats.size() == 1 && formats.at(0).format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	// if restricted, search for optimal format
	for (const auto& format: formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	// return first format
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	// Look for mailbox presentation mode
	for(const auto& presentationMode: presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}
	// if not available, use FIFO spec that Vulkan must support
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	auto newExtent = VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
	newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));
	return newExtent;
}



VulkanRenderer::~VulkanRenderer()
{
	
}

void VulkanRenderer::createInstance() 
{
	if (enableValidationLayers && !checkValidationLayersSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}
	// Info about app itself
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	// Custom version
	appInfo.pEngineName = "No Engine";	// Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_1;

	// Creation info for VkInstance (Vulkan instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();
	// set up extension instance will use
	uint32_t glfwExtensionCount = 0;		// GLFW may require multi extensions
	// Get GLFW extensions
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	// Add GLFW extensions to list
	for(size_t i = 0; i < glfwExtensionCount; ++i)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}
	if (enableValidationLayers)
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	// Check instance extensions supported.
	if (!checkInstanceExtensionSupport(instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// put debugCreateInfo outside of if statement to make sure it won't be disposed before calling vkCreateInstance
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
		createInfo.pNext = nullptr;
	}

	// Create instance
	auto result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance");
	}
}

void VulkanRenderer::getPhysicalDevice()
{
	// Enumerate physical devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If no devices available, then Vulkan not support!
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
	}
	// Get list of physical devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for(const auto& device: deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}

}

void VulkanRenderer::createLogicalDevice()
{
	// Getqueue family indices for the chosen physical device
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// Vector for queue creation info, and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices { indices.graphicsFamily, indices.presentationFamily };


	// Queue the logical device needs to create and info to do so
	for(auto queueFamilyIndex: queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;			// vulkan needs to know how to handle multiple queues, so device priority (1 = highest, 0 = lowest)

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// information to create logical device
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());				// number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.enabledLayerCount = 0;

	// Physical device features the logical device will use.
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	// Create logical device for given physical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device!");
	}
	// Queues are created at the same time as the device
	// so we want handle to queues
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface()
{
	// Create Surface (creates a surface create info struct, runs the create surface function, returns result)
	const auto result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create surface!");
	}
}

void VulkanRenderer::createSwapChain()
{
	// Get swap chain details so we can pick best settings
	SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);
	// Choose best surface format
	auto surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	// Choose best presentation mode
	auto presentationMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
	// choose swap chain image resolution
	auto extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	// How many images are in swap chain? Get 1 more than minimum to allow triple buffering.
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0	// 0 here means there's no limits.
		&& swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;

	// Get Queue Family Indices
	QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

	// If graphics and presentation families are different, then swapchain must let images be shared between families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] = {
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;		// Image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2;							// Number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;			// Array of queues to share between
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	// If old swap chain is destroyed and this one replaces it, then link old one to quickly hand over responsibilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	// Create Swapchain
	auto result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapChain);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a swapChain!");
	}
	// Store for later reference
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	// Get swap chain images (first count, then values)
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapChain, &swapChainImageCount, images.data());

	for (VkImage image: images)
	{
		// Store image handle
		SwapchainImage swapChainImage{};
		swapChainImage.image = image;
		// Create image view
		swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);\
		// save to image list
		swapChainImages.push_back(swapChainImage);
	}
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;											// Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;						// Type of image
	viewCreateInfo.format = format;											// Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Allows remapping of rgba components to other rgba values	
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// subresources allow the view to view only part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;				// which aspect of image to view (e.g. COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;						// start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;							// number of mipmap level to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;						// start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;							// number of array levels to view

	// create image view and return
	VkImageView imageView;
	auto result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view");
	}
	return imageView;
}


QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	// Get all queue family property info for the given device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	int i = 0;
	// Go through each queue family and check if it has at least 1 of the required types of queue
	for(const auto& queueFamily: queueFamilyList)
	{
		// check if queue family has at least 1 queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if it has required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;			// If queue family is valid, then get index
		}

		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		// check if queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}
		// Check if queue family indices are in a valid state, stop searching if so
		if(indices.isValid())
		{
			break;
		}
		++i;
	}
	return indices;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	printf("validation layer: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

bool VulkanRenderer::checkInstanceExtensionSupport(const std::vector<const char*>& checkExtensions)
{
	// Need to get number of extensions to create array of correct size to hold extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	for(const auto& checkExtension: checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension: extensions)
		{
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		return hasExtension;
	}
	return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	// if no extension found, return failure
	if (extensionCount == 0)
	{
		return false;
	}
	// populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	for(const auto& deviceExtension: deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension: extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		return hasExtension;
	}
	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// Information about device itself (ID, name, type, etc...)
	VkPhysicalDeviceProperties deviceProps;
	vkGetPhysicalDeviceProperties(device, &deviceProps);
	// Information about what device can do
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	auto indices = getQueueFamilies(device);
	const bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		const SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}
	return indices.isValid() && extensionsSupported && swapChainValid;

}

bool VulkanRenderer::checkValidationLayersSupport() const
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


	for(const char* layerName: validationLayers)
	{
		bool layerFound = false;

		for(const auto& layerProperties: availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		return layerFound;
	}
	return true;
}

