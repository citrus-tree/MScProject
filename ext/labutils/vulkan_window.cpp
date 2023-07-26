#include "vulkan_window.hpp"

#include <tuple>
#include <limits>
#include <vector>
#include <utility>
#include <optional>
#include <algorithm>
#include <unordered_set>

#include <cstdio>
#include <cassert>
#include <vulkan/vulkan_core.h>

#include "error.hpp"
#include "to_string.hpp"
#include "context_helpers.hxx"
namespace lut = labutils;


namespace
{
	// The device selection process has changed somewhat w.r.t. the one used 
	// earlier (e.g., with VulkanContext.
	VkPhysicalDevice select_device(VkInstance, VkSurfaceKHR);
	float score_device(VkPhysicalDevice, VkSurfaceKHR);

	std::optional<std::uint32_t> find_queue_family(VkPhysicalDevice, VkQueueFlags, VkSurfaceKHR = VK_NULL_HANDLE);

	VkDevice create_device(
		VkPhysicalDevice,
		std::vector<std::uint32_t> const& aQueueFamilies,
		std::vector<char const*> const& aEnabledDeviceExtensions = {},
		const labutils::VulkanWindow::OptionalDeviceFeatures& aFeatures = {}
	);

	std::vector<VkSurfaceFormatKHR> get_surface_formats(VkPhysicalDevice, VkSurfaceKHR);
	std::unordered_set<VkPresentModeKHR> get_present_modes(VkPhysicalDevice, VkSurfaceKHR);

	std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> create_swapchain(
		VkPhysicalDevice,
		VkSurfaceKHR,
		VkDevice,
		GLFWwindow*,
		std::vector<std::uint32_t> const& aQueueFamilyIndices = {},
		VkSwapchainKHR aOldSwapchain = VK_NULL_HANDLE
	);

	void get_swapchain_images(VkDevice, VkSwapchainKHR, std::vector<VkImage>&);
	void create_swapchain_image_views(VkDevice, VkFormat, std::vector<VkImage> const&, std::vector<VkImageView>&);
}

namespace labutils
{
	// VulkanWindow
	VulkanWindow::VulkanWindow() = default;

	VulkanWindow::~VulkanWindow()
	{
		// Device-related objects
		for (auto const view : swapViews)
			vkDestroyImageView(device, view, nullptr);

		if (VK_NULL_HANDLE != swapchain)
			vkDestroySwapchainKHR(device, swapchain, nullptr);

		// Window and related objects
		if (VK_NULL_HANDLE != surface)
			vkDestroySurfaceKHR(instance, surface, nullptr);

		if (window)
		{
			glfwDestroyWindow(window);

			// The following assumes that we never create more than one window;
			// if there are multiple windows, destroying one of them would
			// unload the whole GLFW library. Nevertheless, this solution is
			// convenient when only dealing with one window (which we will do
			// in the exercises), as it ensure that GLFW is unloaded after all
			// window-related resources are.
			glfwTerminate();
		}
	}

	VulkanWindow::VulkanWindow(VulkanWindow&& aOther) noexcept
		: VulkanContext(std::move(aOther))
		, window(std::exchange(aOther.window, VK_NULL_HANDLE))
		, surface(std::exchange(aOther.surface, VK_NULL_HANDLE))
		, presentFamilyIndex(aOther.presentFamilyIndex)
		, presentQueue(std::exchange(aOther.presentQueue, VK_NULL_HANDLE))
		, swapchain(std::exchange(aOther.swapchain, VK_NULL_HANDLE))
		, swapImages(std::move(aOther.swapImages))
		, swapViews(std::move(aOther.swapViews))
		, swapchainFormat(aOther.swapchainFormat)
		, swapchainExtent(aOther.swapchainExtent)
		, features(aOther.features)
	{}

