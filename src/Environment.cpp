#include "Environment.hpp"

/* c++ */
#include <limits>

/* renderer */ 
#include "Constants.hpp"
#include "CreationUtilities.hpp"
#include "Pipeline.hpp" // <- class Pipeline
#include "RenderPass.hpp" // <- class RenderPass

/* labutils */
#include "../labutils/error.hpp"
#include "../labutils/to_string.hpp"
#include "../labutils/vkutil.hpp"

namespace Renderer
{
	/* constructors, etc. */

	Environment::Environment()
	{
		_window = lut::make_vulkan_window();
		_allocator = lut::create_allocator(_window);
		_cmdPool = lut::create_command_pool(_window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		_descPool = CreateDescriptorPool(_window.device);

		_frameStrat = &ENV_STRAT_FIRSTFRAME_INIT;
	}

	Environment::~Environment()
	{
		if (_intermediateTextureSet[0] == nullptr)
			delete _intermediateTextureSet[0];

		if (_intermediateTextureSet[1] == nullptr)
			delete _intermediateTextureSet[1];
	}

	/* private member functions */

	void Environment::createDepthBuffer()
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_D32_SFLOAT;
		imageInfo.extent.width = _window.swapchainExtent.width;
		imageInfo.extent.height = _window.swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		if (const auto& res = vmaCreateImage(_allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res)
		{
			throw lut::Error("VK: vmaCreateImage() failed while creating a depth buffer image. err: %s",
				lut::to_string(res).c_str());
		}

		lut::Image depthImage(_allocator.allocator, image, allocation);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthImage.image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_D32_SFLOAT;
		viewInfo.components = VkComponentMapping{};
		viewInfo.subresourceRange = VkImageSubresourceRange
		{
			VK_IMAGE_ASPECT_DEPTH_BIT,
			0, 1,
			0, 1
		};

		VkImageView view = VK_NULL_HANDLE;
		if (const auto& res = vkCreateImageView(_window.device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkCreateImageView() failed to create an image view for a depth buffer image. err: %s",
				lut::to_string(res).c_str());
		}

		_depthBuffer = std::move(depthImage);
		_depthView = lut::ImageView(_window.device, view);
	}

	void Environment::createIntermediateBuffers()
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
		imageInfo.extent.width = _window.swapchainExtent.width;
		imageInfo.extent.height = _window.swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		_intermediateSampler = Renderer::CreateDefaultSampler(_window);

		_postPresentLayoutData.stages.fragment = true;
		_postPresentLayoutData.bindingCount = 1;
		DescriptorSetType postPresentSets = Renderer::DescriptorSetType::SAMPLER;
		_postPresentLayoutData.pBindingTypes = &postPresentSets;
		DescriptorSetLayout postPresentLayout(this, _postPresentLayoutData);

		for (uint32_t i = 0; i < 2; i++)
		{
			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			VkImage image = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;

			if (const auto& res = vmaCreateImage(_allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res)
			{
				throw lut::Error("VK: vmaCreateImage() failed while creating a deferred intermediate image. err: %s",
					lut::to_string(res).c_str());
			}

			lut::Image intermediateImage(_allocator.allocator, image, allocation);

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = intermediateImage.image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			viewInfo.components = VkComponentMapping
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};
			viewInfo.subresourceRange = VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			};

			VkImageView view = VK_NULL_HANDLE;
			if (const auto& res = vkCreateImageView(_window.device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateImageView() failed to create an image view for a deferred intermediate image. err: %s",
					lut::to_string(res).c_str());
			}

			_intermediateBuffer[i] = std::move(intermediateImage);
			_intermediateView[i] = lut::ImageView(_window.device, view);

			/* Create descriptor sets */

			_intermediateTextureFeatures.binding = 0;
			_intermediateTextureFeatures.s_View = *_intermediateView[i];
			_intermediateTextureFeatures.s_Sampler = *_intermediateSampler;
			_intermediateTextureSet[i] = new DescriptorSet(this, &postPresentLayout,
				1, &_intermediateTextureFeatures);
		}
	}

	lut::SwapChanges Environment::repairSwapChain()
	{
		return lut::recreate_swapchain(_window);
	}

	void Environment::createIntermediateFramebuffers(const Renderer::RenderPass* render_pass)
	{
		assert(_intermediateFramebuffers.empty());

		for (uint32_t i = 0; i < 2; i++)
		{
			VkImageView attachments[2]
			{
				*_intermediateView[i],
				*_depthView
			};

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.flags = 0;
			fbInfo.renderPass = **render_pass;
			fbInfo.attachmentCount = 2;
			fbInfo.pAttachments = attachments;
			fbInfo.width = _window.swapchainExtent.width;
			fbInfo.height = _window.swapchainExtent.height;
			fbInfo.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			if (const auto& res = vkCreateFramebuffer(_window.device, &fbInfo, nullptr, &framebuffer); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateFramebuffer() failed to create hidden framebuffer. err: %s",
					lut::to_string(res).c_str());
			}

			_intermediateFramebuffers.push_back(lut::Framebuffer(_window.device, framebuffer));
		}
	}

	void Environment::createPostProcessingFramebuffers(const Renderer::RenderPass* render_pass)
	{
		assert(_postProcessingFramebuffers.empty());

		for (uint32_t i = 0; i < 2; i++)
		{
			VkImageView attachments[1]
			{
				*_intermediateView[i]
			};

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.flags = 0;
			fbInfo.renderPass = **render_pass;
			fbInfo.attachmentCount = 1;
			fbInfo.pAttachments = attachments;
			fbInfo.width = _window.swapchainExtent.width;
			fbInfo.height = _window.swapchainExtent.height;
			fbInfo.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			if (const auto& res = vkCreateFramebuffer(_window.device, &fbInfo, nullptr, &framebuffer); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateFramebuffer() failed to create hidden framebuffer. err: %s",
					lut::to_string(res).c_str());
			}

			_postProcessingFramebuffers.push_back(lut::Framebuffer(_window.device, framebuffer));
		}
	}

	void Environment::createPresentationFramebuffers(const Renderer::RenderPass* render_pass)
	{
		assert(_swapChainFramebuffers.empty());

		for (auto i = 0U; i < _window.swapViews.size(); i++)
		{
			VkImageView attachments[1]
			{
				_window.swapViews[i],
			};

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.flags = 0;
			fbInfo.renderPass = **render_pass;
			fbInfo.attachmentCount = 1;
			fbInfo.pAttachments = attachments;
			fbInfo.width = _window.swapchainExtent.width;
			fbInfo.height = _window.swapchainExtent.height;
			fbInfo.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			if (const auto& res = vkCreateFramebuffer(_window.device, &fbInfo, nullptr, &framebuffer); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateFramebuffer() failed to create the [%u]th framebuffer for the swapchain. err: %s",
					i, lut::to_string(res).c_str());
			}

			_swapChainFramebuffers.push_back(lut::Framebuffer(_window.device, framebuffer));
		}

		assert(_window.swapViews.size() == _swapChainFramebuffers.size());
	}

	/* public member functions */

	void Environment::InitialiseSwapChain(std::vector<Renderer::RenderPass*> render_passes)
	{
		assert(_state == State::UNINITIALISED);

		/* Swap chain, framebuffers, command pool, and associated synch resources */
		createDepthBuffer();
		createIntermediateBuffers();
		_intermediateFramebuffers.clear();
		createIntermediateFramebuffers(render_passes[0]);
		// _postProcessingFramebuffers.clear();
		// createPostProcessingFramebuffers(render_passes[1]);
		_swapChainFramebuffers.clear();
		createPresentationFramebuffers(render_passes[1]);

		_cmdBuffers.clear();
		_cmdFences.clear();

		for (std::size_t i = 0; i < _swapChainFramebuffers.size(); ++i)
		{
			_cmdBuffers.emplace_back(lut::alloc_command_buffer(_window, *_cmdPool));
			_cmdFences.emplace_back(lut::create_fence(_window, VK_FENCE_CREATE_SIGNALED_BIT));
		}

		_imageAvailable = lut::create_semaphore(_window);
		_renderFinished = lut::create_semaphore(_window);

		_state = State::READY;
	}

	ErrorCode Environment::CheckSwapChain(std::vector<Renderer::RenderPass*> render_passes)
	{
		assert(_state == State::READY);

		auto ret = ErrorCode::SUCCESS;

		if (_swapState != SwapChainState::READY)
		{
			vkDeviceWaitIdle(_window.device);

			const auto changes = repairSwapChain();

			if (changes.changedFormat)
				render_passes[1]->Repair(&_window);

			_swapChainFramebuffers.clear();

			if (changes.changedSize)
				createDepthBuffer();

			_intermediateFramebuffers.clear();
			createIntermediateFramebuffers(render_passes[0]);
			// _postProcessingFramebuffers.clear();
			// createPostProcessingFramebuffers(render_passes[1]);
			_swapChainFramebuffers.clear();
			createPresentationFramebuffers(render_passes[1]);

			ret = ErrorCode::FAILURE;
			_swapState = SwapChainState::READY;
		}

		return ret;
	}

	ErrorCode Environment::PrepareNextFrame()
	{
		assert(_state == State::READY);

		auto ret = ErrorCode::SUCCESS;

		/* Acquire the next swap chain image. */
		const auto& swapImageRes = vkAcquireNextImageKHR(_window.device, _window.swapchain,
			std::numeric_limits<uint32_t>::max(),
			*_imageAvailable, VK_NULL_HANDLE,
			&_currentSwapImage);

		if (swapImageRes == VK_SUBOPTIMAL_KHR || swapImageRes == VK_ERROR_OUT_OF_DATE_KHR)
		{
			ret = ErrorCode::FAILURE;
			_swapState = SwapChainState::INVALID;
		}
		else if (swapImageRes != VK_SUCCESS) // I guess the 'else' is a little unnecessary here...
		{
			throw lut::Error("VK: vkAcquireNextImageKHR() experienced an unrecoverable failure. err: %s",
				lut::to_string(swapImageRes));
		}
		else
		{
			assert(static_cast<std::size_t>(_currentSwapImage) < _cmdFences.size());

			/* Wait for the fences and then reset them for the next frame. */
			if (const auto& res = vkWaitForFences(_window.device, 1, &*_cmdFences[_currentSwapImage], VK_TRUE, std::numeric_limits<uint32_t>::max());
				res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkWaitForFences() failed. err: %s",
					lut::to_string(res).c_str());
			}

			if (const auto& res = vkResetFences(_window.device, 1, &*_cmdFences[_currentSwapImage]); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkResetFences() failed. err: %s",
					lut::to_string(res).c_str());
			}

			assert(static_cast<std::size_t>(_currentSwapImage) < _cmdBuffers.size());
			assert(static_cast<std::size_t>(_currentSwapImage) < _swapChainFramebuffers.size());
		}

		return ret;
	}

	void Environment::BeginFrameCommands()
	{
		assert(_state == State::READY);

		VkCommandBufferBeginInfo beginfo{};
		beginfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginfo.pInheritanceInfo = nullptr;

		if (const auto res = vkBeginCommandBuffer(_cmdBuffers[_currentSwapImage], &beginfo); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkBeginCommandBuffer() failed to begin recording command buffer. err: %s",
				lut::to_string(res).c_str());
		}

		_state = State::RECORDING_NOPASS;

		/* Do first frame actions if necessary */
		_frameStrat = _frameStrat->Execute(this);
	}

	void Environment::BeginRenderPass(const Renderer::RenderPass* render_pass, int32_t side_buffer_index,
		uint32_t targetWidth, uint32_t targetHeight)
	{
		assert(_state == State::RECORDING_NOPASS);

		VkExtent2D resolution = { targetWidth, targetHeight };
		if (resolution.width == 0 || resolution.height == 0)
		{
			resolution = (render_pass->Features().renderTarget == RenderTarget::TEXTURE_SHADOWMAP) ?
				VkExtent2D{ SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION } : _window.swapchainExtent;
		}

		/* Get ready to start the render pass */
		std::vector<VkClearValue> clearValues{};

		if (render_pass->Features().colourPass == ColourPass::ENABLED && render_pass->Features().clearColour == ClearColour::ENABLED)
		{
			clearValues.push_back({});
			/*clearValues.back().color.float32[0] = 0.53f;
			clearValues.back().color.float32[1] = 0.81f;
			clearValues.back().color.float32[2] = 0.92f;
			clearValues.back().color.float32[3] = 1.0f;*/

			clearValues.back().color.float32[0] = 1.0f;
			clearValues.back().color.float32[1] = 1.0f;
			clearValues.back().color.float32[2] = 1.0f;
			clearValues.back().color.float32[3] = 1.0f;
		}

		if (render_pass->Features().depthTest == DepthTest::ENABLED && render_pass->Features().clearDepth == ClearDepth::ENABLED)
		{
			clearValues.push_back({});
			clearValues.back().depthStencil.depth = 1.0f;
		}

		VkRenderPassBeginInfo passInfo{};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passInfo.renderPass = **render_pass;
		if (side_buffer_index == -1)
		{
			if (render_pass->Features().renderTarget == RenderTarget::PRESENT)
				passInfo.framebuffer = *_swapChainFramebuffers[_currentSwapImage];
			else if (render_pass->Features().renderTarget == RenderTarget::TEXTURE_GEOMETRY)
				passInfo.framebuffer = *_intermediateFramebuffers[_drawIntermediateImage];
			else if (render_pass->Features().renderTarget == RenderTarget::TEXTURE_POST_PROC)
				passInfo.framebuffer = *_postProcessingFramebuffers[_drawIntermediateImage];
		}
		else
		{
			assert(static_cast<size_t>(side_buffer_index) < _sideFramebuffers.size());
			passInfo.framebuffer = *_sideFramebuffers[side_buffer_index];
		}
		passInfo.renderArea.offset = VkOffset2D{ 0, 0 };
		passInfo.renderArea.extent = resolution;
		passInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		passInfo.pClearValues = clearValues.data();

		/* Actually start the render pass */
		vkCmdBeginRenderPass(_cmdBuffers[_currentSwapImage], &passInfo, VK_SUBPASS_CONTENTS_INLINE);

		_state = State::RECORDING_RENDERPASS;
	}

	void Environment::EndRenderPass()
	{
		assert(_state == State::RECORDING_RENDERPASS);

		vkCmdEndRenderPass(_cmdBuffers[_currentSwapImage]);

		_state = State::RECORDING_NOPASS;
	}

	void Environment::EndFrameCommands()
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		if (const auto res = vkEndCommandBuffer(_cmdBuffers[_currentSwapImage]); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkEndCommandBuffer() failed to end recording command buffer. err: %s",
				lut::to_string(res).c_str());
		}

		VkPipelineStageFlags waitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo subInfo{};
		subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		subInfo.commandBufferCount = 1;
		subInfo.pCommandBuffers = &_cmdBuffers[_currentSwapImage];

		subInfo.waitSemaphoreCount = 1;
		subInfo.pWaitSemaphores = &*_imageAvailable;
		subInfo.pWaitDstStageMask = &waitPipelineStages;

		subInfo.signalSemaphoreCount = 1;
		subInfo.pSignalSemaphores = &*_renderFinished;

		if (const auto res = vkQueueSubmit(_window.graphicsQueue, 1, &subInfo, *_cmdFences[_currentSwapImage]); res != VK_SUCCESS)
		{
			throw lut::Error("VK: vkEndCommandBuffer() failed to end recording command buffer\n",
				lut::to_string(res).c_str());
		}

		_state = State::READY;
	}

	ErrorCode Environment::Present()
	{
		assert(_state == State::READY);

		auto ret = ErrorCode::SUCCESS;

		/* Present to the window surface */
		VkPresentInfoKHR presInfo{};
		presInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presInfo.waitSemaphoreCount = 1;
		presInfo.pWaitSemaphores = &*_renderFinished;
		presInfo.swapchainCount = 1;
		presInfo.pSwapchains = &_window.swapchain;
		presInfo.pImageIndices = &_currentSwapImage;
		presInfo.pResults = nullptr;

		const auto& presRes = vkQueuePresentKHR(_window.presentQueue, &presInfo);

		if (presRes == VK_SUBOPTIMAL_KHR || presRes == VK_ERROR_OUT_OF_DATE_KHR)
		{
			ret = ErrorCode::FAILURE;
			_swapState = SwapChainState::INVALID;
		}
		else if (presRes != VK_SUCCESS)
		{
			throw lut::Error("VK: vkQueuePresentKHR() experienced an unrecoverable failure. err: %s",
				lut::to_string(presRes));
		}

		return ret;
	}

	void Environment::CmdPrimeIntermediates()
	{
		assert(_intermediatesPrimed == false);

		/* BARRIER: colour attachment -> shader read */
		lut::image_barrier(*CurrentCmdBuffer(), *_intermediateBuffer[_presentIntermediateImage],
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			});

		/* BARRIER: shader read -> colour attachment */
		lut::image_barrier(*CurrentCmdBuffer(), *_intermediateBuffer[_drawIntermediateImage],
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			});

		_intermediatesPrimed = true;
	}

	void Environment::CmdSwapIntermediates()
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		_presentIntermediateImage = _drawIntermediateImage;
		_drawIntermediateImage = ((_drawIntermediateImage + 1) % 2);

		/* BARRIER: colour attachment -> shader read */
		lut::image_barrier(*CurrentCmdBuffer(), *_intermediateBuffer[_presentIntermediateImage],
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			});

		/* BARRIER: shader read -> colour attachment */
		lut::image_barrier(*CurrentCmdBuffer(), *_intermediateBuffer[_drawIntermediateImage],
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VkImageSubresourceRange
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			});
	}

	/* getters */

	const lut::VulkanWindow& Environment::Window() const
	{
		return _window;
	}

	const lut::VulkanWindow* Environment::WindowPtr() const
	{
		return &_window;
	}

	uint32_t Environment::CreateSideBuffers(
		Renderer::RenderPass* render_pass,
		uint32_t count,
		SideBufferType type,
		bool sharedBuffers,
		SideBufferShareData* shareData,
		int Width,
		int Height)
	{
		assert(count > 0);
		assert((sharedBuffers) ? (shareData != nullptr) : true);

		uint32_t width = (Width >= 0) ? Width : ((type == SideBufferType::DEPTH) ? SHADOW_MAP_RESOLUTION : _window.swapchainExtent.width);
		uint32_t height = (Height >= 0) ? Height : ((type == SideBufferType::DEPTH) ? SHADOW_MAP_RESOLUTION : _window.swapchainExtent.height);
		VkExtent2D resolution = { width, height };

		uint32_t ret = static_cast<uint32_t>(_sideBufferViews.size());

		for (uint32_t i = 0; i < count; i++)
		{
			/* stores views to be used as framebuffer attachments (could be new or copied from existing buffers) */
			std::vector<VkImageView> views = {};

			/* make a new vector for new images and image views */
			_sideBuffers.push_back({});
			_sideBufferViews.push_back({});

			if (type == SideBufferType::COLOUR || type == SideBufferType::COMBINED)
			{
				if (sharedBuffers == true && shareData->colourIndex > -1 && shareData->colourSubindex > -1)
				{
					views.push_back(*GetSideBufferImageView(shareData->colourIndex)->at(shareData->colourSubindex));
				}
				else
				{
					/* create COLOUR image and image view */
					VkImageCreateInfo imageInfo{};
					imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageInfo.imageType = VK_IMAGE_TYPE_2D;
					imageInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
					imageInfo.extent.width = resolution.width;
					imageInfo.extent.height = resolution.height;
					imageInfo.extent.depth = 1;
					imageInfo.mipLevels = 1;
					imageInfo.arrayLayers = 1;
					imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
					imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
					imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					VmaAllocationCreateInfo allocInfo{};
					allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

					VkImage image = VK_NULL_HANDLE;
					VmaAllocation allocation = VK_NULL_HANDLE;

					if (const auto& res = vmaCreateImage(_allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res)
					{
						throw lut::Error("VK: vmaCreateImage() failed while creating a deferred intermediate image. err: %s",
							lut::to_string(res).c_str());
					}

					lut::Image sideImage(_allocator.allocator, image, allocation);

					VkImageViewCreateInfo viewInfo{};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.image = sideImage.image;
					viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
					viewInfo.components = VkComponentMapping
					{
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY,
						VK_COMPONENT_SWIZZLE_IDENTITY
					};
					viewInfo.subresourceRange = VkImageSubresourceRange
					{
						VK_IMAGE_ASPECT_COLOR_BIT,
						0, 1,
						0, 1
					};

					VkImageView view = VK_NULL_HANDLE;
					if (const auto& res = vkCreateImageView(_window.device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
					{
						throw lut::Error("VK: vkCreateImageView() failed to create an image view for a side image. err: %s",
							lut::to_string(res).c_str());
					}

					_sideBuffers.back().push_back(std::move(sideImage));
					_sideBufferViews.back().push_back(lut::ImageView(_window.device, view));

					views.push_back(view);
				}
			}
			if (type == SideBufferType::DEPTH || type == SideBufferType::COMBINED)
			{
				if (sharedBuffers == true && shareData->depthIndex > -1 && shareData->depthSubindex > -1)
				{
					views.push_back(*GetSideBufferImageView(shareData->depthIndex)->at(shareData->depthSubindex));
				}
				else
				{
					/* create DEPTH image and image view */
					VkImageCreateInfo imageInfo{};
					imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageInfo.imageType = VK_IMAGE_TYPE_2D;
					imageInfo.format = VK_FORMAT_D32_SFLOAT;
					imageInfo.extent.width = resolution.width;
					imageInfo.extent.height = resolution.height;
					imageInfo.extent.depth = 1;
					imageInfo.mipLevels = 1;
					imageInfo.arrayLayers = 1;
					imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
					imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
					imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
					imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

					VmaAllocationCreateInfo allocInfo{};
					allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

					VkImage image = VK_NULL_HANDLE;
					VmaAllocation allocation = VK_NULL_HANDLE;

					if (const auto& res = vmaCreateImage(_allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res)
					{
						throw lut::Error("VK: vmaCreateImage() failed while creating a depth buffer image. err: %s",
							lut::to_string(res).c_str());
					}

					lut::Image depthImage(_allocator.allocator, image, allocation);

					VkImageViewCreateInfo viewInfo{};
					viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					viewInfo.image = depthImage.image;
					viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					viewInfo.format = VK_FORMAT_D32_SFLOAT;
					viewInfo.components = VkComponentMapping{};
					viewInfo.subresourceRange = VkImageSubresourceRange
					{
						VK_IMAGE_ASPECT_DEPTH_BIT,
						0, 1,
						0, 1
					};

					VkImageView view = VK_NULL_HANDLE;
					if (const auto& res = vkCreateImageView(_window.device, &viewInfo, nullptr, &view); res != VK_SUCCESS)
					{
						throw lut::Error("VK: vkCreateImageView() failed to create an image view for a depth buffer image. err: %s",
							lut::to_string(res).c_str());
					}

					_sideBuffers.back().push_back(std::move(depthImage));
					_sideBufferViews.back().push_back(lut::ImageView(_window.device, view));

					views.push_back(view);
				}
			}

			/* Create the frambuffer */

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.flags = 0;
			fbInfo.renderPass = **render_pass;
			fbInfo.attachmentCount = static_cast<uint32_t>(views.size());
			fbInfo.pAttachments = views.data();
			fbInfo.width = resolution.width;
			fbInfo.height = resolution.height;
			fbInfo.layers = 1;

			VkFramebuffer framebuffer = VK_NULL_HANDLE;
			if (const auto& res = vkCreateFramebuffer(_window.device, &fbInfo, nullptr, &framebuffer); res != VK_SUCCESS)
			{
				throw lut::Error("VK: vkCreateFramebuffer() failed to create side framebuffer. err: %s",
					lut::to_string(res).c_str());
			}

			_sideFramebuffers.push_back(lut::Framebuffer(_window.device, framebuffer));
		}

		return ret;
	}
	std::vector<lut::Image>* Environment::GetSideBufferImage(uint32_t index)
	{
		assert(index < _sideBufferViews.size());

		return &_sideBuffers[index];
	}
	std::vector<lut::ImageView>* Environment::GetSideBufferImageView(uint32_t index)
	{
		assert(index < _sideBufferViews.size());

		return &_sideBufferViews[index];
	}

	const lut::Allocator& Environment::Allocator() const
	{
		return _allocator;
	}
	const lut::Allocator* Environment::AllocatorPtr() const
	{
		return &_allocator;
	}

	const lut::CommandPool& Environment::CommandPool() const
	{
		return _cmdPool;
	}
	const lut::CommandPool* Environment::CommandPoolPtr() const
	{
		return &_cmdPool;
	}

	const lut::DescriptorPool& Environment::DescPool() const
	{
		return _descPool;
	}

	const lut::DescriptorPool* Environment::DescPoolPtr() const
	{
		return &_descPool;
	}

	const VkCommandBuffer* Environment::CurrentCmdBuffer()
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		return &_cmdBuffers[_currentSwapImage];
	}
	const lut::Framebuffer* Environment::CurrentPresentationFramebuffer()
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		return &_swapChainFramebuffers[_currentSwapImage];
	}
	const lut::Framebuffer* Environment::IntermediateDrawFramebuffer()
	{
		return &_intermediateFramebuffers[_drawIntermediateImage];
	}
	const lut::Framebuffer* Environment::IntermediatePresentFramebuffer()
	{
		return &_intermediateFramebuffers[_presentIntermediateImage];
	}
	const lut::Image* Environment::IntermediateDrawTextureImage()
	{
		return &_intermediateBuffer[_drawIntermediateImage];
	}
	const lut::ImageView* Environment::IntermediateDrawTextureImageView()
	{
		return &_intermediateView[_drawIntermediateImage];
	}
	void Environment::CmdBindIntermediatePresentTexture(Renderer::Pipeline* pipeline, uint32_t set_index)
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		_intermediateTextureSet[_presentIntermediateImage]->CmdBind(this, pipeline, set_index);
	}
	void Environment::CmdBindIntermediateDrawTexture(Renderer::Pipeline* pipeline, uint32_t set_index)
	{
		assert(_state == State::RECORDING_NOPASS || _state == State::RECORDING_RENDERPASS);

		_intermediateTextureSet[_drawIntermediateImage]->CmdBind(this, pipeline, set_index);
	}

	const Renderer::Pipeline* Environment::CurrentPipeline()
	{
		assert(_epCurrentPipeline != nullptr);

		return _epCurrentPipeline;
	}
}
