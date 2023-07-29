#include "RenderPass.hpp"

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vulkan_window.hpp" // <- class VulkanWindow

namespace Renderer
{
	/* constructors, etc. */

	RenderPass::RenderPass(const labutils::VulkanWindow* window, const RenderPassFeatures& init_data)
	{
		/* Make sure the pipeline is actually rendering something */
		assert(init_data.colourPass == ColourPass::ENABLED || init_data.depthTest == DepthTest::ENABLED);

		_initData = init_data;

		createRenderPass(window);
	}

	RenderPass::~RenderPass()
	{
		/* This space intentionally left blank */
	}

	/* private member functions */

	void RenderPass::createRenderPass(const labutils::VulkanWindow* window)
	{
		using namespace labutils;

		/* I know this function could be a lot more efficient, but time constraints are... constraining. */

		int32_t attachmentCount = 0;

		if (_initData.colourPass == ColourPass::ENABLED)
			attachmentCount++;

		if (_initData.depthTest == DepthTest::ENABLED)
			attachmentCount++;

		std::vector< VkAttachmentDescription> attachments(attachmentCount, VkAttachmentDescription{});
		int32_t curAttachInd = 0;
		int32_t colourInd = -1;
		int32_t depthInd = -1;

		if (_initData.colourPass == ColourPass::ENABLED)
		{
			/* Colour Buffer */
			attachments[curAttachInd].format = (_initData.specialColour == SpecialColour::NONE) ? window->swapchainFormat : VK_FORMAT_R32G32B32A32_SFLOAT;
			attachments[curAttachInd].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[curAttachInd].loadOp = (_initData.clearColour == ClearColour::ENABLED) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			attachments[curAttachInd].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[curAttachInd].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (_initData.renderTarget == RenderTarget::PRESENT)
				attachments[curAttachInd].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			else if (_initData.renderTarget == RenderTarget::TEXTURE_GEOMETRY ||
				_initData.renderTarget == RenderTarget::TEXTURE_POST_PROC ||
				_initData.renderTarget == RenderTarget::TEXTURE_COLORDEPTH)
				attachments[curAttachInd].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			colourInd = curAttachInd;
			curAttachInd++;
		}

		if (_initData.depthTest == DepthTest::ENABLED)
		{
			/* Depth Buffer */
			attachments[curAttachInd].format = VK_FORMAT_D32_SFLOAT;
			attachments[curAttachInd].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[curAttachInd].loadOp = (_initData.clearDepth == ClearDepth::ENABLED) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
			attachments[curAttachInd].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[curAttachInd].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[curAttachInd].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			depthInd = curAttachInd;
			curAttachInd++;
		}

		VkAttachmentReference subpassAttachment{};
		if (colourInd >= 0)
		{
			subpassAttachment.attachment = colourInd;
			subpassAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		VkAttachmentReference depthAttachment{};
		if (depthInd >= 0)
		{
			depthAttachment.attachment = depthInd;
			depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		VkSubpassDescription subpasses[1]{};
		subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount = (colourInd >= 0) ? 1 : 0;
		subpasses[0].pColorAttachments = (colourInd >= 0) ? &subpassAttachment : nullptr;
		subpasses[0].pDepthStencilAttachment = (depthInd >= 0) ? &depthAttachment : nullptr;

		VkRenderPassCreateInfo passInfo{};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passInfo.attachmentCount = attachmentCount;
		passInfo.pAttachments = attachments.data();
		passInfo.subpassCount = 1;
		passInfo.pSubpasses = subpasses;

		VkRenderPass renderPass = VK_NULL_HANDLE;
		if (auto const res = vkCreateRenderPass(window->device, &passInfo, nullptr, &renderPass); res != VK_SUCCESS)
		{
			throw Error("vk: vkCreateRenderPass() failed. err: %s",
				to_string(res).c_str());
		}

		_renderPass = labutils::RenderPass(window->device, renderPass);
	}

	/* public member functions */

	void RenderPass::Repair(const labutils::VulkanWindow* window)
	{
		createRenderPass(window);
	}

	/* getters */

	const VkRenderPass& RenderPass::operator*() const
	{
		return *_renderPass;
	}
	const RenderPassFeatures& RenderPass::Features() const
	{
		return _initData;
	}
}