	VulkanWindow& VulkanWindow::operator=(VulkanWindow&& aOther) noexcept
	{
		VulkanContext::operator=(std::move(aOther));
		std::swap(window, aOther.window);
		std::swap(surface, aOther.surface);
		std::swap(presentFamilyIndex, aOther.presentFamilyIndex);
		std::swap(presentQueue, aOther.presentQueue);
		std::swap(swapchain, aOther.swapchain);
		std::swap(swapImages, aOther.swapImages);
		std::swap(swapViews, aOther.swapViews);
		std::swap(swapchainFormat, aOther.swapchainFormat);
		std::swap(swapchainExtent, aOther.swapchainExtent);
		std::swap(features, aOther.features);
		return *this;
	}

	// make_vulkan_window()
	VulkanWindow make_vulkan_window()
	{
		VulkanWindow ret;

		// Initialize Volk
		if (auto const res = volkInitialize(); VK_SUCCESS != res)
		{
			throw lut::Error("Unable to load Vulkan API\n"
				"Volk returned error %s", lut::to_string(res).c_str()
			);
		}

		// Initialize GLFW
		if (glfwInit() != GLFW_TRUE)
		{
			const char* err = nullptr;
			glfwGetError(&err);
			throw lut::Error("GLFW: intialisation failed :( err: %s", err);
		}

		if (!glfwVulkanSupported())
		{
			throw lut::Error("GLFW: failed to find required Vulkan dependencies.");
		}


		// Check for instance layers and extensions
		auto const supportedLayers = detail::get_instance_layers();
		auto const supportedExtensions = detail::get_instance_extensions();

		bool enableDebugUtils = false;

		std::vector<char const*> enabledLayers, enabledExensions;

		// Handle GLFW extensions
		uint32_t reqExtCount = 0;
		const char** requiredExt = glfwGetRequiredInstanceExtensions(&reqExtCount);

		for (uint32_t i = 0; i < reqExtCount; ++i)
		{
			if (supportedExtensions.count(requiredExt[i]) == 0)
			{
				throw lut::Error("GLFW/VK: required instance extension [%s] not supported.", requiredExt[i]);
			}

			enabledExensions.push_back(requiredExt[i]);
		}

		// Validation layers support.
#		if !defined(NDEBUG) // debug builds only
		if (supportedLayers.count("VK_LAYER_KHRONOS_validation"))
		{
			enabledLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		}

		if (supportedExtensions.count("VK_EXT_debug_utils"))
		{
			enableDebugUtils = true;
			enabledExensions.emplace_back("VK_EXT_debug_utils");
		}
#		endif // ~ debug builds

		std::fprintf(stderr, " * Enabled layers:\n");
		for (auto const& layer : enabledLayers)
		{
			std::fprintf(stderr, "     -> %s\n", layer);
		}

		std::fprintf(stderr, " * Instance extensions:\n");
		for (auto const& ext : enabledExensions)
		{
			std::fprintf(stderr, "     -> %s\n", ext);
		}


		// Create Vulkan instance
		ret.instance = detail::create_instance(enabledLayers, enabledExensions, enableDebugUtils);

		// Load rest of the Vulkan API
		volkLoadInstance(ret.instance);

		// Setup debug messenger
		if (enableDebugUtils)
			ret.debugMessenger = detail::create_debug_messenger(ret.instance);

		// Window creation and get handle for surface
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		/* Disable window resize */
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		ret.window = glfwCreateWindow(1080, 720, "Modisett MSc Project: "
			"An Analysis of Shadow Mapping Techniques for Rendering Coloured Shadows", nullptr, nullptr);
		if (ret.window == nullptr)
		{
			const char* err = nullptr;
			glfwGetError(&err);

			throw lut::Error("GLFW: Unable to create window err: %s", err);
		}

		if (const auto res = glfwCreateWindowSurface(ret.instance, ret.window, nullptr, &ret.surface); res != VK_SUCCESS)
		{
			throw lut::Error("GLFW/VK: glfwCreateWindow() failed. %s", lut::to_string(res).c_str());
		}

		// Select appropriate Vulkan device
		ret.physicalDevice = select_device(ret.instance, ret.surface);
		if (VK_NULL_HANDLE == ret.physicalDevice)
			throw lut::Error("No suitable physical device found!");

		{
			/* Output selected device and selected api version */
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(ret.physicalDevice, &props);
			std::fprintf(stderr, "Selected device: %s (%d.%d.%d)\n", props.deviceName, VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion));

			/* Query for optional device features, then output if they are supported */
			VkPhysicalDeviceFeatures feats;
			vkGetPhysicalDeviceFeatures(ret.physicalDevice, &feats);
			ret.features.samplerAnisotropy = (feats.samplerAnisotropy == VK_TRUE);
			ret.features.maxSamplerAnisotropy = props.limits.maxSamplerAnisotropy;

			std::fprintf(stderr, " * Optional features:\n");
			std::fprintf(stderr, "     -> SamplerAnisotropy: %s\n", (ret.features.samplerAnisotropy) ? "YES" : "NO");
			std::fprintf(stderr, "          -> maxSamplerAnisotropy: %f\n", ret.features.maxSamplerAnisotropy);
		}

