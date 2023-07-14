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
	class ViewerCamera
	{
		public:
			/* constructors, etc. */

			ViewerCamera(Renderer::Environment* environment, float fov_radians, float near_clip, float far_clip, uint32_t frame_width, uint32_t frame_height);
			~ViewerCamera();

			ViewerCamera(const ViewerCamera&) = delete;
			ViewerCamera& operator=(const ViewerCamera&) = delete;

		private:
			/* private member variables */

			Renderer::Environment* _epEnv = nullptr;

			float _fov = 90.0f;
			float _nearClip = 0.01f;
			float _farClip = 100.0f;
			uint32_t _frameWidth = 0;
			uint32_t _frameHeight = 0;
			float _aspect = 0.0f;

			glm::vec3 _position = glm::vec3(0);

			glm::vec3 _forward = glm::vec3(0);
			glm::vec3 _right = glm::vec3(0);
			glm::vec3 _up = glm::vec3(0);

			bool _mouseLook = false;
			bool _pressedLastFrame = false;
			float _mouseSensitivity = 0.002f;
			double _lastMouseX = 0.0f, _lastMouseY = 0.0f;
			float _yRotation = 0.0f, _xRotation = 0.0f;

			Uniforms::CameraData _data{};
			glm::mat4 _invView = glm::mat4(1);

		public:
			/* public member functions */

			void FrameUpdate(double delta_time, bool* cameraMoved = nullptr);
			void UpdateCameraSettings(float fov, uint32_t frame_width, uint32_t frame_height);
			void SetPosition(glm::vec3 new_position);
			void SetOrientation(float x, float y);
			void SetOrientation(glm::vec2 rot_xy);

			/* getters */

			Uniforms::CameraData GetUniformData();
			Uniforms::CameraData* GetUniformDataPtr();

			const glm::vec3& Position() const;
			const glm::vec3 Direction() const;
			float FOVY() const;
			float NearDist() const;
			float FarDist() const;
			float Aspect() const;

			const glm::vec3& Forward() const;
			const glm::vec3& Right() const;
			const glm::vec3& Up() const;

			const glm::mat4& InvView() const;
			const glm::mat4& InvProjView() const;

			void PrintPositionalData();
	};
}
