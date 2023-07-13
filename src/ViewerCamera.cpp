#include "ViewerCamera.hpp"

/* c */
#include <cstdio>

/* glfw */
#include <GLFW/glfw3.h>

/* glm */
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

/* renderer */ 
#include "Environment.hpp" // <- class Environment

/* labutils */
#include "../labutils/angle.hpp"

namespace Renderer
{
	ViewerCamera::ViewerCamera(Renderer::Environment* environment, float fov_radians,
		float near_clip, float far_clip,
		uint32_t frame_width, uint32_t frame_height)
		: _epEnv(environment), _fov(fov_radians),
			_nearClip(near_clip), _farClip(far_clip),
			_frameWidth(frame_width), _frameHeight(frame_height),
			_aspect(static_cast<float>(frame_width) / static_cast<float>(frame_height))
	{}

	ViewerCamera::~ViewerCamera()
	{}

	void ViewerCamera::FrameUpdate(double delta_time)
	{
		/* not ideal, but optimisation will come in a later pass of the project
			(if time permits). */
		glm::vec3 trans = glm::vec3(0);
		float speed = 2.5f;

		/* movement axis */
		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_W) == GLFW_PRESS)
			trans.z += 1.0f;

		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_S) == GLFW_PRESS)
			trans.z -= 1.0f;

		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_A) == GLFW_PRESS)
			trans.x += 1.0f;

		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_D) == GLFW_PRESS)
			trans.x -= 1.0f;

		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_Q) == GLFW_PRESS)
			trans.y += 1.0f;

		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_E) == GLFW_PRESS)
			trans.y -= 1.0f;

		/* speed control */
		if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			speed = 10.0f;
		else if (glfwGetKey(_epEnv->Window().window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			speed = 0.5f;

		/* is mouse look enabled? */
		bool isPressed = (glfwGetMouseButton(_epEnv->Window().window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
		if (isPressed == true && _pressedLastFrame == false)
			_mouseLook = !_mouseLook;
		_pressedLastFrame = isPressed;

		/* get mouse motion */
		double _newMouseX = 0.0f, _newMouseY = 0.0f;
		glfwGetCursorPos(_epEnv->Window().window, &_newMouseX, &_newMouseY);
		if (_mouseLook)
		{
			_yRotation += static_cast<float>(_newMouseX - _lastMouseX);
			_xRotation += static_cast<float>(_newMouseY - _lastMouseY);
			glfwSetInputMode(_epEnv->Window().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else
		{
			glfwSetInputMode(_epEnv->Window().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		_lastMouseX = _newMouseX;
		_lastMouseY = _newMouseY;

		/* generate rotation matrix */
		glm::mat4 rotation = glm::mat4(1);
		rotation = glm::rotate(rotation, static_cast<float>(_xRotation * _mouseSensitivity), glm::vec3(1, 0, 0));
		rotation = glm::rotate(rotation, static_cast<float>(_yRotation * _mouseSensitivity), glm::vec3(0, 1, 0));

		/* normalise translation then apply it */
		if (glm::length(trans) > 0.0f)
		{
			trans = glm::normalize(trans);
			_position += glm::vec3(glm::vec4((trans * (speed * static_cast<float>(delta_time))), 1.0f) * rotation);
		}

		/* update data struct */
		_data.projection = glm::perspectiveRH_ZO(
			_fov,
			_aspect,
			_nearClip, _farClip);
		_data.projection[1][1] = -1.0f;

		_data.view = rotation * glm::translate(_position);

		_data.projView = _data.projection * _data.view;
		_data.invProjView = glm::inverse(_data.projView);

		_data.position = glm::vec4(_position, 1.0f);

		_invView = glm::inverse(_data.view);

		/* calculate forward, right, and up vectors */
		_forward = glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		_right = glm::cross(_forward, glm::vec3(0.0f, 1.0f, 0.0f));
		_up = glm::cross(_right, _forward);
	}

	void ViewerCamera::UpdateCameraSettings(float fov, uint32_t frame_width, uint32_t frame_height)
	{
		_fov = fov;
		_frameWidth = frame_width;
		_frameHeight = frame_height;
		_aspect = static_cast<float>(frame_width) / static_cast<float>(frame_height);
	}

	void ViewerCamera::SetPosition(glm::vec3 new_position)
	{
		_position = new_position;
	}

	void ViewerCamera::SetOrientation(float x, float y)
	{
		_xRotation = x;
		_yRotation = y;
	}

	void ViewerCamera::SetOrientation(glm::vec2 rot_xy)
	{
		SetOrientation(rot_xy.x, rot_xy.y);
	}

	/* getters */

	Uniforms::CameraData ViewerCamera::GetUniformData()
	{
		return _data;
	}
	Uniforms::CameraData* ViewerCamera::GetUniformDataPtr()
	{
		return &_data;
	}

	const glm::vec3& ViewerCamera::Position() const
	{
		return _position;
	}
	const glm::vec3 ViewerCamera::Direction() const
	{
		glm::mat4 rotation = glm::mat4(1);
		rotation = glm::rotate(rotation, static_cast<float>(_xRotation * _mouseSensitivity), glm::vec3(1, 0, 0));
		rotation = glm::rotate(rotation, static_cast<float>(_yRotation * _mouseSensitivity), glm::vec3(0, 1, 0));

		return normalize(glm::vec4(0.0f, 0.0f, 1.0f, 0.0f) * rotation);
	}
	float ViewerCamera::FOVY() const
	{
		return _fov;
	}
	float ViewerCamera::NearDist() const
	{
		return _nearClip;
	}
	float ViewerCamera::FarDist() const
	{
		return _farClip;
	}

	float ViewerCamera::Aspect() const
	{
		return _aspect;
	}

	const glm::vec3& ViewerCamera::Forward() const
	{
		return _forward;
	}

	const glm::vec3& ViewerCamera::Right() const
	{
		return _right;
	}

	const glm::vec3& ViewerCamera::Up() const
	{
		return _up;
	}

	const glm::mat4& ViewerCamera::InvView() const
	{
		return _invView;
	}

	void ViewerCamera::PrintPositionalData()
	{
		printf("POS: vec3(%.3f, %.3f, %.3f), ROT: vec2(%.3f, %.3f)\n",
			_position.x, _position.y, _position.z,
			_xRotation, _yRotation);
	}
}