		// Create a logical device
		// Enable required extensions. The device selection method ensures that
		// the VK_KHR_swapchain extension is present, so we can safely just
		// request it without further checks.
		std::vector<char const*> enabledDevExensions;

		// Necessary device extensions:
		enabledDevExensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		std::fprintf(stderr, " * Device extensions:\n");
		for (auto const& ext : enabledDevExensions)
		{
			std::fprintf(stderr, "     -> %s\n", ext);
		}


		// We need one or two queues:
		// - best case: one GRAPHICS queue that can present
		// - otherwise: one GRAPHICS queue and any queue that can present
		std::vector<std::uint32_t> queueFamilyIndices;

		// Select necessary queue families to instantiate
		if (const auto index = find_queue_family(ret.physicalDevice, VK_QUEUE_GRAPHICS_BIT, ret.surface); index.has_value())
		{
			ret.graphicsFamilyIndex = *index;
			queueFamilyIndices.push_back(*index);
		}
		else
		{
			auto graphics = find_queue_family(ret.physicalDevice, VK_QUEUE_GRAPHICS_BIT);
			auto present = find_queue_family(ret.physicalDevice, 0, ret.surface);

			assert(graphics.has_value() && present.has_value());

			ret.graphicsFamilyIndex = *graphics;
			ret.presentFamilyIndex = *present;

			queueFamilyIndices.push_back(*graphics);
			queueFamilyIndices.push_back(*present);
		}

		ret.device = create_device(ret.physicalDevice, queueFamilyIndices, enabledDevExensions, ret.features);

		// Retrieve VkQueues
		vkGetDeviceQueue(ret.device, ret.graphicsFamilyIndex, 0, &ret.graphicsQueue);

		assert(VK_NULL_HANDLE != ret.graphicsQueue);

		if (queueFamilyIndices.size() >= 2)
			vkGetDeviceQueue(ret.device, ret.presentFamilyIndex, 0, &ret.presentQueue);
		else
		{
			ret.presentFamilyIndex = ret.graphicsFamilyIndex;
			ret.presentQueue = ret.graphicsQueue;
		}

		// Create swap chain
		std::tie(ret.swapchain, ret.swapchainFormat, ret.swapchainExtent) = create_swapchain(ret.physicalDevice, ret.surface, ret.device, ret.window, queueFamilyIndices);

		// Get swap chain images & create associated image views
		get_swapchain_images(ret.device, ret.swapchain, ret.swapImages);
		create_swapchain_image_views(ret.device, ret.swapchainFormat, ret.swapImages, ret.swapViews);

