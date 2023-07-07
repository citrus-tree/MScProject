#include "vkutil.hpp"

#include <algorithm>
#include <vector>

#include <cstdio>
#include <cassert>

/* labutils */
#include "error.hpp"
#include "to_string.hpp"
#include "vulkan_window.hpp" // <- class VulkanWindow

namespace labutils
{
	ShaderModule load_shader_module(VulkanContext const& aContext, char const* aSpirvPath)
	{
		if (std::FILE* fin = std::fopen(aSpirvPath, "rb"))
		{
			std::fseek(fin, 0, SEEK_END);
			auto const bytes = std::size_t(std::ftell(fin));
			std::fseek(fin, 0, SEEK_SET);

			assert(bytes % 4 == 0);
			auto const words = bytes / 4;

			std::vector<std::uint32_t> code(words);

			std::size_t offset = 0;
			while (offset != words)
			{
				auto const read = std::fread(code.data() + offset, sizeof(std::uint32_t), words - offset, fin);

				if (read == 0)
				{
					std::fclose(fin);
					throw Error("Error reading ’%s’: ferror = %d, feof = %d", aSpirvPath, std::ferror(fin), std::feof(fin));
				}

				offset += read;
			}

			std::fclose(fin);

			VkShaderModuleCreateInfo moduleInfo{};
			moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleInfo.codeSize = bytes;
			moduleInfo.pCode = code.data();

			VkShaderModule shaderMod = VK_NULL_HANDLE;
			if (auto const res = vkCreateShaderModule(aContext.device, &moduleInfo, nullptr, &shaderMod); res != VK_SUCCESS)
			{
				throw Error("VK: vkCreateShaderModule() failed. err: %s",
					aSpirvPath, to_string(res).c_str());
			}

			return ShaderModule(aContext.device, shaderMod);
		}

		throw Error("Cannot open '%s' for reading", aSpirvPath);
	}


	CommandPool create_command_pool(VulkanContext const& aContext, VkCommandPoolCreateFlags aFlags)
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = aContext.graphicsFamilyIndex;
		poolInfo.flags = aFlags;

