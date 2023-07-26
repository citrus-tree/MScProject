#pragma once

/* renderer */
#include "Environment.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vulkan_window.hpp"

/* volk */
#include <volk/volk.h>

namespace Renderer
{
	labutils::DescriptorPool CreateDescriptorPool(const VkDevice& logicalDevice, std::uint32_t maxDescriptors = 2048, std::uint32_t maxSets = 1024);

	labutils::Sampler CreateDefaultSampler(const labutils::VulkanWindow& window,
		VkFilter minFilter = VK_FILTER_LINEAR, VkFilter magFilter = VK_FILTER_LINEAR,
		bool anisotropicFiltering = true, float anisotropy = 16.0f);
	labutils::Sampler CreateDefaultShadowSampler(const labutils::VulkanWindow& window);
}