#include "DescriptorSetLayout.hpp"

/* renderer */ 
#include "Environment.hpp" // <- class Environment
#include "Pipeline.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

namespace Renderer
{
	/* constructors, etc. */

	DescriptorSetLayout::DescriptorSetLayout(const Environment* environment, const DescriptorSetLayoutFeatures& init_data)
	{
		createLayout(environment, init_data);
	}

	DescriptorSetLayout::DescriptorSetLayout(const Environment* environment, const ShaderStages stages, DescriptorSetType type, uint32_t bindings)
	{
		DescriptorSetLayoutFeatures initData{};
		initData.stages = stages;
		initData.bindingCount = 1;

		DescriptorSetType descType = type;
		initData.pBindingTypes = &descType;

		createLayout(environment, initData);
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		/* This space intentionally left blank. */
	}

	/* private member functions*/

	void DescriptorSetLayout::createLayout(const Environment* environment, const DescriptorSetLayoutFeatures& init_data)
	{
		assert(init_data.bindingCount > 0);
		assert(init_data.pBindingTypes != nullptr);

		using namespace lut;

		/* set up a collection of binding info structs */
		std::vector<VkDescriptorSetLayoutBinding> bindings(0);
		bindings.resize(init_data.bindingCount, {});

		/* get the correct shader stage flag */
		VkShaderStageFlags stages = 0;

		if (init_data.stages.vertex)
			stages |= VK_SHADER_STAGE_VERTEX_BIT;

		if (init_data.stages.fragment)
			stages |= VK_SHADER_STAGE_FRAGMENT_BIT;

		if (init_data.stages.geometry)
			stages |= VK_SHADER_STAGE_GEOMETRY_BIT;

		/* set up each of the layout bindings */
		for (uint32_t i = 0; i < init_data.bindingCount; i++)
		{
			bindings[i].binding = i;
			bindings[i].descriptorCount = 1;

			switch (init_data.pBindingTypes[i])
			{
				case (DescriptorSetType::SAMPLER):
					bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					break;

				case (DescriptorSetType::UNIFORM_BUFFER):
					bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

					break;
			}
			bindings[i].stageFlags = stages;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
		if (const auto& res = vkCreateDescriptorSetLayout(environment->Window().device, &layoutInfo, nullptr, &descLayout); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkCreateDescriptorSetLayout() failed. err: %s",
				lut::to_string(res).c_str());
		}

		_layout = lut::DescriptorSetLayout(environment->Window().device, descLayout);
	}

	/* getters */

	const VkDescriptorSetLayout& DescriptorSetLayout::operator*() const
	{
		return *_layout;
	}
}
