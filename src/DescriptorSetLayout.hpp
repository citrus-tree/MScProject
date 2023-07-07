#pragma once
#pragma once

/* renderer */ 
#include "DescriptorSetLayoutFeatures.hpp"

/* labutils */
#include "vkobject.hpp"

namespace Renderer
{
	class Environment;
}

namespace Renderer
{
	namespace lut = labutils;

	class DescriptorSetLayout
	{
	public:
		/* constructors, etc. */

		DescriptorSetLayout() = delete;
		DescriptorSetLayout(const Environment* environment, const DescriptorSetLayoutFeatures& init_data);
		DescriptorSetLayout(const Environment* environment, const ShaderStages stages, DescriptorSetType type, uint32_t binding = 0);
		~DescriptorSetLayout();

		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	private:
		/* private member variables */

		lut::DescriptorSetLayout _layout;

		/* private member functions */

		void createLayout(const Environment* environment, const DescriptorSetLayoutFeatures& init_data);

	public:

		/* getters */

		const VkDescriptorSetLayout& operator*() const;
	};
}
