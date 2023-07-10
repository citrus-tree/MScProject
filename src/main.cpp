
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
#include "FlyingCamera.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "RenderingUtilities.hpp"
#include "RenderPass.hpp"
#include "TextureUtilities.hpp"

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

	/* Initialise swapchain */
	env.InitialiseSwapChain({ &simpleOpaquePass, &presentPass });

	/* Set up the camera */
	Renderer::FlyingCamera camera(&env, lut::Radians(90.0f).value(), 0.01f, 1000.0f,
		env.Window().swapchainExtent.width, env.Window().swapchainExtent.height);
	camera.SetPosition(glm::vec3(0.0f, -1.0f, -15.0f));

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

	/* camera data uniform */
	Renderer::DescriptorSetLayout cameraUniformLayout(&env, { true, true, false }, Renderer::DescriptorSetType::UNIFORM_BUFFER);
	lut::Buffer cameraUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::CameraData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer::FreeUpdateBuffer(&env, &cameraUBO, 0, sizeof(Renderer::Uniforms::CameraData), camera.GetUniformDataPtr());
	Renderer::DescriptorSet cameraSet(&env, &cameraUniformLayout, *cameraUBO);

	/* sampler */
	lut::Sampler defaultSampler = Renderer::CreateDefaultSampler(env.Window());

	/* load model */
	Renderer::Model model(&env, "../res/models/simple_cube.glb", &simpleLayout, &defaultSampler);

	return 0;
}
