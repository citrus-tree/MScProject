#include "CreationUtilities.hpp"

/* c */
#include <algorithm>
#include <cassert>

namespace Renderer
{
	labutils::DescriptorPool CreateDescriptorPool(const VkDevice& logicalDevice, std::uint32_t maxDescriptors, std::uint32_t maxSets)
	{
		using namespace labutils;

		VkDescriptorPoolSize const pools[2] =
		{
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxDescriptors },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxDescriptors }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = maxSets;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = pools;

		VkDescriptorPool pool = VK_NULL_HANDLE;
		if (const auto& res = vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &pool); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateDescriptorPool() failed. err: %s",
				to_string(res).c_str());
		}

		return DescriptorPool(logicalDevice, pool);
	}

	labutils::Sampler CreateDefaultSampler(const labutils::VulkanWindow& window,
		VkFilter minFilter, VkFilter magFilter,
		bool anisotropicFiltering, float anisotropy)
	{
		using namespace labutils;

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = magFilter;
		samplerInfo.minFilter = minFilter;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.f;

		/* optional features */
		if (window.features.samplerAnisotropy == true &&
			anisotropicFiltering == true)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			if (anisotropy > window.features.maxSamplerAnisotropy || anisotropy < 1.0f)
				printf("The requested sampler anisotropy value of [%.2f] is out of the range "
					"[%.2f, %.2f] capable by the current device.",
					anisotropy, 1.0f, window.features.maxSamplerAnisotropy);
			samplerInfo.maxAnisotropy = std::clamp(anisotropy, 1.0f, window.features.maxSamplerAnisotropy);
		}

		VkSampler sampler = VK_NULL_HANDLE;
		if (const auto& res = vkCreateSampler(window.device, &samplerInfo, nullptr, &sampler); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateSampler() failed. err: %s",
				to_string(res).c_str());
		}

		return Sampler(window.device, sampler);
	}
	labutils::Sampler CreateDefaultShadowSampler(const labutils::VulkanWindow& window)
	{
		using namespace labutils;

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

		/* Extra bits for shadow mapping */
			/* Anything outside the shadowmap is considered in shadow. 
				I'm only doing this since I'm only using spotlights;
				I would usually do the opposite of this for directional shadows.*/
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			/* Compare ops */
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS;

		VkSampler sampler = VK_NULL_HANDLE;
		if (const auto& res = vkCreateSampler(window.device, &samplerInfo, nullptr, &sampler); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateSampler() failed. err: %s",
				to_string(res).c_str());
		}

		return Sampler(window.device, sampler);
	}
}