		VkCommandPool pool = VK_NULL_HANDLE;
		if (const auto res = vkCreateCommandPool(aContext.device, &poolInfo, nullptr, &pool); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateCommandPool() failed. err: %s",
				to_string(res).c_str());
		}

		return CommandPool(aContext.device, pool);
	}

	VkCommandBuffer alloc_command_buffer(VulkanContext const& aContext, VkCommandPool aCmdPool)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = aCmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cBuffer = VK_NULL_HANDLE;
		if (const auto res = vkAllocateCommandBuffers(aContext.device, &allocInfo, &cBuffer); res != VK_SUCCESS)
		{
			throw Error("VK: vkAllocateCommandBuffers() failed. err: %s",
				to_string(res).c_str());
		}

		return cBuffer;
	}


	Fence create_fence(VulkanContext const& aContext, VkFenceCreateFlags aFlags)
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = aFlags;

		VkFence fence = VK_NULL_HANDLE;
		if (const auto res = vkCreateFence(aContext.device, &fenceInfo, nullptr, &fence); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateFence() failed. err: %s",
				to_string(res).c_str());
		}

		return Fence(aContext.device, fence);
	}

	Semaphore create_semaphore(VulkanContext const& aContext)
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkSemaphore semaphore = VK_NULL_HANDLE;
		if (const auto& res = vkCreateSemaphore(aContext.device, &semaphoreInfo, nullptr, &semaphore); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateSemaphore() failed. err: %s",
				to_string(res).c_str());
		}

		return Semaphore(aContext.device, semaphore);
	}

	DescriptorPool create_descriptor_pool(const VulkanContext& aContext, std::uint32_t aMaxDescriptors, std::uint32_t aMaxSets)
	{
		VkDescriptorPoolSize const pools[2] =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, aMaxDescriptors },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, aMaxDescriptors }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = aMaxSets;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = pools;

		VkDescriptorPool pool = VK_NULL_HANDLE;
		if (const auto& res = vkCreateDescriptorPool(aContext.device, &poolInfo, nullptr, &pool); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateDescriptorPool() failed. err: %s",
				to_string(res).c_str());
		}

		return DescriptorPool(aContext.device, pool);
	}
	VkDescriptorSet alloc_desc_set(const VulkanContext& aContext, VkDescriptorPool aDescPool, VkDescriptorSetLayout aDescSetLayout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = aDescPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &aDescSetLayout;

		VkDescriptorSet set = VK_NULL_HANDLE;
		if (const auto& res = vkAllocateDescriptorSets(aContext.device, &allocInfo, &set); res != VK_SUCCESS)
		{
			throw Error("VK: vkAllocateDescriptorSets() failed. err: %s",
				to_string(res).c_str());
		}

		return set;
	}

	ImageView create_image_view_texture2d(VulkanContext const& aContext, VkImage aImage, VkFormat aFormat)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = aImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = aFormat;
		viewInfo.components = VkComponentMapping{};
		viewInfo.subresourceRange = VkImageSubresourceRange
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, VK_REMAINING_MIP_LEVELS,
			0, 1
		};

		VkImageView view = VK_NULL_HANDLE;
		if (auto const res = vkCreateImageView(aContext.device, &viewInfo, nullptr, &view); VK_SUCCESS != res)
		{
			throw Error("VK: vkCreateImageView() failed to create a texture image view. err: %s",
				to_string(res).c_str());
		}

		return ImageView(aContext.device, view);
	}

	Sampler create_default_sampler(VulkanWindow const* aWindow, bool anisotropicFiltering, float anisotropy)
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.f;

		/* optional features */
		if (aWindow->features.samplerAnisotropy == true &&
			anisotropicFiltering == true)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			if (anisotropy > aWindow->features.maxSamplerAnisotropy || anisotropy < 1.0f)
				printf("The requested sampler anisotropy value of [%.2f] is out of the range "
					"[%.2f, %.2f] capable by the current device.",
					anisotropy, 1.0f, aWindow->features.maxSamplerAnisotropy);
			samplerInfo.maxAnisotropy = std::clamp(anisotropy, 1.0f, aWindow->features.maxSamplerAnisotropy);
		}

		VkSampler sampler = VK_NULL_HANDLE;
		if (const auto& res = vkCreateSampler(aWindow->device, &samplerInfo, nullptr, &sampler); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateSampler() failed. err: %s",
				to_string(res).c_str());
		}

		return Sampler(aWindow->device, sampler);
	}

	void buffer_barrier(
		VkCommandBuffer aCommandBuffer, VkBuffer aBuffer,
		VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask,
		VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask,
		VkDeviceSize aSize, VkDeviceSize aOffset,
		uint32_t aSrcQueueFamilyIndex, uint32_t aDstQueueFamilyIndex)
	{
		VkBufferMemoryBarrier bbarrier{};
		bbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bbarrier.srcAccessMask = aSrcAccessMask;
		bbarrier.dstAccessMask = aDstAccessMask;
		bbarrier.buffer = aBuffer;
		bbarrier.size = aSize;
		bbarrier.offset = aOffset;
		bbarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
		bbarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;

		vkCmdPipelineBarrier(aCommandBuffer, aSrcStageMask, aDstStageMask, 0, 0, nullptr, 1, &bbarrier, 0, nullptr);
	}

	void image_barrier(VkCommandBuffer aCommandBuffer, VkImage aImage,
		VkAccessFlags aSrcAccessMask, VkAccessFlags aDstAccessMask,
		VkImageLayout aSrcLayout, VkImageLayout aDstLayout,
		VkPipelineStageFlags aSrcStageMask, VkPipelineStageFlags aDstStageMask,
		VkImageSubresourceRange aSubRange,
		std::uint32_t aSrcQueueFamilyIndex, std::uint32_t aDstQueueFamilyIndex)
	{
		VkImageMemoryBarrier ibarrier{};
		ibarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ibarrier.image = aImage;
		ibarrier.srcAccessMask = aSrcAccessMask;
		ibarrier.dstAccessMask = aDstAccessMask;
		ibarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
		ibarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;
		ibarrier.oldLayout = aSrcLayout;
		ibarrier.newLayout = aDstLayout;
		ibarrier.subresourceRange = aSubRange;

		vkCmdPipelineBarrier(aCommandBuffer, aSrcStageMask, aDstStageMask, 0, 0, nullptr, 0, nullptr, 1, &ibarrier);
	}
}
