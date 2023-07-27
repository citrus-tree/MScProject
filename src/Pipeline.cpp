#include "Pipeline.hpp"

/* renderer */ 
#include "RenderPass.hpp" // <- class RenderPass
#include "Environment.hpp" // <- class Environment
#include "Constants.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vulkan_window.hpp"

namespace Renderer
{
	Pipeline::Pipeline(const Environment* environment,
		const PipelineFeatures& init_data,
		const RenderPass* render_pass,
		const std::vector<const VkDescriptorSetLayout*>& descSets)
	{
		_epRenderPass = render_pass;

		_initData = init_data;

		createPipelineLayout(environment, descSets);
		createPipeline(environment);
	}

	Pipeline::~Pipeline()
	{
		/* Everything is a member variable, so I don't *think* anything needs to be here... */
	}


	/* private member functions */

	void Pipeline::createPipelineLayout(const Environment* environment,
		const std::vector<const VkDescriptorSetLayout*>& descSets)
	{
		using namespace labutils;

		std::vector<VkDescriptorSetLayout> vkSetLayouts(0, VkDescriptorSetLayout{});
		for (uint32_t i = 0; i < descSets.size(); i++)
			vkSetLayouts.push_back(*descSets[i]);

		VkPipelineLayoutCreateInfo pLayoutInfo{};
		pLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pLayoutInfo.setLayoutCount = static_cast<uint32_t>(descSets.size());
		pLayoutInfo.pSetLayouts = vkSetLayouts.data();
		pLayoutInfo.pushConstantRangeCount = 0;
		pLayoutInfo.pPushConstantRanges = nullptr;

		VkPipelineLayout layout = VK_NULL_HANDLE;
		if (auto const res = vkCreatePipelineLayout(environment->Window().device, &pLayoutInfo, nullptr, &layout); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreatePipelineLayout() failed. err: %s",
				to_string(res).c_str());
			layout = VK_NULL_HANDLE;
		}

