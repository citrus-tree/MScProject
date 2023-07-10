#pragma once

/* glm */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Renderer
{
	namespace Uniforms
	{
		struct CameraData
		{
			glm::mat4 view;
			glm::mat4 projection;
			glm::mat4 projView;
			glm::vec4 position;
			glm::mat4 invProjView;
		};

		struct FrameData
		{
			float width;
			float height;
			float texelWidth;
			float texelHeight;
		};

		struct ColourData
		{
			glm::vec4 col;
		};

		struct LightData
		{
			struct PointLight
			{
				glm::vec4 position = glm::vec4(0.0f);
				glm::vec4 colour = glm::vec4(1.0f);
				glm::vec4 attenuation = glm::vec4(512.0f, 0.8f, 0.12f, 0.09f); /* not strictly needed, but I like how it looks :) */
			};

			struct SpotLight
			{
				glm::vec4 position = glm::vec4(0.0f);
				glm::vec4 colour = glm::vec4(1.0f);
				glm::vec4 attenuation = glm::vec4(512.0f, 0.8f, 0.12f, 0.09f); /* not strictly needed, but I like how it looks :) */
				glm::vec4 direction_halfangle = glm::vec4(0.0f, -1.0f, 0.0f, 0.7854f);
			};

			struct AmbientLight
			{
				glm::vec4 colour = glm::vec4(0.02f, 0.02f, 0.02f, 1.0f);
			};

			PointLight pointLight;
			SpotLight spotLight;
			SpotLight spotLight1;
			SpotLight spotLight2;
			AmbientLight ambientLight;
		};

		struct SimpleMaterial
		{
			union
			{
				float big[16] = {
					1.0f, 1.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 0.0f,
					0.0f,
					0.7f,
					0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 0.0f, 0.0f
					};

				struct SimpleMaterialData
				{
					glm::vec4 abledo;
					glm::vec3 emissive;
					float roughness;
					float metallic;
					glm::vec3 transmission;
					float _padding[4];
				} inner_data;

			} data;
		};

		struct SpotlightShadowData
		{
			glm::mat4 view = glm::mat4(1);
			glm::mat4 projection = glm::mat4(1);
			glm::mat4 projView = glm::mat4(1);

			inline void Set(glm::mat4 invCamViewProj, LightData::SpotLight spotLight)
			{
				view = glm::lookAt(glm::vec3(spotLight.position),
					glm::vec3(spotLight.position) - glm::vec3(spotLight.direction_halfangle),
					glm::vec3(0.0f, 1.0f, 0.0f));
				projection = glm::perspectiveRH_ZO(
					spotLight.direction_halfangle[3],
					1.0f,
					0.1f, 1000.0f);
				projection[1][1] = -1.0f;

				projView = projection * view;
			}
		};
	}
}