		// Done
		return ret;
	}

	void create_swapchain_framebuffers(VulkanWindow const& aWindow, VkRenderPass aRenderPass, std::vector<Framebuffer>& aFramebuffers, VkImageView aDepthBufferView)
	{
		assert(aFramebuffers.empty());

		for (auto i = 0U; i < aWindow.swapViews.size(); i++)
		{
			VkImageView attachments[2]
			{
				aWindow.swapViews[i],
				aDepthBufferView
			};

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.flags = 0;
			fbInfo.renderPass = aRenderPass;
			fbInfo.attachmentCount = 2;
			fbInfo.pAttachments = attachments;
			fbInfo.width = aWindow.swapchainExtent.width;
			fbInfo.height = aWindow.swapchainExtent.height;
			fbInfo.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			if (const auto& res = vkCreateFramebuffer(aWindow.device, &fbInfo, nullptr, &framebuffer); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateFramebuffer() failed to create the [%u]th framebuffer for the swapchain. err: %s",
					i, lut::to_string(res).c_str());
			}

			aFramebuffers.push_back(lut::Framebuffer(aWindow.device, framebuffer));
		}

		assert(aWindow.swapViews.size() == aFramebuffers.size());
	}

	SwapChanges recreate_swapchain(VulkanWindow& aWindow)
	{
		/* Store info and items we need to keep around */
		const auto oldFormat = aWindow.swapchainFormat;
		const auto oldExtent = aWindow.swapchainExtent;
		VkSwapchainKHR oldSwapchain = aWindow.swapchain;

		/* Destroy the old image views */
		for (auto view : aWindow.swapViews)
			vkDestroyImageView(aWindow.device, view, nullptr);

		/* Clear the old image views and images */
		aWindow.swapViews.clear();
		aWindow.swapImages.clear();

		/* Create the new swap chain */
		std::vector<uint32_t> queueFamilyIndices;
		if (aWindow.presentFamilyIndex != aWindow.graphicsFamilyIndex)
		{
			queueFamilyIndices.push_back(aWindow.presentFamilyIndex);
			queueFamilyIndices.push_back(aWindow.graphicsFamilyIndex);
		}

		try
		{
			std::tie(aWindow.swapchain, aWindow.swapchainFormat, aWindow.swapchainExtent) =
				create_swapchain(aWindow.physicalDevice, aWindow.surface, aWindow.device, aWindow.window, queueFamilyIndices, oldSwapchain);
		}
		catch (...)
		{
			aWindow.swapchain = oldSwapchain;
			throw;
		}

		/* Destroy the old swap chain */
		vkDestroySwapchainKHR(aWindow.device, oldSwapchain, nullptr);

		/* Image and image view setup for new swap chain */
		get_swapchain_images(aWindow.device, aWindow.swapchain, aWindow.swapImages);
		create_swapchain_image_views(aWindow.device, aWindow.swapchainFormat, aWindow.swapImages, aWindow.swapViews);

		SwapChanges ret{};
		if (aWindow.swapchainExtent.width != oldExtent.width || aWindow.swapchainExtent.height != oldExtent.height)
			ret.changedSize = true;
		if (aWindow.swapchainFormat != oldFormat)
			ret.changedFormat = true;

		return ret;
	}
}

