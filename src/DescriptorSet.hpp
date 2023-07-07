#pragma once

/* renderer */ 
#include "DescriptorSetFeatures.hpp"

/* labutils */
#include "vkobject.hpp"

namespace Renderer
{
	class DescriptorSetLayout;
	class Environment;
	class Pipeline;
}

namespace Renderer
{
	class DescriptorSet
	{
		public:
			/* constructors, etc. */

			DescriptorSet() = delete;
			DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout);
			DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout, const VkBuffer& uniform_buffer);
			DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout, const VkImageView& image_view, const VkSampler& sampler);
			DescriptorSet(const Environment* environment, const DescriptorSetLayout* layout, uint32_t descriptorCount, DescriptorSetFeatures* pDescriptorsData);
			~DescriptorSet();

			DescriptorSet(const DescriptorSet&) = delete;
			DescriptorSet& operator=(const DescriptorSet&) = delete;

		private:
			/* private enum types */
			enum class State
			{
				NOT_READY,
				READY
			};

			/* private member variables */

			VkDescriptorSet _set{};
			State _state = State::NOT_READY;

			/* private member functions */

			void allocateDescriptorSet(const Environment* environment, const Renderer::DescriptorSetLayout* layout);

		public:
			/* public member functions */

			void UpdateDescriptorSet(const Environment* environment, uint32_t descriptorCount, DescriptorSetFeatures* pDescriptorsData);

			void CmdBind(Environment* environment, Pipeline* pipeline, uint32_t set_index);

			/* getters */

			const VkDescriptorSet& operator*() const;
	};
}
