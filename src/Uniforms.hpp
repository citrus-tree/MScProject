#pragma once

/* glm */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Renderer
{
	class ViewerCamera;
}

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
			struct DirectionalLight
			{
				glm::vec4 direction = glm::vec4(0.0f);
				glm::vec4 colour = glm::vec4(1.0f);
			};

			struct AmbientLight
			{
				glm::vec4 colour = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
			};

			DirectionalLight sunLight;
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
					glm::vec4 albedo;
					glm::vec3 emissive;
					float roughness;
					glm::vec3 transmission;
					float metallic;
					float _padding[4];
				} inner_data;

			} data;
		};

		struct DirectionalShadowData
		{
			glm::mat4 view = glm::mat4(1);
			glm::mat4 projection = glm::mat4(1);
			glm::mat4 projView = glm::mat4(1);
			glm::mat4 cam2shadow = glm::mat4(1);

			private:
				inline float rightTriangleOpposite(float angleRadians, float adjacentSide)
				{
					/* solves for the opposite side length of a right triangle given
						a non-right angle measurement and its adjacent side's length */

					float h = adjacentSide / cosf(angleRadians);
					return sqrtf(h * h - adjacentSide * adjacentSide);
				}

			public:
				void Update(const ViewerCamera* camera, const LightData::DirectionalLight* sunLight, float bufferDistance = 1000.0f, float shadowDistance = 0.0f);
		};
	}
}