namespace
{
	std::vector<VkSurfaceFormatKHR> get_surface_formats(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface)
	{
		uint32_t formatCount = 0;
		if (const auto& res = vkGetPhysicalDeviceSurfaceFormatsKHR(aPhysicalDev, aSurface, &formatCount, nullptr); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkGetPhysicalDeviceSurfaceFormatsKHR () failed to provide surface presentation mode count. err: %s",
				lut::to_string(res).c_str());
		}

		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		if (const auto& res = vkGetPhysicalDeviceSurfaceFormatsKHR(aPhysicalDev, aSurface, &formatCount, formats.data()); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkGetPhysicalDeviceSurfaceFormatsKHR () failed to enumerate surface presentation modes. err: %s",
				lut::to_string(res).c_str());
		}

		return formats;
	}

	std::unordered_set<VkPresentModeKHR> get_present_modes(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface)
	{
		uint32_t modeCount = 0;
		if (const auto& res = vkGetPhysicalDeviceSurfacePresentModesKHR(aPhysicalDev, aSurface, &modeCount, nullptr); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkGetPhysicalDeviceSurfacePresentModesKHR() failed to provide surface presentation mode count. err: %s",
				lut::to_string(res).c_str());
		}

		std::vector<VkPresentModeKHR> modes(modeCount);
		if (const auto& res = vkGetPhysicalDeviceSurfacePresentModesKHR(aPhysicalDev, aSurface, &modeCount, modes.data()); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkGetPhysicalDeviceSurfacePresentModesKHR() failed to enumerate surface presentation modes. err: %s",
				lut::to_string(res).c_str());
		}

		std::unordered_set<VkPresentModeKHR> modesSet;
		for (auto mode : modes)
			modesSet.emplace(mode);

		return modesSet;
	}

	std::tuple<VkSwapchainKHR, VkFormat, VkExtent2D> create_swapchain(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface, VkDevice aDevice, GLFWwindow* aWindow, std::vector<std::uint32_t> const& aQueueFamilyIndices, VkSwapchainKHR aOldSwapchain)
	{
		auto const formats = get_surface_formats(aPhysicalDev, aSurface);
		assert(formats.empty() == false);
		auto const modes = get_present_modes(aPhysicalDev, aSurface);
		assert(modes.empty() == false);

		// Select the appropriate VkSurfaceFormatKHR
		VkSurfaceFormatKHR format = formats[0];
		for (const auto fmt : formats)
		{
			if (fmt.format == VK_FORMAT_R8G8B8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				format = fmt;
				break;
			}

			if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				format = fmt;
				break;
			}
		}

		// Select appropriate VkPresentModeKHR
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		if (modes.count(VK_PRESENT_MODE_FIFO_RELAXED_KHR) != 0)
		{
			presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		}

		// Get device surface capabilities;
		VkSurfaceCapabilitiesKHR caps{};
		if (const auto& res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aPhysicalDev, aSurface, &caps); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkGetPhysicalDeviceSurfaceCapabilitiesKHR() failed to return device surface capabilities. err: %s",
				lut::to_string(res).c_str());
		}

		// Pick image count
		std::uint32_t imageCount = 2;

		if (imageCount < caps.minImageCount + 1)
			imageCount = caps.minImageCount + 1;

		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
			imageCount = caps.maxImageCount;

		// Figure out swap extent
		VkExtent2D extent = caps.currentExtent;

		/* Not sure if I missed something in the exercises, but my
			application crashes when the window is minimised, so
			this is here to prevent that. */
		while (extent.width == 0 || extent.height == 0)
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(aWindow, &width, &height);
			glfwWaitEvents();

			extent.width = width;
			extent.height = height;
		}

		if (extent.width == std::numeric_limits<std::uint32_t>::max())
		{
			int width = 0, height = 0;
			glfwGetFramebufferSize(aWindow, &width, &height);

			const auto& minExt = caps.minImageExtent;
			const auto& maxExt = caps.maxImageExtent;

			extent.width = std::clamp(static_cast<uint32_t>(width), minExt.width, maxExt.width);
			extent.height = std::clamp(static_cast<uint32_t>(height), minExt.height, maxExt.height);
		}

		// Create swap chain
		VkSwapchainCreateInfoKHR scInfo{};
		scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		scInfo.surface = aSurface;
		scInfo.minImageCount = imageCount;
		scInfo.imageFormat = format.format;
		scInfo.imageColorSpace = format.colorSpace;
		scInfo.imageExtent = extent;
		scInfo.imageArrayLayers = 1;
		scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		scInfo.preTransform = caps.currentTransform;
		scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		scInfo.presentMode = presentMode;
		scInfo.clipped = VK_TRUE;
		scInfo.oldSwapchain = aOldSwapchain;

		if (aQueueFamilyIndices.size() <= 1)
		{
			scInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		else
		{
			scInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			scInfo.queueFamilyIndexCount = static_cast<uint32_t>(aQueueFamilyIndices.size());
			scInfo.pQueueFamilyIndices = aQueueFamilyIndices.data();
		}

		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		if (const auto& res = vkCreateSwapchainKHR(aDevice, &scInfo, nullptr, &swapChain); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkCreateSwapchainKHR() failed to create a swap chain. err: %s",
				lut::to_string(res).c_str());
		}

		return { swapChain, format.format, extent };
	}


	void get_swapchain_images(VkDevice aDevice, VkSwapchainKHR aSwapchain, std::vector<VkImage>& aImages)
	{
		assert(aImages.size() == 0);

		uint32_t imageCount = 0;
		if (const auto& res = vkGetSwapchainImagesKHR(aDevice, aSwapchain, &imageCount, nullptr))
		{
			throw lut::Error("VK: vkGetSwapchainImagesKHR() failed to return a swap chain image count. err: %s",
				lut::to_string(res).c_str());
		}

		aImages.resize(imageCount, 0);
		if (const auto& res = vkGetSwapchainImagesKHR(aDevice, aSwapchain, &imageCount, aImages.data()))
		{
			throw lut::Error("VK: vkGetSwapchainImagesKHR() failed to return swap chain images. err: %s",
				lut::to_string(res).c_str());
		}
	}

	void create_swapchain_image_views(VkDevice aDevice, VkFormat aSwapchainFormat,
		std::vector<VkImage> const& aImages, std::vector<VkImageView>& aViews)
	{
		assert(aViews.size() == 0);

		for (std::size_t i = 0; i < aImages.size(); i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.image = aImages[i];
			viewInfo.format = aSwapchainFormat;
			viewInfo.components = VkComponentMapping
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};
			viewInfo.subresourceRange = VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			};

			VkImageView view = VK_NULL_HANDLE;
			if (const auto& res = vkCreateImageView(aDevice, &viewInfo, nullptr, &view); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateImageView() failed to create a swap chain image view. err: %s",
					lut::to_string(res).c_str());
			}

			aViews.push_back(view);
		}

		assert(aViews.size() == aImages.size());
	}
}

