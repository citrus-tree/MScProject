
/* c++ */
#include <algorithm>

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
#include "Constants.hpp"
#include "CreationUtilities.hpp"
#include "DescriptorSets.hpp"
#include "Environment.hpp"
#include "ViewerCamera.hpp"
#include "Model.hpp"
#include "Pipeline.hpp"
#include "RenderingUtilities.hpp"
#include "RenderPass.hpp"
#include "TextureUtilities.hpp"

#define VANILLA 0
#define TRANSLUCENT_SHADOWS 0
#define SSM 0
#define CSSM 0
#define CTS 1

const float FOV = 90.0f / 180.0f * 3.1415f;
const float FarClipDist = 32.0f;
const float ShadowBufferDistance = 100.0f;

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

	Renderer::RenderPassFeatures shadowPassFeatures;
	shadowPassFeatures.colourPass = Renderer::ColourPass::DISABLED;
	shadowPassFeatures.depthTest = Renderer::DepthTest::ENABLED;
	shadowPassFeatures.renderTarget = Renderer::RenderTarget::TEXTURE_SHADOWMAP;
	Renderer::RenderPass shadowPass(env.WindowPtr(), shadowPassFeatures);

	#if TRANSLUCENT_SHADOWS or CTS
		Renderer::RenderPassFeatures TS_translucentShadowPassFeatures;
		TS_translucentShadowPassFeatures.colourPass = Renderer::ColourPass::ENABLED;
		TS_translucentShadowPassFeatures.depthTest = Renderer::DepthTest::ENABLED;
		TS_translucentShadowPassFeatures.renderTarget = Renderer::RenderTarget::TEXTURE_GEOMETRY;
		TS_translucentShadowPassFeatures.clearDepth = Renderer::ClearDepth::DISABLED;
		Renderer::RenderPass TS_translucentShadowPass(env.WindowPtr(), TS_translucentShadowPassFeatures);
	#endif

	#if CTS
		Renderer::RenderPassFeatures CTS_compositingFeatures;
		CTS_compositingFeatures.colourPass = Renderer::ColourPass::ENABLED;
		CTS_compositingFeatures.depthTest = Renderer::DepthTest::ENABLED;
		CTS_compositingFeatures.renderTarget = Renderer::RenderTarget::TEXTURE_GEOMETRY;
		CTS_compositingFeatures.clearColour = Renderer::ClearColour::DISABLED;
		Renderer::RenderPass CTS_compositingPass(env.WindowPtr(), CTS_compositingFeatures);
	#endif

	#if CSSM
		Renderer::RenderPassFeatures CSSM_shadowFeatures;
		CSSM_shadowFeatures.colourPass = Renderer::ColourPass::ENABLED;
		CSSM_shadowFeatures.depthTest = Renderer::DepthTest::ENABLED;
		CSSM_shadowFeatures.renderTarget = Renderer::RenderTarget::TEXTURE_GEOMETRY;
		CSSM_shadowFeatures.specialColour = Renderer::SpecialColour::CSSM_SHADOWMAP;
		Renderer::RenderPass CSSM_shadowPass(env.WindowPtr(), CSSM_shadowFeatures);
	#endif

	/* initialise swapchain */
	env.InitialiseSwapChain({ &simpleOpaquePass, &presentPass });

		/* samplers */
	lut::Sampler defaultSampler = Renderer::CreateDefaultSampler(env.Window());
	lut::Sampler pointSampler = Renderer::CreateDefaultSampler(env.Window(), VK_FILTER_NEAREST, VK_FILTER_NEAREST);
	lut::Sampler shadowSampler = Renderer::CreateDefaultShadowSampler(env.Window());

	/* set up the camera */
	Renderer::ViewerCamera camera(&env, FOV, 0.1f, FarClipDist,
		env.Window().swapchainExtent.width, env.Window().swapchainExtent.height);
	// camera.SetPosition(glm::vec3(0.0f, -1.0f, -15.0f));
	// camera.FrameUpdate(0.01f);

	//*
	camera.SetPosition(glm::vec3(10.090, -7.994, 5.043));
	camera.SetOrientation(glm::vec2(274.000, 1083.000));
	camera.FrameUpdate(0.01f);//*/

	/*
	camera.SetPosition(glm::vec3(3.976, -2.095, -0.002));
	camera.SetOrientation(glm::vec2(790.000, 786.000));
	camera.FrameUpdate(0.01f);//*/

	/*
	camera.SetPosition(glm::vec3(4.869, -2.837, 0.344));
	camera.SetOrientation(glm::vec2(368.000, 907.000));
	camera.FrameUpdate(0.01f);//*/

	/* camera data uniform */
	Renderer::DescriptorSetLayout cameraUniformLayout(&env, { true, true, false }, Renderer::DescriptorSetType::UNIFORM_BUFFER);
	lut::Buffer cameraUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::CameraData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer::FreeUpdateBuffer(&env, &cameraUBO, 0, sizeof(Renderer::Uniforms::CameraData), camera.GetUniformDataPtr());
	Renderer::DescriptorSet cameraSet(&env, &cameraUniformLayout, *cameraUBO);

	/* set lighting parameters */
	Renderer::Uniforms::LightData lights;
	lights.sunLight.direction = glm::normalize(glm::vec4(1.0f, -1.0f, 0.0f, 0.0f));
	// lights.sunLight.direction = glm::normalize(glm::vec4(1.0f, -0.5f, 0.0f, 0.0f));
	lights.sunLight.colour = glm::normalize(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	/* lighting uniform */
	Renderer::DescriptorSetLayout lightingUniformLayout(&env, Renderer::ShaderStageConstants::FRAGMENT_STAGE, Renderer::DescriptorSetType::UNIFORM_BUFFER);
	lut::Buffer lightingUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::LightData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
	Renderer::FreeUpdateBuffer(&env, &lightingUBO, 0, sizeof(Renderer::Uniforms::LightData), &lights);
	Renderer::DescriptorSet lightingSet(&env, &lightingUniformLayout, *lightingUBO);

	/* shadow data, uniform, and descriptor sets */
	Renderer::Uniforms::DirectionalShadowData shadowData;
	shadowData.Update(&camera, &lights.sunLight, ShadowBufferDistance);

	/* create image buffers for the shadow maps */
	uint32_t shadowMapIndex = env.CreateSideBuffers(&shadowPass, 1, Renderer::Environment::SideBufferType::DEPTH);
	lut::ImageView* shadowMapView = &(*env.GetSideBufferImageView(shadowMapIndex))[0];

	lut::Buffer shadowMapProjUBO = lut::create_buffer(env.Allocator(), sizeof(Renderer::Uniforms::DirectionalShadowData),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
		Renderer::FreeUpdateBuffer(&env, &shadowMapProjUBO, 0, sizeof(Renderer::Uniforms::DirectionalShadowData), &shadowData);

	Renderer::DescriptorSetLayout shadowMapProjSetLayout(&env, { true, false, false }, Renderer::DescriptorSetType::UNIFORM_BUFFER);

	Renderer::DescriptorSetLayoutFeatures shadowMapSetFeatures;
	shadowMapSetFeatures.stages.fragment = true;
	shadowMapSetFeatures.bindingCount = 2;
	Renderer::DescriptorSetType shadowMapSetTypes[2]
	{
		Renderer::DescriptorSetType::SAMPLER, /* shadowmap texture */
		Renderer::DescriptorSetType::UNIFORM_BUFFER /* shadowmap transform data */
	};
	shadowMapSetFeatures.pBindingTypes = shadowMapSetTypes;
	Renderer::DescriptorSetLayout shadowMapLayout(&env, shadowMapSetFeatures);

	Renderer::DescriptorSet shadowMapProjSet(&env, &shadowMapProjSetLayout, *shadowMapProjUBO);
	std::vector<Renderer::DescriptorSetFeatures> bindingData{};
	bindingData.resize(2);

	bindingData[0].binding = 0;
	bindingData[0].s_View = **shadowMapView;
	bindingData[0].s_Sampler = *shadowSampler;

	bindingData[1].binding = 1;
	bindingData[1].u_Buffer = *shadowMapProjUBO;
	Renderer::DescriptorSet shadowMapSet(&env, &shadowMapLayout, 2, bindingData.data());

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

	/* load model */
	Renderer::Model model(&env, "../res/models/teapot scene.glb", &simpleLayout, &defaultSampler);
	model.SortTransparentGeometry(-lights.sunLight.direction * 9999.9f, camera.Position());

	/* Pipelines and Dependencies */
	std::vector<const VkDescriptorSetLayout*> simpleLayouts = { &*cameraUniformLayout, &*simpleLayout, &*lightingUniformLayout, &*shadowMapLayout };
	Renderer::Pipeline simplePipeline(&env, Renderer::Pipeline_Default, &simpleOpaquePass, simpleLayouts);
	
	Renderer::PipelineFeatures transparentFeatures = Renderer::Pipeline_Default;
	transparentFeatures.alphaBlend = Renderer::AlphaBlend::ENABLED;
	transparentFeatures.depthTest = Renderer::DepthTest::ENABLED;
	transparentFeatures.depthWrite = Renderer::DepthWrite::DISABLED;
	Renderer::Pipeline transparentPipeline(&env, transparentFeatures, &simpleOpaquePass, simpleLayouts);

	std::vector<const VkDescriptorSetLayout*> postProcessingLayouts = { &*singleTextureLayout };
	Renderer::PipelineFeatures postPresentFeatures;
	postPresentFeatures.alphaBlend = Renderer::AlphaBlend::DISABLED;
	postPresentFeatures.fillMode = Renderer::FillMode::FILL;
	postPresentFeatures.specialMode = Renderer::SpecialMode::SCREEN_QUAD_PRESENT;
	Renderer::Pipeline postPresentPipeline(&env, postPresentFeatures, &presentPass, postProcessingLayouts);

	std::vector<const VkDescriptorSetLayout*> shadowLayouts = { &*shadowMapProjSetLayout };
	Renderer::PipelineFeatures shadowPipelineFeatures;
	shadowPipelineFeatures.alphaBlend = Renderer::AlphaBlend::DISABLED;
	shadowPipelineFeatures.fillMode = Renderer::FillMode::FILL;
	shadowPipelineFeatures.specialMode = Renderer::SpecialMode::SHADOW_MAP;
	Renderer::Pipeline shadowPipeline(&env, shadowPipelineFeatures, &shadowPass, shadowLayouts);

	#if TRANSLUCENT_SHADOWS or CTS /* TRANSLUCENT SHADOWS IMPLEMENTATION: START UP TASKS */
		/* TRANSLUCENT SHADOWS: extra buffers for translucent shadows */
		uint32_t TS_translucentDepthMapIndex = env.CreateSideBuffers(&shadowPass, 1, Renderer::Environment::SideBufferType::DEPTH,
			false, nullptr, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		Renderer::SideBufferShareData TS_translucentShareData;
		TS_translucentShareData.depthIndex = shadowMapIndex;
		TS_translucentShareData.depthSubindex = 0;
		uint32_t TS_translucentShadowMapIndex = env.CreateSideBuffers(&TS_translucentShadowPass, 1, Renderer::Environment::SideBufferType::COMBINED,
			true, &TS_translucentShareData, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION);
		lut::ImageView* TS_translucentDepthMapView = &(*env.GetSideBufferImageView(TS_translucentDepthMapIndex))[0];
		lut::ImageView* TS_translucentShadowMapView = &(*env.GetSideBufferImageView(TS_translucentShadowMapIndex))[0];

		/* TRANSLUCENT SHADOWS: shadowmap set needs two depth textures and a colour texture */

		Renderer::DescriptorSetLayoutFeatures TS_shadowMapSetFeatures;
		TS_shadowMapSetFeatures.stages.fragment = true;
		TS_shadowMapSetFeatures.bindingCount = 4;
		Renderer::DescriptorSetType TS_shadowMapSetTypes[4]
		{
			Renderer::DescriptorSetType::SAMPLER, /* opaque shadowmap texture */
			Renderer::DescriptorSetType::SAMPLER, /* transparent shadowmap texture */
			Renderer::DescriptorSetType::SAMPLER, /* shadowmap texture */
			Renderer::DescriptorSetType::UNIFORM_BUFFER /* shadowmap transform data */
		};
		TS_shadowMapSetFeatures.pBindingTypes = TS_shadowMapSetTypes;
		Renderer::DescriptorSetLayout TS_shadowMapLayout(&env, TS_shadowMapSetFeatures);

		std::vector<Renderer::DescriptorSetFeatures> TS_shadowBindingData{};
		TS_shadowBindingData.resize(4);

		TS_shadowBindingData[0].binding = 0;
		TS_shadowBindingData[0].s_View = **shadowMapView;
		TS_shadowBindingData[0].s_Sampler = *shadowSampler;

		TS_shadowBindingData[1].binding = 1;
		TS_shadowBindingData[1].s_View = **TS_translucentDepthMapView;
		TS_shadowBindingData[1].s_Sampler = *shadowSampler;

		TS_shadowBindingData[2].binding = 2;
		TS_shadowBindingData[2].s_View = **TS_translucentShadowMapView;
		TS_shadowBindingData[2].s_Sampler = *shadowSampler;

		TS_shadowBindingData[3].binding = 3;
		TS_shadowBindingData[3].u_Buffer = *shadowMapProjUBO;

		Renderer::DescriptorSet TS_shadowMapSet(&env, &TS_shadowMapLayout, 4, TS_shadowBindingData.data());

		/* TRANSLUCENT SHADOWS: Pipelines */

		std::vector<const VkDescriptorSetLayout*> TS_simpleLayouts = { &*cameraUniformLayout, &*simpleLayout, &*lightingUniformLayout, &*TS_shadowMapLayout };
		Renderer::PipelineFeatures TS_geometryFeatures = Renderer::Pipeline_Default;
		TS_geometryFeatures.specialMode = Renderer::SpecialMode::TS_GEOMETRY;
		Renderer::Pipeline TS_geometryPipeline(&env, TS_geometryFeatures, &simpleOpaquePass, TS_simpleLayouts);

		Renderer::PipelineFeatures TS_transparentGeometryFeatures = TS_geometryFeatures;
		TS_transparentGeometryFeatures.alphaBlend = Renderer::AlphaBlend::ENABLED;
		TS_transparentGeometryFeatures.depthTest = Renderer::DepthTest::ENABLED;
		TS_transparentGeometryFeatures.depthWrite = Renderer::DepthWrite::DISABLED;
		Renderer::Pipeline TS_transparentGeometryPipeline(&env, TS_transparentGeometryFeatures, &simpleOpaquePass, TS_simpleLayouts);

		std::vector<const VkDescriptorSetLayout*> TS_transparentPipelineLayouts = { &*shadowMapProjSetLayout, &*simpleLayout };
		Renderer::PipelineFeatures TS_transparentFeatures = Renderer::Pipeline_Default;
		TS_transparentFeatures.alphaBlend = Renderer::AlphaBlend::ENABLED;
		TS_transparentFeatures.depthTest = Renderer::DepthTest::ENABLED;
		TS_transparentFeatures.depthWrite = Renderer::DepthWrite::DISABLED;
		TS_transparentFeatures.specialMode = Renderer::SpecialMode::TS_COLOURED_SHADOW_MAP;
		Renderer::Pipeline TS_transparentPipeline(&env, TS_transparentFeatures, &simpleOpaquePass, TS_transparentPipelineLayouts);
	#endif

	#if SSM or CSSM

		/* Both stochastic approaches utilise a noise texture */

		lut::Image noiseImage = lut::load_image_texture2d("../res/images/rgb_noise_2048.png",
			env.Window(), *env.CommandPool(), env.Allocator(), VK_FORMAT_R8G8B8A8_UNORM);
		lut::ImageView noiseView = Renderer::CreateImageView(&env, *noiseImage, VK_FORMAT_R8G8B8A8_UNORM);

		Renderer::DescriptorSet noiseTextureSet(&env, &singleTextureLayout, *noiseView, *pointSampler);
	#endif

	#if SSM

		std::vector<const VkDescriptorSetLayout*> SSM_shadowLayouts = { &*shadowMapProjSetLayout, &*simpleLayout, &*singleTextureLayout };
		Renderer::PipelineFeatures SSM_shadowPipelineFeatures;
		SSM_shadowPipelineFeatures.alphaBlend = Renderer::AlphaBlend::DISABLED;
		SSM_shadowPipelineFeatures.fillMode = Renderer::FillMode::FILL;
		SSM_shadowPipelineFeatures.specialMode = Renderer::SpecialMode::SSM_STOCHASTIC_SHADOW_MAP;
		Renderer::Pipeline SSM_shadowPipeline(&env, SSM_shadowPipelineFeatures, &shadowPass, SSM_shadowLayouts);

		Renderer::PipelineFeatures SMM_defaultFeatures = Renderer::Pipeline_Default;
		SMM_defaultFeatures.specialMode = Renderer::SpecialMode::SSM_DEFAULT_BIG_PCF;
		Renderer::Pipeline SSM_defaultPipeline(&env, SMM_defaultFeatures, &simpleOpaquePass, simpleLayouts);

		Renderer::PipelineFeatures SSM_transparentFeatures = transparentFeatures;
		SSM_transparentFeatures.specialMode = Renderer::SpecialMode::SSM_DEFAULT_BIG_PCF;
		Renderer::Pipeline SSM_transparentPipeline(&env, SSM_transparentFeatures, &simpleOpaquePass, simpleLayouts);
	#endif

	#if CSSM

		/* Buffers */

		uint32_t CSSM_shadowMapIndex = env.CreateSideBuffers(
			&CSSM_shadowPass, 1, Renderer::Environment::SideBufferType::COMBINED,
			false, nullptr, SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, true);
		lut::ImageView* CSSM_shadowMapView = &(*env.GetSideBufferImageView(CSSM_shadowMapIndex))[0];
		lut::ImageView* CSSM_depthMapView = &(*env.GetSideBufferImageView(CSSM_shadowMapIndex))[1];

		/* Descriptor Sets */

		Renderer::DescriptorSetLayoutFeatures CSSM_shadowMapSetFeatures;
		CSSM_shadowMapSetFeatures.stages.fragment = true;
		CSSM_shadowMapSetFeatures.bindingCount = 3;
		Renderer::DescriptorSetType CSSM_shadowMapSetTypes[3]
		{
			Renderer::DescriptorSetType::SAMPLER, /* shadowmap texture */
			Renderer::DescriptorSetType::SAMPLER, /* shadow colour texture */
			Renderer::DescriptorSetType::UNIFORM_BUFFER /* shadow transform data */
		};
		CSSM_shadowMapSetFeatures.pBindingTypes = CSSM_shadowMapSetTypes;
		Renderer::DescriptorSetLayout CSSM_shadowMapLayout(&env, CSSM_shadowMapSetFeatures);

		std::vector<Renderer::DescriptorSetFeatures> CSSM_shadowBindingData{};
		CSSM_shadowBindingData.resize(3);

		CSSM_shadowBindingData[0].binding = 0;
		CSSM_shadowBindingData[0].s_View = **CSSM_depthMapView;
		CSSM_shadowBindingData[0].s_Sampler = *shadowSampler;

		CSSM_shadowBindingData[1].binding = 1;
		CSSM_shadowBindingData[1].s_View = **CSSM_shadowMapView;
		CSSM_shadowBindingData[1].s_Sampler = *pointSampler;

		CSSM_shadowBindingData[2].binding = 2;
		CSSM_shadowBindingData[2].u_Buffer = *shadowMapProjUBO;

		Renderer::DescriptorSet CSSM_shadowMapSet(&env, &CSSM_shadowMapLayout, 3, CSSM_shadowBindingData.data());

		/* Pipelines */

		std::vector<const VkDescriptorSetLayout*> CSSM_shadowLayouts = { &*shadowMapProjSetLayout, &*simpleLayout, &*singleTextureLayout };
		Renderer::PipelineFeatures CSSM_shadowPipelineFeatures;
		CSSM_shadowPipelineFeatures.alphaBlend = Renderer::AlphaBlend::DISABLED;
		CSSM_shadowPipelineFeatures.fillMode = Renderer::FillMode::FILL;
		CSSM_shadowPipelineFeatures.specialMode = Renderer::SpecialMode::CSSM_COLORED_STOCHASTIC_SHADOW_MAP;
		CSSM_shadowPipelineFeatures.depthWrite = Renderer::DepthWrite::ENABLED;
		Renderer::Pipeline CSSM_shadowOpaquePipeline(&env, CSSM_shadowPipelineFeatures, &CSSM_shadowPass, CSSM_shadowLayouts);

		CSSM_shadowPipelineFeatures.specialMode = Renderer::SpecialMode::CSSM_COLORED_STOCHASTIC_SHADOW_MAP_2;
		CSSM_shadowPipelineFeatures.depthWrite = Renderer::DepthWrite::DISABLED;
		CSSM_shadowPipelineFeatures.blendMode = Renderer::BlendMode::MIN_ONE_ONE;
		Renderer::Pipeline CSSM_shadowTransparentPipeline(&env, CSSM_shadowPipelineFeatures, &CSSM_shadowPass, CSSM_shadowLayouts);

		std::vector<const VkDescriptorSetLayout*> CSSM_layouts = { &*cameraUniformLayout, &*simpleLayout, &*lightingUniformLayout, &*CSSM_shadowMapLayout };
		Renderer::PipelineFeatures CSMM_defaultFeatures = Renderer::Pipeline_Default;
		CSMM_defaultFeatures.specialMode = Renderer::SpecialMode::CSSM_DEFAULT;
		Renderer::Pipeline CSSM_defaultPipeline(&env, CSMM_defaultFeatures, &simpleOpaquePass, CSSM_layouts);

		Renderer::PipelineFeatures CSSM_transparentFeatures = transparentFeatures;
		CSSM_transparentFeatures.specialMode = Renderer::SpecialMode::CSSM_DEFAULT;
		Renderer::Pipeline CSSM_transparentPipeline(&env, CSSM_transparentFeatures, &simpleOpaquePass, CSSM_layouts);
	#endif

	#if CTS
		Renderer::PipelineFeatures CTS_transparentGeometryFeatures = TS_geometryFeatures;
		CTS_transparentGeometryFeatures.alphaBlend = Renderer::AlphaBlend::ENABLED;
		CTS_transparentGeometryFeatures.depthTest = Renderer::DepthTest::ENABLED;
		CTS_transparentGeometryFeatures.depthWrite = Renderer::DepthWrite::DISABLED;
		Renderer::Pipeline CTS_transparentGeometryPipeline(&env, CTS_transparentGeometryFeatures, &CTS_compositingPass, TS_simpleLayouts);
	#endif

	/* Main loop */
	double time = glfwGetTime();
	bool firstFrame = true;
	bool printOutLastFrame = false;
	uint32_t frameNumber = 0;
	while (glfwWindowShouldClose(env.Window().window) == false)
	{
		/* Window polling */
		glfwPollEvents();

		/* print camera position and direction */
		if (glfwGetKey(env.Window().window, GLFW_KEY_P) == GLFW_PRESS)
		{
			if (printOutLastFrame == false)
			{
				camera.PrintPositionalData();
			}

			printOutLastFrame = true;
		}
		else
		{
			printOutLastFrame = false;
		}

		/* Recreate the swap chain if it's been invalidated.
			This also necessitates adjusting the pipelines,
			since the window has likely changed size. */
		if (env.CheckSwapChain({ &simpleOpaquePass, &presentPass }) != ErrorCode::SUCCESS)
		{
			simplePipeline.Repair(&env);
			transparentPipeline.Repair(&env);
			postPresentPipeline.Repair(&env);

			#if TRANSLUCENT_SHADOWS or CTS
				TS_geometryPipeline.Repair(&env);
				TS_transparentGeometryPipeline.Repair(&env);
			#endif


			#if CTS
				CTS_transparentGeometryPipeline.Repair(&env);
			#endif

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
		bool moved = false;
		camera.FrameUpdate(timeDelta, &moved);

		if (moved)
		{
			/* update shadow data */

			shadowData.Update(&camera, &lights.sunLight, ShadowBufferDistance);
			model.SortTransparentGeometry(-lights.sunLight.direction * 9999.9f, camera.Position(), false, true);
		}

		/* Prepare to queue commands */
		env.BeginFrameCommands();

		/* Update the camera and lighting data buffers */
		Renderer::CmdUpdateBuffer(&env, &cameraUBO, 0, sizeof(Renderer::Uniforms::CameraData), camera.GetUniformDataPtr());
		Renderer::CmdUpdateBuffer(&env, &lightingUBO, 0, sizeof(Renderer::Uniforms::LightData), &lights);
		Renderer::CmdUpdateBuffer(&env, &shadowMapProjUBO, 0, sizeof(Renderer::Uniforms::DirectionalShadowData), &shadowData);

		/* Set the shadow map texture(s) for writing */
		if (firstFrame)
		{
			firstFrame = false;
			Renderer::CmdPrimeImageForRead(&env, &(*env.GetSideBufferImage(shadowMapIndex))[0], true);

			#if TRANSLUCENT_SHADOWS or CTS
				Renderer::CmdPrimeImageForRead(&env, &(*env.GetSideBufferImage(TS_translucentDepthMapIndex))[0], true);
				Renderer::CmdPrimeImageForRead(&env, &(*env.GetSideBufferImage(TS_translucentShadowMapIndex))[0], false);
			#endif

			#if CSSM
				Renderer::CmdPrimeImageForRead(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[0], false);
				Renderer::CmdPrimeImageForRead(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[1], true);
			#endif
		}
		#if not CSSM
			Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(shadowMapIndex))[0], true);
		#endif

		#if TRANSLUCENT_SHADOWS or CTS
			Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(TS_translucentDepthMapIndex))[0], true);
			Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(TS_translucentShadowMapIndex))[0], false);
		#endif


		#if VANILLA or TRANSLUCENT_SHADOWS or SSM or CTS
			/* Begin shadow map pass */
			env.BeginRenderPass(&shadowPass, shadowMapIndex); /* rendering to the opaque shadow map */

			{
				#if VANILLA or TRANSLUCENT_SHADOWS or CTS
					/* opaque meshes */
					shadowPipeline.CmdBind(&env);
					shadowMapProjSet.CmdBind(&env, &shadowPipeline, 0);
					model.CmdDrawOpaque_DepthOnly(&env, &shadowPipeline);
				#endif

				#if SSM
					/* all meshes */
					SSM_shadowPipeline.CmdBind(&env);
					shadowMapProjSet.CmdBind(&env, &SSM_shadowPipeline, 0);
					noiseTextureSet.CmdBind(&env, &SSM_shadowPipeline, 2);
					model.CmdDrawOpaque(&env, &SSM_shadowPipeline);
					model.CmdDrawTransparent(&env, &SSM_shadowPipeline);
				#endif
			}

			/* End render pass */
			env.EndRenderPass();
		#endif

		#if CSSM
			/* transition the cssm textures for writing */
			Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[0], false);
			Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[1], true);

			/* Begin cssm render pass */
			env.BeginRenderPass(&CSSM_shadowPass, CSSM_shadowMapIndex,
				SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION); /* rendering to the colored stochastic shadow map */

			{
				/* all meshes */
				CSSM_shadowOpaquePipeline.CmdBind(&env);
				shadowMapProjSet.CmdBind(&env, &CSSM_shadowOpaquePipeline, 0);
				noiseTextureSet.CmdBind(&env, &CSSM_shadowOpaquePipeline, 2);
				model.CmdDrawOpaque(&env, &CSSM_shadowOpaquePipeline);

				CSSM_shadowTransparentPipeline.CmdBind(&env);
				shadowMapProjSet.CmdBind(&env, &CSSM_shadowTransparentPipeline, 0);
				noiseTextureSet.CmdBind(&env, &CSSM_shadowTransparentPipeline, 2);
				model.CmdDrawTransparent(&env, &CSSM_shadowTransparentPipeline);
			}

			/* End render pass */
			env.EndRenderPass();

			/* transition the cssm textures for reading */
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[0], false);
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(CSSM_shadowMapIndex))[1], true);
		#endif

		/* Set the opaque shadow map texture for reading */
		#if VANILLA or SSM
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(shadowMapIndex))[0], true);
		#endif

		#if TRANSLUCENT_SHADOWS or CTS
			/* TRANSLUCENT SHADOWS: Begin translucent shadow map pass */
			env.BeginRenderPass(&shadowPass, TS_translucentDepthMapIndex); /* rendering to translucent shadow depth map */

			{
				/* transparent meshes
					this pass records the transparent surface closest to the camera */
				shadowPipeline.CmdBind(&env);
				shadowMapProjSet.CmdBind(&env, &shadowPipeline, 0);
				model.CmdDrawTransparentLightFrontToBack_DepthOnly(&env, &shadowPipeline, true);
			}

			/* TRANSLUCENT SHADOWS: End render pass */
			env.EndRenderPass();

			/* TRANSLUCENT SHADOWS: Set the opaque shadow map texture for reading */
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(TS_translucentDepthMapIndex))[0], true);

			/* TRANSLUCENT SHADOWS: Begin translucent shadow colour pass */
			env.BeginRenderPass(&TS_translucentShadowPass, TS_translucentShadowMapIndex,
				SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION); /* rendering to translucent shadow colour map */

			{
				/* this pass accumulates the colours of transparent geometry visible to the light,
					so the final translucent shadow colour can be determined. */
				TS_transparentPipeline.CmdBind(&env);
				shadowMapProjSet.CmdBind(&env, &TS_transparentPipeline, 0);
				model.CmdDrawTransparentLightFrontToBack(&env, &TS_transparentPipeline);
			}

			/* TRANSLUCENT SHADOWS: End render pass */
			env.EndRenderPass();

			/* TRANSLUCENT SHADOWS: Set the translucent shadow colour texture for reading */
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(TS_translucentShadowMapIndex))[0], false);

			/* Set the opaque shadow map texture for reading */
			Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(shadowMapIndex))[0], true);
		#endif

		/* Begin geometry pass */
		env.BeginRenderPass(&simpleOpaquePass); /* rendering to intermediate 0 */

		{
			/* Draw meshes */

			#if VANILLA
				/* opaque geometry */
				simplePipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &simplePipeline, 0);
				lightingSet.CmdBind(&env, &simplePipeline, 2);
				shadowMapSet.CmdBind(&env, &simplePipeline, 3);
				model.CmdDrawOpaque(&env, &simplePipeline);

				/* transparent geometry */
				transparentPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &transparentPipeline, 0);
				lightingSet.CmdBind(&env, &transparentPipeline, 2);
				shadowMapSet.CmdBind(&env, &transparentPipeline, 3);
				model.CmdDrawTransparentCameraBackToFront(&env, &transparentPipeline);
			#endif

			#if TRANSLUCENT_SHADOWS or CTS
				/* opaque geometry */
				TS_geometryPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &TS_geometryPipeline, 0);
				lightingSet.CmdBind(&env, &TS_geometryPipeline, 2);
				TS_shadowMapSet.CmdBind(&env, &TS_geometryPipeline, 3);
				model.CmdDrawOpaque(&env, &TS_geometryPipeline);

				#if TRANSLUCENT_SHADOWS
					/* transparent geometry */
					TS_transparentGeometryPipeline.CmdBind(&env);
					cameraSet.CmdBind(&env, &TS_transparentGeometryPipeline, 0);
					lightingSet.CmdBind(&env, &TS_transparentGeometryPipeline, 2);
					TS_shadowMapSet.CmdBind(&env, &TS_transparentGeometryPipeline, 3);
					model.CmdDrawTransparentCameraBackToFront(&env, &TS_transparentGeometryPipeline);
				#endif
			#endif

			#if SSM
				/* opaque geometry */
				SSM_defaultPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &SSM_defaultPipeline, 0);
				lightingSet.CmdBind(&env, &SSM_defaultPipeline, 2);
				shadowMapSet.CmdBind(&env, &SSM_defaultPipeline, 3);
				model.CmdDrawOpaque(&env, &SSM_defaultPipeline);

				/* transparent geometry */
				SSM_transparentPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &SSM_transparentPipeline, 0);
				lightingSet.CmdBind(&env, &SSM_transparentPipeline, 2);
				shadowMapSet.CmdBind(&env, &SSM_transparentPipeline, 3);
				model.CmdDrawTransparentCameraBackToFront(&env, &SSM_transparentPipeline);
			#endif

			#if CSSM
				/* opaque geometry */
				CSSM_defaultPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &CSSM_defaultPipeline, 0);
				lightingSet.CmdBind(&env, &CSSM_defaultPipeline, 2);
				CSSM_shadowMapSet.CmdBind(&env, &CSSM_defaultPipeline, 3);
				model.CmdDrawOpaque(&env, &CSSM_defaultPipeline);

				/* transparent geometry */
				CSSM_transparentPipeline.CmdBind(&env);
				cameraSet.CmdBind(&env, &CSSM_transparentPipeline, 0);
				lightingSet.CmdBind(&env, &CSSM_transparentPipeline, 2);
				CSSM_shadowMapSet.CmdBind(&env, &CSSM_transparentPipeline, 3);
				model.CmdDrawTransparentCameraBackToFront(&env, &CSSM_transparentPipeline);
			#endif
		}

		/* End render pass */
		env.EndRenderPass();

	#if not CTS
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

		#else
			/* Render the depth peeled layers */
			for (uint32_t i = 0; i < model.TransparentMeshCount(); i++)
			{
				uint32_t currentMesh = model.TransparentMeshesSortedFarthestFromCamera()[i];
				uint32_t lightFarIndex = std::find(
					model.TransparentMeshesSortedClosestToLight().begin(),
					model.TransparentMeshesSortedClosestToLight().end(),
					static_cast<int>(currentMesh)) -
					model.TransparentMeshesSortedClosestToLight().begin();

				Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(TS_translucentDepthMapIndex))[0], true);
				Renderer::CmdTransitionForWrite(&env, &(*env.GetSideBufferImage(TS_translucentShadowMapIndex))[0], false);

				env.BeginRenderPass(&TS_translucentShadowPass, TS_translucentShadowMapIndex,
					SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION); /* rendering to translucent shadow colour map */

				{
					/* this pass accumulates the colours of transparent geometry visible to the light,
						so the final translucent shadow colour can be determined. */
					TS_transparentPipeline.CmdBind(&env);
					shadowMapProjSet.CmdBind(&env, &TS_transparentPipeline, 0);
					model.CmdDrawTransparentLightFrontToBack(&env, &TS_transparentPipeline, 0, lightFarIndex);
				}

				env.EndRenderPass();

				Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(shadowMapIndex))[0], true);
				Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(TS_translucentDepthMapIndex))[0], true);
				Renderer::CmdTransitionForRead(&env, &(*env.GetSideBufferImage(TS_translucentShadowMapIndex))[0], false);

				/* Begin geometry pass */
				env.BeginRenderPass(&CTS_compositingPass); /* rendering to intermediate 0 */

				{
					/* draw transparent geometry */
					CTS_transparentGeometryPipeline.CmdBind(&env);
					cameraSet.CmdBind(&env, &CTS_transparentGeometryPipeline, 0);
					lightingSet.CmdBind(&env, &CTS_transparentGeometryPipeline, 2);
					TS_shadowMapSet.CmdBind(&env, &CTS_transparentGeometryPipeline, 3);
					model.CmdDrawTransparentCameraBackToFront(&env, &CTS_transparentGeometryPipeline, i, i + 1);
				}

				/* End render pass */
				env.EndRenderPass();
			}

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
	#endif

		/* Submitted queued commands */
		env.EndFrameCommands();

		/* Present the frame */
		env.Present();

		/* Increment the frame count */
		frameNumber++;
	}

	/* Wait for the GPU to finishing doing what it's doing. */
	vkDeviceWaitIdle(env.Window().device);

	return 0;
}
