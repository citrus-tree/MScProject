#pragma once

/* renderer */ 
#include "RenderPassFeatures.hpp"

/* labutils */
#include "vkobject.hpp"

namespace labutils
{
	class VulkanWindow;
}

namespace Renderer
{
	namespace lut = labutils;

	class RenderPass
	{
		public:
			/* constructors, etc. */

			RenderPass() = delete;
			RenderPass(const labutils::VulkanWindow* window, const RenderPassFeatures& init_data = RenderPass_Default);
			~RenderPass();

			RenderPass(const RenderPass&) = delete;
			RenderPass& operator=(const RenderPass&) = delete;

		private:
			/* private member variables */

			lut::RenderPass _renderPass{};
			RenderPassFeatures _initData{};

			/* private member functions */

			void createRenderPass(const labutils::VulkanWindow* window);

		public:
			/* public member functions */

			void Repair(const labutils::VulkanWindow* window);

			/* getters */

			const VkRenderPass& operator*() const;
			const RenderPassFeatures& Features() const;
	};
}