namespace
{
	// Note: this finds *any* queue that supports the aQueueFlags. As such,
	//   find_queue_family( ..., VK_QUEUE_TRANSFER_BIT, ... );
	// might return a GRAPHICS queue family, since GRAPHICS queues typically
	// also set TRANSFER (and indeed most other operations; GRAPHICS queues are
	// required to support those operations regardless). If you wanted to find
	// a dedicated TRANSFER queue (e.g., such as those that exist on NVIDIA
	// GPUs), you would need to use different logic.
	std::optional<std::uint32_t> find_queue_family(VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface)
	{
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, nullptr);

		std::vector<VkQueueFamilyProperties> families(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(aPhysicalDev, &numQueues, families.data());

		for (uint32_t i = 0; i < numQueues; ++i)
		{
			const auto& family = families[i];

			if (aQueueFlags == (aQueueFlags & family.queueFlags))
			{
				if (VK_NULL_HANDLE == aSurface)
					return i;

				VkBool32 supported = VK_FALSE;
				const auto res = vkGetPhysicalDeviceSurfaceSupportKHR(aPhysicalDev, i, aSurface, &supported);

				if (VK_SUCCESS == res && supported)
					return i;
			}
		}

		return {};
	}

	VkDevice create_device(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t> const& aQueues, std::vector<char const*> const& aEnabledExtensions, const labutils::VulkanWindow::OptionalDeviceFeatures& aFeatures)
	{
		if (aQueues.empty())
			throw lut::Error("create_device(): no queues requested");

		float queuePriorities[1] = { 1.f };

		std::vector<VkDeviceQueueCreateInfo> queueInfos(aQueues.size());
		for (std::size_t i = 0; i < aQueues.size(); ++i)
		{
			auto& queueInfo = queueInfos[i];
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = aQueues[i];
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = queuePriorities;
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		if (aFeatures.samplerAnisotropy == true)
			deviceFeatures.samplerAnisotropy = VK_TRUE;
		/* gotta have the geom shader! */
		deviceFeatures.geometryShader = VK_TRUE; // (used for the mesh density visualisation)

		VkPhysicalDeviceVulkan12Features deviceExtraFeatures{};
		deviceExtraFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		deviceExtraFeatures.hostQueryReset = VK_TRUE;

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		deviceInfo.queueCreateInfoCount = std::uint32_t(queueInfos.size());
		deviceInfo.pQueueCreateInfos = queueInfos.data();

		deviceInfo.enabledExtensionCount = std::uint32_t(aEnabledExtensions.size());
		deviceInfo.ppEnabledExtensionNames = aEnabledExtensions.data();

		deviceInfo.pEnabledFeatures = &deviceFeatures;

		deviceInfo.pNext = &deviceExtraFeatures;

		VkDevice device = VK_NULL_HANDLE;
		if (auto const res = vkCreateDevice(aPhysicalDev, &deviceInfo, nullptr, &device); VK_SUCCESS != res)
		{
			throw lut::Error("Unable to create logical device\n"
				"vkCreateDevice() returned %s", lut::to_string(res).c_str()
			);
		}

		return device;
	}
}

namespace
{
	float score_device(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface)
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(aPhysicalDev, &props);

