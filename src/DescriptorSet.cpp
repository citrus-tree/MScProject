#include "DescriptorSet.hpp"

/* renderer */ 
#include "DescriptorSetLayout.hpp" // <- class DescriptorSetLayout
#include "Environment.hpp" // <- class Environment
#include "Pipeline.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

namespace Renderer
{
	/* constructors, etc. */

	DescriptorSet::DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout)
		: _state(State::NOT_READY) // <- not necessary, but feels like good practice
	{
		allocateDescriptorSet(environment, layout);
	}

	DescriptorSet::DescriptorSet(const Environment* environment, const Renderer::DescriptorSetLayout* layout, const VkBuffer& uniform_buffer)
	{
		DescriptorSetFeatures initData{};
		initData.u_Buffer = uniform_buffer;

		allocateDescriptorSet(environment, layout);
		UpdateDescriptorSet(environment, 1, &initData);
	}

	DescriptorSet::DescriptorSet(const Environment* environment, const Renderer::DescriptorSetLayout* layout, const VkImageView& image_view, const VkSampler& sampler)
	{
		DescriptorSetFeatures initData{};
		initData.s_View = image_view;
		initData.s_Sampler = sampler;

		allocateDescriptorSet(environment, layout);
		UpdateDescriptorSet(environment, 1, &initData);
	}

	DescriptorSet::DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout, uint32_t descriptorCount, DescriptorSetFeatures* pDescriptorsData)
	{
		allocateDescriptorSet(environment, layout);
		UpdateDescriptorSet(environment, descriptorCount, pDescriptorsData);
	}

	DescriptorSet::~DescriptorSet()
	{
		/* This space intentionally left blank. */
	}

	/* private member functions */

	void DescriptorSet::allocateDescriptorSet(const Environment* environment, const Renderer::DescriptorSetLayout* layout)
	{
		using namespace lut;

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = *environment->DescPool();
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &**layout;

		if (const auto& res = vkAllocateDescriptorSets(environment->Window().device, &allocInfo, &_set); res != VK_SUCCESS)
		{
			throw Error("VK: vkAllocateDescriptorSets() failed. err: %s",
				to_string(res).c_str());
		}
	}

	/* public member functions */

	void DescriptorSet::UpdateDescriptorSet(const Environment* environment, uint32_t descriptorCount, DescriptorSetFeatures* pDescriptorsData)
	{
		std::vector<VkWriteDescriptorSet> descWrites(0);
		descWrites.resize(descriptorCount, {});

		std::vector<VkDescriptorBufferInfo> bufferInfo{};
		uint32_t bufferCount = 0;
		std::vector<VkDescriptorImageInfo> imageInfo{};
		uint32_t imageCount = 0;

		/* count descriptor count of each type */
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			/* The dataset cannot be ambiguous or lacking data */
			assert(pDescriptorsData[i].u_Buffer != VK_NULL_HANDLE ||
				(pDescriptorsData[i].s_View != VK_NULL_HANDLE && pDescriptorsData[i].s_Sampler != VK_NULL_HANDLE));

			if (pDescriptorsData[i].u_Buffer != VK_NULL_HANDLE)
			{
				bufferCount++;
			}
			else
			{
				imageCount++;
			}
		}

		bufferInfo.resize(bufferCount, {});
		bufferCount = 0;
		imageInfo.resize(imageCount, {});
		imageCount = 0;

		/* write data to VkWriteDescriptorSet structs */
		for (uint32_t i = 0; i < descriptorCount; i++)
		{
			descWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descWrites[i].dstSet = _set;
			descWrites[i].dstBinding = pDescriptorsData[i].binding;
			descWrites[i].descriptorCount = 1;

			/* The dataset cannot be ambiguous or lacking data */
			assert(pDescriptorsData[i].u_Buffer != VK_NULL_HANDLE ||
				(pDescriptorsData[i].s_View != VK_NULL_HANDLE && pDescriptorsData[i].s_Sampler != VK_NULL_HANDLE));

			if (pDescriptorsData[i].u_Buffer != VK_NULL_HANDLE)
			{
				uint32_t index = bufferCount++;
				bufferInfo[index].buffer = pDescriptorsData[i].u_Buffer;
				bufferInfo[index].offset = 0;
				bufferInfo[index].range = VK_WHOLE_SIZE;

				descWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descWrites[i].pBufferInfo = &bufferInfo[index];
			}
			else
			{
				uint32_t index = imageCount++;
				imageInfo[index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo[index].imageView = pDescriptorsData[i].s_View;
				imageInfo[index].sampler = pDescriptorsData[i].s_Sampler;

				descWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descWrites[i].pImageInfo = &imageInfo[index];
			}
		}

		vkUpdateDescriptorSets(environment->Window().device, static_cast<uint32_t>(descWrites.size()), descWrites.data(), 0, nullptr);

		/* This descriptor set now has data, so it
			is ready to be used during rendering. */
		_state = State::READY;
	}

	void DescriptorSet::CmdBind(Environment* environment, Pipeline* pipeline, uint32_t set_index)
	{
		assert(_state == State::READY);

		vkCmdBindDescriptorSets(*environment->CurrentCmdBuffer(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			*pipeline->GetPipelineLayout(),
			set_index, 1, &_set, 0, nullptr);
	}

	/* getters */

	const VkDescriptorSet& DescriptorSet::operator*() const
	{
		return _set;
	}
}
