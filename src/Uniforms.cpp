#include "Uniforms.hpp"

/* c++ */
#include <vector>

/* Renderer */
#include "ViewerCamera.hpp" // <- class ViewerCamera

namespace Renderer
{
	namespace Uniforms
	{
		void DirectionalShadowData::Set(const ViewerCamera* camera, const LightData::DirectionalLight* sunLight, unsigned int bufferDistance)
		{
			{
				/* first calculate the bounds of the camera */

				std::vector<glm::vec3> _cameraLocalPoints;
				std::vector<glm::vec3> _shadowLocalPoints;

				/* near and far width and height */
				float recipAspect = 1.0f / camera->Aspect();
				float nearHeight = rightTriangleOpposite(camera->FOVY() / 2.0f, camera->NearDist());
				float nearWidth = recipAspect * nearHeight;
				float farHeight = rightTriangleOpposite(camera->FOVY() / 2.0f, camera->FarDist());
				float farWidth = recipAspect * farHeight;

				/* calculate the camera localPoints */
				for (float w = -1.0f; w < 1.01f; w += 2.0f)
				{
					for (float h = -1.0f; h < 1.01f; h += 2.0f)
					{
						_cameraLocalPoints.push_back(
							camera->Forward() * camera->NearDist() + 
							camera->Right() * nearWidth + 
							camera->Up() * nearHeight);

						_cameraLocalPoints.push_back(
							camera->Forward() * camera->FarDist() +
							camera->Right() * farWidth +
							camera->Up() * farHeight);
					}
				}

				/* calculate the shadow projection-view matrix */


				/* calculate the camera to shadow transform matrix */
				
			}
		}
	}
}