		// Only consider Vulkan 1.1 devices
		auto const major = VK_API_VERSION_MAJOR(props.apiVersion);
		auto const minor = VK_API_VERSION_MINOR(props.apiVersion);

		if (major < 1 || (major == 1 && minor < 2))
		{
			std::fprintf(stderr, "Info: Discarding device '%s': insufficient vulkan version\n", props.deviceName);
			return -1.f;
		}

		// Has swapchain extension support?
		const auto extensions = lut::detail::get_device_extensions(aPhysicalDev);
		if (extensions.count(VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
		{
			std::fprintf(stderr, "Info: Discarding device [%s] because it lacks support for extension [%s].\n",
				props.deviceName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);

			return -1.0f;
		}

		// Supports presenting to the surface?
		if (find_queue_family(aPhysicalDev, 0, aSurface).has_value() == false)
		{
			std::fprintf(stderr, "Info: Discarding device [%s] because it cannot present to the given surface.\n",
				props.deviceName);

			return -1.0f;
		}

		// Has a graphics family queue?
		if (find_queue_family(aPhysicalDev, VK_QUEUE_GRAPHICS_BIT).has_value() == false)
		{
			std::fprintf(stderr, "Info: Discarding device [%s] because it lacks a graphics queue.\n",
				props.deviceName);

			return -1.0f;
		}

		// Discrete GPU > Integrated GPU > others
		float score = 0.f;

		if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == props.deviceType)
			score += 500.f;
		else if (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == props.deviceType)
			score += 100.f;

		return score;
	}

	VkPhysicalDevice select_device(VkInstance aInstance, VkSurfaceKHR aSurface)
	{
		std::uint32_t numDevices = 0;
		if (auto const res = vkEnumeratePhysicalDevices(aInstance, &numDevices, nullptr); VK_SUCCESS != res)
		{
			throw lut::Error("Unable to get physical device count\n"
				"vkEnumeratePhysicalDevices() returned %s", lut::to_string(res).c_str()
			);
		}

		std::vector<VkPhysicalDevice> devices(numDevices, VK_NULL_HANDLE);
		if (auto const res = vkEnumeratePhysicalDevices(aInstance, &numDevices, devices.data()); VK_SUCCESS != res)
		{
			throw lut::Error("Unable to get physical device list\n"
				"vkEnumeratePhysicalDevices() returned %s", lut::to_string(res).c_str()
			);
		}

		float bestScore = -1.f;
		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

		for (auto const device : devices)
		{
			auto const score = score_device(device, aSurface);
			if (score > bestScore)
			{
				bestScore = score;
				bestDevice = device;
			}
		}

		return bestDevice;
	}
}

