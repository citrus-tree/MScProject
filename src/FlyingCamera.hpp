#pragma once

/* renderer */
#include "Uniforms.hpp"

/* glm */
#include "glm/glm.hpp"

namespace Renderer
{
	class Environment;
}

namespace Renderer
{
	class FlyingCamera
	{
		public:
			/* constructors, etc. */

			FlyingCamera(Renderer::Environment* environment, float fov_radians, float near_clip, float far_clip, uint32_t frame_width, uint32_t frame_height);
			~FlyingCamera();

			FlyingCamera(const FlyingCamera&) = delete;
			FlyingCamera& operator=(const FlyingCamera&) = delete;

		private:
			/* private member variables */

			Renderer::Environment* _epEnv = nullptr;

			float _fov = 90.0f;
			float _nearClip = 0.01f;
			float _farClip = 100.0f;
			uint32_t _frameWidth = 0;
			uint32_t _frameHeight = 0;

			glm::vec3 _position = glm::vec3(0);

			bool _mouseLook = false;
			bool _pressedLastFrame = false;
			float _mouseSensitivity = 0.002f;
			double _lastMouseX = 0.0f, _lastMouseY = 0.0f;
			float _yRotation = 0.0f, _xRotation = 0.0f;

			Uniforms::CameraData _data{};

		public:
			/* public member functions */

			void FrameUpdate(double delta_time);
			void UpdateCameraSettings(float fov, uint32_t frame_width, uint32_t frame_height);
			void SetPosition(glm::vec3 new_position);
			void SetOrientation(float x, float y);
			void SetOrientation(glm::vec2 rot_xy);

			/* getters */

			Uniforms::CameraData GetUniformData();
			Uniforms::CameraData* GetUniformDataPtr();

			glm::vec3 Position() const;
			glm::vec3 Direction() const;

			void PrintPositionalData();
	};
}
