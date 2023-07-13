
/* glm */
#if !defined(GLM_FORCE_RADIANS)
#	define GLM_FORCE_RADIANS
#endif
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

/* labutils */
#include "../labutils/to_string.hpp"
#include "../labutils/vulkan_window.hpp"

#include "../labutils/angle.hpp"
using namespace labutils::literals;

#include "../labutils/error.hpp"
#include "../labutils/vkutil.hpp"
#include "../labutils/vkimage.hpp"
#include "../labutils/vkobject.hpp"
#include "../labutils/vkbuffer.hpp"
#include "../labutils/allocator.hpp" 
#include "../labutils/to_string.hpp"
namespace lut = labutils;

/* Renderer */
#include "BufferUtilities.hpp"
#include "CreationUtilities.hpp"
#include "DescriptorSets.hpp"
#include "Environment.hpp"
#include "ViewerCamera.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "RenderingUtilities.hpp"
#include "RenderPass.hpp"
#include "TextureUtilities.hpp"

const float FOV = 90.0f / 180.0f * 3.1415f;

int main(int argc, char** argv)
{
	printf("Application starting...\n");

	/* create the environment
		stores the state of the renderer as well as 
		various other data such as the context, window,
		frame buffers, texture buffer, etc. */
	Renderer::Environment env;

	/* create render passes */
	Renderer::RenderPassFeatures simpleOpaqueFeatures;
	simpleOpaqueFeatures.colourPass = Renderer::ColourPass::ENABLED;
	simpleOpaqueFeatures.depthTest = Renderer::DepthTest::ENABLED;
	simpleOpaqueFeatures.renderTarget = Renderer::RenderTarget::TEXTURE_GEOMETRY;
	Renderer::RenderPass simpleOpaquePass(env.WindowPtr(), simpleOpaqueFeatures);

	Renderer::RenderPassFeatures presentFeatures;
	presentFeatures.colourPass = Renderer::ColourPass::ENABLED;
	presentFeatures.depthTest = Renderer::DepthTest::DISABLED;
	presentFeatures.renderTarget = Renderer::RenderTarget::PRESENT;
	Renderer::RenderPass presentPass(env.WindowPtr(), presentFeatures);

	/* initialise swapchain */
	env.InitialiseSwapChain({ &simpleOpaquePass, &presentPass });

	/* set up the camera */
	Renderer::ViewerCamera camera(&env, FOV, 0.1f, 500.0f,
		env.Window().swapchainExtent.width, env.Window().swapchainExtent.height);
	camera.SetPosition(glm::vec3(0.0f, -1.0f, -15.0f));

	/* camera data uniform */
	Renderer::DescriptorSetLayout cameraUniformLayout(&env, { true, true, false }, Renderer::DescriptorSetType::UNIFORM_BUFFER);
	lut::Buffer cameraUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::CameraData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer::FreeUpdateBuffer(&env, &cameraUBO, 0, sizeof(Renderer::Uniforms::CameraData), camera.GetUniformDataPtr());
	Renderer::DescriptorSet cameraSet(&env, &cameraUniformLayout, *cameraUBO);

	/* set lighting parameters */
	Renderer::Uniforms::LightData lights;
	lights.sunLight.direction = glm::normalize(glm::vec4(0.0f, -1.0f, 1.0f, 0.0f));
	lights.sunLight.colour = glm::normalize(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	/* lighting uniform */
	Renderer::DescriptorSetLayout lightingUniformLayout(&env, Renderer::ShaderStageConstants::FRAGMENT_STAGE, Renderer::DescriptorSetType::UNIFORM_BUFFER);
	lut::Buffer lightingUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::LightData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer::FreeUpdateBuffer(&env, &lightingUBO, 0, sizeof(Renderer::Uniforms::LightData), &lights);
	Renderer::DescriptorSet lightingSet(&env, &lightingUniformLayout, *lightingUBO);

	/* material descriptor set(s) */
	Renderer::DescriptorSetLayoutFeatures simpleLayoutData{};
	simpleLayoutData.stages.fragment = true;
	simpleLayoutData.bindingCount = 3;
	Renderer::DescriptorSetType simpleSets[3]
	{
		Renderer::DescriptorSetType::SAMPLER, // colour
		Renderer::DescriptorSetType::SAMPLER, // metal & roughness
		Renderer::DescriptorSetType::UNIFORM_BUFFER // material data
	};
	simpleLayoutData.pBindingTypes = simpleSets;
	Renderer::DescriptorSetLayout simpleLayout(&env, simpleLayoutData);

	/* descriptor set for a single texture being made available in the fragment shader */
	Renderer::DescriptorSetLayoutFeatures singleTextureLayoutData{};
	singleTextureLayoutData.stages.fragment = true;
	singleTextureLayoutData.bindingCount = 1;
	Renderer::DescriptorSetType singleTextureTypes = Renderer::DescriptorSetType::SAMPLER;
	singleTextureLayoutData.pBindingTypes = &singleTextureTypes;
	Renderer::DescriptorSetLayout singleTextureLayout(&env, singleTextureLayoutData);

	/* sampler */
	lut::Sampler defaultSampler = Renderer::CreateDefaultSampler(env.Window());

	/* load model */
	Renderer::Model model(&env, "../res/models/teapot scene.glb", &simpleLayout, &defaultSampler);

	/* Pipelines and Dependencies */
	std::vector<const VkDescriptorSetLayout*> simpleLayouts = { &*cameraUniformLayout, &*simpleLayout, &*lightingUniformLayout };
	Renderer::Pipeline simplePipeline(&env, Renderer::Pipeline_Default, &simpleOpaquePass, simpleLayouts);

	std::vector<const VkDescriptorSetLayout*> postProcessingLayouts = { &*singleTextureLayout };
	Renderer::PipelineFeatures postPresentFeatures;
	postPresentFeatures.alphaBlend = Renderer::AlphaBlend::DISABLED;
	postPresentFeatures.fillMode = Renderer::FillMode::FILL;
	postPresentFeatures.specialMode = Renderer::SpecialMode::SCREEN_QUAD_PRESENT;
	Renderer::Pipeline postPresentPipeline(&env, postPresentFeatures, &presentPass, postProcessingLayouts);

	/* Main loop */
	double time = glfwGetTime();

	while (glfwWindowShouldClose(env.Window().window) == false)
	{
		/* Window polling */
		glfwPollEvents();

		/* Recreate the swap chain if it's been invalidated.
			This also necessitates adjusting the pipelines,
			since the window has likely changed size. */
		if (env.CheckSwapChain({ &simpleOpaquePass, &presentPass }) != ErrorCode::SUCCESS)
		{
			simplePipeline.Repair(&env);
			postPresentPipeline.Repair(&env);

			camera.UpdateCameraSettings(FOV,
				env.Window().swapchainExtent.width, env.Window().swapchainExtent.height);

			continue;
		}

		/* Get the next swap chain image, etc. and wait for fences */
		if (env.PrepareNextFrame() != ErrorCode::SUCCESS)
			continue;

		/* Update time and camera */
		double now = glfwGetTime();
		double timeDelta = now - time;
		time = now;
		camera.FrameUpdate(timeDelta);

		/* Prepare to queue commands */
		env.BeginFrameCommands();

		/* Update the camera and lighting data buffers */
		Renderer::CmdUpdateBuffer(&env, &cameraUBO, 0, sizeof(Renderer::Uniforms::CameraData), camera.GetUniformDataPtr());
		Renderer::CmdUpdateBuffer(&env, &lightingUBO, 0, sizeof(Renderer::Uniforms::LightData), &lights);

		/* Begin geometry pass */
		env.BeginRenderPass(&simpleOpaquePass); /* rendering to intermediate 0 */

		{
			/* draw meshes */
			simplePipeline.CmdBind(&env);
			cameraSet.CmdBind(&env, &simplePipeline, 0);
			lightingSet.CmdBind(&env, &simplePipeline, 2);
			model.CmdDrawOpaque(&env, &simplePipeline);
		}

		/* End render pass */
		env.EndRenderPass();

		/* Swap the intermediate images */
		env.CmdSwapIntermediates(); /* 0 -> 1 */

		/* Begin present pass */
		env.BeginRenderPass(&presentPass); /* rendering to swap chain */

		{
			postPresentPipeline.CmdBind(&env);
			env.CmdBindIntermediatePresentTexture(&postPresentPipeline, 0);
			Renderer::CmdDrawFullscreenQuad(&env);
		}

		/* End render pass */
		env.EndRenderPass();

		/* Submitted queued commands */
		env.EndFrameCommands();

		/* Present the frame */
		env.Present();
	}

	/* Wait for the GPU to finishing doing what it's doing. */
	vkDeviceWaitIdle(env.Window().device);

	return 0;
}
