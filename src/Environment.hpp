#pragma once

/* renderer */ 
#include "ErrorCode.hpp"
#include "DescriptorSets.hpp"
#include "Env_Strat_FirstFrame.hpp"

/* labutils */
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vulkan_window.hpp"
#include "../labutils/allocator.hpp"

namespace Renderer
{
	class Pipeline;
	class RenderPass;
}

namespace Renderer
{
	namespace lut = labutils;

	struct SideBufferShareData
	{
		int colourIndex = -1;
		int colourSubindex = -1;

		int depthIndex = -1;
		int depthSubindex = -1;
	};

	class Environment
	{
		public:
			/* constructors, etc. */

			Environment();
			~Environment();

			Environment(const Environment&) = delete;
			Environment& operator=(const Environment&) = delete;

		private:
			/* private enum types*/

			enum class State
			{
				UNINITIALISED = 0,
				READY,
				RECORDING_NOPASS,
				RECORDING_RENDERPASS
			};

			enum class SwapChainState
			{
				READY = 0,
				INVALID
			};

			/* private member variables */

			State _state = State::UNINITIALISED;
			SwapChainState _swapState = SwapChainState::INVALID;

			Renderer::Pipeline* _epCurrentPipeline = nullptr;

			lut::VulkanWindow _window{};
			lut::Allocator _allocator{};
			lut::CommandPool _cmdPool{};
			lut::DescriptorPool _descPool{};

			std::vector<lut::Framebuffer> _intermediateFramebuffers{};
			std::vector<lut::Framebuffer> _postProcessingFramebuffers{};
			std::vector<lut::Framebuffer> _sideFramebuffers{};
			std::vector<lut::Framebuffer> _swapChainFramebuffers{};

			lut::Image _depthBuffer{};
			lut::ImageView _depthView{};

			lut::Image _intermediateBuffer[2]{};
			lut::ImageView _intermediateView[2]{};
			uint32_t _drawIntermediateImage = 0;
			uint32_t _presentIntermediateImage = 1;
			bool _intermediatesPrimed = false;
			FirstFrameStrat* _frameStrat;

			std::vector<std::vector<lut::Image>> _sideBuffers{};
			std::vector<std::vector<lut::ImageView>> _sideBufferViews{};

			lut::Sampler _intermediateSampler{};
			DescriptorSetLayoutFeatures _postPresentLayoutData{};
			DescriptorSetFeatures _intermediateTextureFeatures;
			DescriptorSet* _intermediateTextureSet[2];

			std::vector<VkCommandBuffer> _cmdBuffers{};
			std::vector<lut::Fence> _cmdFences{};

			lut::Semaphore _imageAvailable{};
			lut::Semaphore _renderFinished{};

			uint32_t _currentSwapImage = 0;

			/* private member functions */

			void createDepthBuffer();
			void createIntermediateBuffers();
			lut::SwapChanges repairSwapChain();
			void createIntermediateFramebuffers(const Renderer::RenderPass* render_pass);
			void createPostProcessingFramebuffers(const Renderer::RenderPass* render_pass);
			void createPresentationFramebuffers(const Renderer::RenderPass* render_pass);

		public:

			enum class SideBufferType
			{
				COLOUR = 0,
				DEPTH,
				COMBINED
			};

			/* public member functions */
			
			void InitialiseSwapChain(std::vector<Renderer::RenderPass*> render_passes);
			ErrorCode CheckSwapChain(std::vector<Renderer::RenderPass*> render_passes);
			ErrorCode PrepareNextFrame();
			void BeginFrameCommands();
			void BeginRenderPass(const Renderer::RenderPass* render_pass, int32_t side_buffer_index = -1);
			void EndRenderPass();
			void EndFrameCommands();
			ErrorCode Present();

			void CmdPrimeIntermediates();
			void CmdSwapIntermediates();

			uint32_t CreateSideBuffers(
				Renderer::RenderPass* render_pass,
				uint32_t count = 1,
				SideBufferType type = SideBufferType::COLOUR,
				bool sharedBuffers = false,
				SideBufferShareData* shareData = nullptr,
				int Width = -1,
				int Height = -1);
			std::vector<lut::Image>* GetSideBufferImage(uint32_t index);
			std::vector<lut::ImageView>* GetSideBufferImageView(uint32_t index);

			/* getters */

			const lut::VulkanWindow& Window() const;
			const lut::VulkanWindow* WindowPtr() const;

			const lut::Allocator& Allocator() const;
			const lut::Allocator* AllocatorPtr() const;

			const lut::CommandPool& CommandPool() const;
			const lut::CommandPool* CommandPoolPtr() const;

			const lut::DescriptorPool& DescPool() const;
			const lut::DescriptorPool* DescPoolPtr() const;

			const VkCommandBuffer* CurrentCmdBuffer();
			const lut::Framebuffer* CurrentPresentationFramebuffer();
			const lut::Framebuffer* IntermediateDrawFramebuffer();
			const lut::Framebuffer* IntermediatePresentFramebuffer();
			const lut::Image* IntermediateDrawTextureImage();
			const lut::ImageView* IntermediateDrawTextureImageView();
			void CmdBindIntermediatePresentTexture(Renderer::Pipeline* pipeline, uint32_t set_index);
			void CmdBindIntermediateDrawTexture(Renderer::Pipeline* pipeline, uint32_t set_index);

			const Renderer::Pipeline* CurrentPipeline();

	};
}
