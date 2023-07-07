#pragma once

#include <volk/volk.h>

#include "vkobject.hpp"
#include "vulkan_context.hpp"

namespace labutils
{
	class VulkanWindow;
}

namespace labutils
{
	ShaderModule load_shader_module(VulkanContext const&, char const* aSpirvPath);

	CommandPool create_command_pool(VulkanContext const&, VkCommandPoolCreateFlags = 0);
	VkCommandBuffer alloc_command_buffer(VulkanContext const&, VkCommandPool);

	Fence create_fence(VulkanContext const&, VkFenceCreateFlags = 0);
	Semaphore create_semaphore(VulkanContext const&);

	DescriptorPool create_descriptor_pool(const VulkanContext&, std::uint32_t aMaxDescriptors = 2048, std::uint32_t aMaxSets = 1024);
	VkDescriptorSet alloc_desc_set(const VulkanContext&, VkDescriptorPool, VkDescriptorSetLayout);

	ImageView create_image_view_texture2d(VulkanContext const&, VkImage, VkFormat);

	Sampler create_default_sampler(VulkanWindow const*, bool anisotropicFiltering = true, float anisotropy = 16.0f);

	void buffer_barrier(
		VkCommandBuffer,
		VkBuffer,
		VkAccessFlags aSrcAccessMask,
		VkAccessFlags aDstAccessMask,
		VkPipelineStageFlags aSrcStageMask,
		VkPipelineStageFlags aDstStageMask,
		VkDeviceSize aSize = VK_WHOLE_SIZE,
		VkDeviceSize aOffset = 0,
		uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

	void image_barrier(
		VkCommandBuffer,
		VkImage,
		VkAccessFlags aSrcAccessMask,
		VkAccessFlags aDstAccessMask,
		VkImageLayout aSrcLayout,
		VkImageLayout aDstLayout,
		VkPipelineStageFlags aSrcStageMask,
		VkPipelineStageFlags aDstStageMask,
		VkImageSubresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1 },
		std::uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		std::uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
}
