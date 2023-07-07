#pragma once

/* volk */
#include <volk/volk.h>

namespace Renderer
{
	/* Descriptor Set Types and Data */
	
	struct DescriptorSetFeatures
	{
		/* shared data */
		uint32_t binding{};

		/* data for uniform buffers*/
		VkBuffer u_Buffer{};
		
		/* data for texture samplers */
		VkImageView s_View{};
		VkSampler s_Sampler{};
	};
}
