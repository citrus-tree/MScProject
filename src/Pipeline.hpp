#pragma once

/* c */
#include <cstdint>

/* c++ */
#include <vector>

/* renderer */ 
#include "PipelineFeatures.hpp"

/* labutils */
#include "vkobject.hpp"

namespace Renderer
{
	class RenderPass;
	class Environment;
}

namespace Renderer
{
	namespace lut = labutils;

	class Pipeline
	{

		public:
			/* constructors, etc. */

			Pipeline() = delete;
			Pipeline(const Environment* environment,
				const PipelineFeatures& init_data,
				const RenderPass* render_pass,
				const std::vector<const VkDescriptorSetLayout*>& descSets = {});
			~Pipeline();

			Pipeline(const Pipeline&) = delete;
			Pipeline& operator=(const Pipeline&) = delete;

		private:
			/* private member variables */

			lut::PipelineLayout _layout{};
			lut::Pipeline _pipeline{};
			PipelineFeatures _initData{};
			VkExtent2D _currentExtent{};

			const Renderer::RenderPass* _epRenderPass = nullptr;

			/* private member functions */

			void createPipelineLayout(const Environment* environment,
				const std::vector<const VkDescriptorSetLayout*>& descSets);
			void createPipeline(const Environment* environment);

		public:
			/* public functions */

			bool IsCompatible(const Environment* environment) const;
			void Repair(const Environment* environment, const Renderer::RenderPass* render_pass = nullptr);

			void CmdBind(Environment* environment);

			/* getters */

			const lut::PipelineLayout& GetPipelineLayout() const;
			const lut::Pipeline& GetPipeline() const;
	};
}