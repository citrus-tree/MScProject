#include "Uniforms.hpp"

/* c++ */
#include <vector>

/* Renderer */
#include "ViewerCamera.hpp" // <- class ViewerCamera

namespace Renderer
{
	namespace Uniforms
	{
		void DirectionalShadowData::Set(const ViewerCamera* camera, const LightData::DirectionalLight* sunLight, float bufferDistance)
		{
			/* first calculate the bounds of the camera frustum */

			std::vector<glm::vec3> cameraLocalPoints;
			std::vector<glm::vec3> worldPoints;
			std::vector<glm::vec3> shadowLocalPoints;

			/* near and far width and height */
			float recipAspect = camera->Aspect();
			float nearHeight = rightTriangleOpposite(camera->FOVY() / 2.0f, camera->NearDist());
			float nearWidth = recipAspect * nearHeight;
			float farHeight = rightTriangleOpposite(camera->FOVY() / 2.0f, camera->FarDist());
			float farWidth = recipAspect * farHeight;

			/* calculate the camera localPoints and midpoint */
			for (float w = -1.0f; w < 1.01f; w += 2.0f)
			{
				for (float h = -1.0f; h < 1.01f; h += 2.0f)
				{
					cameraLocalPoints.push_back(
						camera->Forward() * camera->NearDist() + 
						camera->Right() * nearWidth + 
						camera->Up() * nearHeight);

					cameraLocalPoints.push_back(
						camera->Forward() * camera->FarDist() +
						camera->Right() * farWidth +
						camera->Up() * farHeight);
				}
			}

			/* calculate the light right and up values */
			glm::vec3 lightForward = glm::normalize(sunLight->direction);
			glm::vec3 lightRight = glm::normalize(glm::cross(lightForward, glm::vec3(0.0f, 1.0f, 0.0f)));
			glm::vec3 lightUp = glm::normalize(glm::cross(lightRight, lightForward));

			/* transform the points into world space */
			for (size_t i = 0; i < cameraLocalPoints.size(); i++)
				worldPoints.push_back(glm::vec4(cameraLocalPoints[i], 1.0f) * camera->InvView());

			/* calculate the camera orientation min and max */

			float minX = std::numeric_limits<float>().max();
			float maxX = std::numeric_limits<float>().min();
			float minY = std::numeric_limits<float>().max();
			float maxY = std::numeric_limits<float>().min();
			float minZ = std::numeric_limits<float>().max();
			float maxZ = std::numeric_limits<float>().min();

			for (size_t i = 0; i < worldPoints.size(); i++)
			{
				float projX = glm::dot(worldPoints[i], lightRight);
				float projY = glm::dot(worldPoints[i], lightUp);
				float projZ = glm::dot(worldPoints[i], lightForward);

				if (projX < minX)
					minX = projX;

				if (projX > maxX)
					maxX = projX;

				if (projY < minY)
					minY = projY;

				if (projY > maxY)
					maxY = projY;

				if (projZ < minZ)
					minZ = projZ;

				if (projZ > maxZ)
					maxZ = projZ;
			}

			float lightSpanX = maxX - minX;
			float lightSpanY = maxY - minY;
			float lightSpanZ = maxZ - minZ;

			glm::vec3 lightTarget = glm::vec3(
				(minX + maxX) * 0.5f,
				(minY + maxY) * 0.5f,
				(maxZ + minZ) * 0.5f);

			glm::vec3 lightPosition = lightTarget + (lightSpanZ * 0.5f) + bufferDistance;

			/* calculate the shadow projection-view matrix */
			projection = glm::orthoRH_ZO(
				-lightSpanX * 0.5f,
				lightSpanX * 0.5f,
				-lightSpanY * 0.5f,
				lightSpanY * 0.5f,
				0.01f,
				lightSpanZ + bufferDistance);
			projection[1][1] = -1.0f;

			view = glm::lookAtRH(lightPosition, lightTarget, lightUp);

			projView = projection * view;

			/* calculate the camera to shadow transform matrix */
			cam2shadow = projView * camera->InvProjView();
		}
	}
}