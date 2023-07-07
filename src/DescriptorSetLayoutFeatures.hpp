#pragma once

/* c++ */
#include <vector>

/* volk */
#include <volk/volk.h>

namespace Renderer
{
	/* Descriptor Set Types and Data */

	enum class DescriptorSetType
	{
		UNIFORM_BUFFER = 0,
		SAMPLER
	};

	struct ShaderStages
	{
		bool vertex = false;
		bool fragment = false;
		bool geometry = false;
	};

	namespace ShaderStageConstants
	{
		const ShaderStages VERTEX_STAGE
		{
			true,
			false,
			false
		};

		const ShaderStages FRAGMENT_STAGE
		{
			false,
			true,
			false
		};

		const ShaderStages GEOMETRY_STAGE
		{
			false,
			false,
			true
		};
	}

	struct DescriptorSetLayoutFeatures
	{
		ShaderStages stages{ false, false, false };
		uint32_t bindingCount = 0;
		DescriptorSetType* pBindingTypes = nullptr;
	};
}