		_layout = PipelineLayout(environment->Window().device, layout);
	}

	void Pipeline::createPipeline(const Environment* environment)
	{
		using namespace labutils;
		
		_currentExtent = environment->Window().swapchainExtent;

		ShaderModule vert{};
		ShaderModule frag{};
		std::vector<VkPipelineShaderStageCreateInfo> stagesInfo{};

		if (_initData.specialMode == SpecialMode::NONE)
		{
			switch (_initData.fragmentMode)
			{
				case (FragmentMode::SIMPLE):
				default:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "default.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "default.frag.spv");
					stagesInfo.resize(2);
					break;
			}
		}
		else if (_initData.specialMode != SpecialMode::NONE)
		{

			switch (_initData.specialMode)
			{
				case SpecialMode::SCREEN_QUAD_PRESENT:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "fullscreen.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "present_quad.frag.spv");
					break;

				case SpecialMode::SHADOW_MAP:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "shadowmap.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "shadowmap.frag.spv");
					break;

				case SpecialMode::TS_GEOMETRY:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "default.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "TS_geometryPass.frag.spv");
					break;
					
				case SpecialMode::TS_COLOURED_SHADOW_MAP:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "TS_colouredShadowPass.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "TS_colouredShadowPass.frag.spv");
					break;

				case SpecialMode::SSM_STOCHASTIC_SHADOW_MAP:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "SSM_shadowPass.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "SSM_shadowPass.frag.spv");
					break;

				case SpecialMode::SSM_DEFAULT_BIG_PCF:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "default.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "SSM_defaultPCF.frag.spv");
					break;

				case SpecialMode::CSSM_COLORED_STOCHASTIC_SHADOW_MAP:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "SSM_shadowPass.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "CSSM_shadowPass.frag.spv");
					break;

				case SpecialMode::CSSM_DEFAULT:
				default:
					vert = load_shader_module(environment->Window(), "../res/shaders/" "default.vert.spv");
					frag = load_shader_module(environment->Window(), "../res/shaders/" "CSSM_defaultPCF.frag.spv");
					break;
			}

			stagesInfo.resize(2);
		}


		/* Vertex Shader */
		stagesInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stagesInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stagesInfo[0].module = *vert;
		stagesInfo[0].pName = "main";

		/* Fragment Shader */
		stagesInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stagesInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stagesInfo[1].module = *frag;
		stagesInfo[1].pName = "main";

		/* Vertex input info */
		std::vector<VkVertexInputBindingDescription> vertexInputs{};
		std::vector <VkVertexInputAttributeDescription> vertexAttributes{};

		/* Input state info */
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		if (_initData.specialMode == SpecialMode::NONE || _initData.specialMode != SpecialMode::SCREEN_QUAD_PRESENT)
		{
			/* Vertex input info continued */

				/* Position input info */
			vertexInputs.push_back({});
			vertexInputs[0].binding = 0;
			vertexInputs[0].stride = sizeof(float) * 3;
			vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			vertexAttributes.push_back({});
			vertexAttributes[0].binding = 0;
			vertexAttributes[0].location = 0;
			vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributes[0].offset = 0;

				/* UV input info */
			vertexInputs.push_back({});
			vertexInputs[1].binding = 1;
			vertexInputs[1].stride = sizeof(float) * 2;
			vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			vertexAttributes.push_back({});
			vertexAttributes[1].binding = 1;
			vertexAttributes[1].location = 1;
			vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			vertexAttributes[1].offset = 0;

			if (_initData.specialMode == SpecialMode::NONE ||
				_initData.specialMode == SpecialMode::TS_GEOMETRY ||
				_initData.specialMode == SpecialMode::SSM_DEFAULT_BIG_PCF ||
				_initData.specialMode == SpecialMode::CSSM_DEFAULT)
			{
					/* Normals input info */
				vertexInputs.push_back({});
				vertexInputs[2].binding = 2;
				vertexInputs[2].stride = sizeof(float) * 3;
				vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

				vertexAttributes.push_back({});
				vertexAttributes[2].binding = 2;
				vertexAttributes[2].location = 2;
				vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
				vertexAttributes[2].offset = 0;
			}

			/* Input state info continued */
			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputs.size());
			vertexInputInfo.pVertexBindingDescriptions = vertexInputs.data();
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
			vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();
		}

		/* Input Assembly Info */
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		/* Viewport Info */
		VkViewport viewport{};
		VkRect2D scissorRect{};

		viewport.x = 0.0f;
		viewport.y = 0.0f;
		scissorRect.offset = VkOffset2D{ 0, 0 };
		if (_initData.specialMode == SpecialMode::SHADOW_MAP ||
			_initData.specialMode == SpecialMode::TS_COLOURED_SHADOW_MAP ||
			_initData.specialMode == SpecialMode::SSM_STOCHASTIC_SHADOW_MAP ||
			_initData.specialMode == SpecialMode::CSSM_COLORED_STOCHASTIC_SHADOW_MAP)
		{
			viewport.width = SHADOW_MAP_RESOLUTION_F;
			viewport.height = SHADOW_MAP_RESOLUTION_F;
			scissorRect.extent = VkExtent2D{ SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION };
			_currentExtent = VkExtent2D{ SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION };
		}
		else
		{
			viewport.width = static_cast<float>(_currentExtent.width);
			viewport.height = static_cast<float>(_currentExtent.height);
			scissorRect.extent = VkExtent2D{ _currentExtent.width, _currentExtent.height };
		}
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;


		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissorRect;

		/* Depth Stencil Settings */
		VkPipelineDepthStencilStateCreateInfo depthInfo{};
		depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		if (_epRenderPass->Features().depthTest == DepthTest::ENABLED && _initData.depthTest == DepthTest::ENABLED)
		{
			depthInfo.depthWriteEnable = (_initData.depthWrite == DepthWrite::ENABLED) ? VK_TRUE : VK_FALSE;
			depthInfo.depthTestEnable = VK_TRUE;

			switch(_initData.depthOp)
			{
				case DepthOp::LEQUAL:
					depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					break;

				case DepthOp::GEQUAL:
					depthInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
					break;

				case DepthOp::LESS:
					depthInfo.depthCompareOp = VK_COMPARE_OP_LESS;
					break;

				case DepthOp::GREATER:
					depthInfo.depthCompareOp = VK_COMPARE_OP_GREATER;
					break;
			}
		}
		else
		{
			depthInfo.depthWriteEnable = VK_FALSE;
			depthInfo.depthTestEnable = VK_FALSE;
		}

		depthInfo.minDepthBounds = 0.0f;
		depthInfo.maxDepthBounds = 1.0f;

		/* Rasterisation Behaviour Settings */
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;
		rasterizationInfo.depthClampEnable = VK_FALSE;
		rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		switch (_initData.fillMode)
		{
			case (FillMode::WIREFRAME):
				rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
				break;

			case (FillMode::FILL):
			default:
				rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		}
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		/* Blend Info */
		VkPipelineColorBlendAttachmentState blendState[1]{};
		if (_initData.alphaBlend == AlphaBlend::ENABLED)
		{
			blendState[0].blendEnable = VK_TRUE;
			blendState[0].colorBlendOp = VK_BLEND_OP_ADD;
			blendState[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendState[0].alphaBlendOp = VK_BLEND_OP_ADD;
			blendState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		}
		else
		{
			blendState[0].blendEnable = VK_FALSE;
		}
		blendState[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blendInfo{};
		blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blendInfo.attachmentCount = 1;
		blendInfo.pAttachments = blendState;
		blendInfo.logicOpEnable = VK_FALSE;

		/* Create the Pipeline (finally) */
		VkGraphicsPipelineCreateInfo plInfo{};
		plInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		plInfo.stageCount = static_cast<uint32_t>(stagesInfo.size());
		plInfo.pStages = stagesInfo.data();

		plInfo.pVertexInputState = &vertexInputInfo;
		plInfo.pInputAssemblyState = &inputAssemblyInfo;
		plInfo.pTessellationState = nullptr;
		plInfo.pViewportState = &viewportInfo;
		plInfo.pRasterizationState = &rasterizationInfo;
		plInfo.pMultisampleState = &multisampleInfo;
		plInfo.pColorBlendState = &blendInfo;
		plInfo.pDepthStencilState = &depthInfo;

		plInfo.layout = *_layout;
		plInfo.renderPass = **_epRenderPass;
		plInfo.subpass = 0;

		VkPipeline pipeline = VK_NULL_HANDLE;
		if (const auto res = vkCreateGraphicsPipelines(environment->Window().device, VK_NULL_HANDLE, 1, &plInfo, nullptr, &pipeline); res != VK_SUCCESS)
		{
			throw Error("VK: vkCreateGraphicsPipelines() failed. err: %s",
				to_string(res).c_str());
		}

		_pipeline = labutils::Pipeline(environment->Window().device, pipeline);
	}

	/* public member functions */

	bool Pipeline::IsCompatible(const Environment* environment) const
	{
		if (_initData.specialMode == SpecialMode::NONE)
			return (environment->Window().swapchainExtent.width == _currentExtent.width &&
				environment->Window().swapchainExtent.height == _currentExtent.height);
		else
			return true;
	}

	void Pipeline::Repair(const Environment* environment, const Renderer::RenderPass* render_pass)
	{
		if (render_pass != nullptr)
			_epRenderPass = render_pass;

		createPipeline(environment);
	}

	void Pipeline::CmdBind(Environment* environment)
	{
		vkCmdBindPipeline(*environment->CurrentCmdBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, *_pipeline);
	}

	/* getters */

	const labutils::PipelineLayout& Pipeline::GetPipelineLayout() const
	{
		return _layout;
	}
	const labutils::Pipeline& Pipeline::GetPipeline() const
	{
		return _pipeline;
	}
}